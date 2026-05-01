/**---------------------------------------------------------------------------
@file   mutex_pcp.cpp

@brief  Immediate Priority Ceiling Protocol example  C++ API

Three-thread bounded-priority-inversion scenario using the IPCP mutex
(equivalent to POSIX PTHREAD_PRIO_PROTECT). Low takes the mutex and holds
it for ~20 ms. High blocks on it. Medium runs at NORMAL priority and would
classically preempt Low — except that Low's effective priority is boosted
to the mutex Ceiling on Lock(), so Medium does not run until Low Unlocks
and the boost is removed.

Expected behaviour:
  - gMediumIterationsBeforeUnlock is small (Medium did not run during the
    critical section) — IPCP successfully bounded the inversion.
  - gHighAcquireTick is gLowUnlockTick + ~1 (High acquires immediately on
    Unlock).
  - High never observes Medium completing iterations while it waits.

Compare with the plain-mutex scenario: with TaktOSMutexInit() (no ceiling)
Medium would preempt Low for the full 20 ms hold, and gMediumIterations-
BeforeUnlock would grow to ~20.

Inspect gMediumIterationsBeforeUnlock and gHighAcquireTick in a debugger.

@author Nguyen Hoan Hoang
@date   Apr. 2026

@license

MIT License

Copyright (c) 2026 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/
#include <cstddef>
#include <cstdint>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

static uint8_t gLowThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));
static uint8_t gMediumThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));
static uint8_t gHighThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));

static TaktOSThread gLowThread;
static TaktOSThread gMediumThread;
static TaktOSThread gHighThread;
static TaktOSMutex  gMutex;
static TaktOSSem    gStartHighSem;
static TaktOSSem    gStartMediumSem;

volatile uint32_t gLowEnteredCritical = 0u;
volatile uint32_t gLowUnlocked = 0u;
volatile uint32_t gMediumIterations = 0u;
volatile uint32_t gMediumIterationsBeforeUnlock = 0u;
volatile uint32_t gHighBlockedCount = 0u;
volatile uint32_t gHighAcquiredCount = 0u;
volatile uint32_t gHighAcquireTick = 0u;
volatile uint32_t gLowUnlockTick = 0u;

static void LowThreadEntry(void *)
{
    for (;;)
    {
        // Lock() boosts effective priority to max(LOW, HIGH) = HIGH.
        gMutex.Lock(true, TAKTOS_WAIT_FOREVER);
        ++gLowEnteredCritical;

        gStartHighSem.Give(false);
        gStartMediumSem.Give(false);

        for (uint32_t i = 0u; i < 20u; ++i)
        {
            // 1 ms of work each iteration. Medium would normally preempt
            // here, but IPCP keeps this thread at HIGH priority for the
            // duration of the critical section.
            TaktOSThreadSleep(TaktOSCurrentThread(), 1u);
        }

        gMediumIterationsBeforeUnlock = gMediumIterations;
        gLowUnlockTick = TaktOSTickCount();
        ++gLowUnlocked;
        // Unlock() restores effective priority to BasePriority (LOW).
        // High is unblocked and runs immediately.
        gMutex.Unlock();

        TaktOSThreadSleep(TaktOSCurrentThread(), 50u);
    }
}

static void MediumThreadEntry(void *)
{
    gStartMediumSem.Take(true, TAKTOS_WAIT_FOREVER);

    for (;;)
    {
        ++gMediumIterations;
        TaktOSThreadSleep(TaktOSCurrentThread(), 1u);
    }
}

static void HighThreadEntry(void *)
{
    gStartHighSem.Take(true, TAKTOS_WAIT_FOREVER);

    ++gHighBlockedCount;
    gMutex.Lock(true, TAKTOS_WAIT_FOREVER);
    --gHighBlockedCount;

    ++gHighAcquiredCount;
    gHighAcquireTick = TaktOSTickCount();
    gMutex.Unlock();

    for (;;)
    {
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

int main()
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    // Ceiling = highest priority of any thread that will Lock this mutex.
    // Here HIGH is the maximum across Low, High (Medium does not Lock).
    gMutex.InitProtect(TAKTOS_PRIORITY_HIGH);

    gStartHighSem.Init(0u, 1u);
    gStartMediumSem.Init(0u, 1u);

    gLowThread.Create(gLowThreadMem, sizeof(gLowThreadMem),
                      LowThreadEntry, nullptr, TAKTOS_PRIORITY_LOW);
    gMediumThread.Create(gMediumThreadMem, sizeof(gMediumThreadMem),
                         MediumThreadEntry, nullptr, TAKTOS_PRIORITY_NORMAL);
    gHighThread.Create(gHighThreadMem, sizeof(gHighThreadMem),
                       HighThreadEntry, nullptr, TAKTOS_PRIORITY_HIGH);

    TaktOSStart();
}
