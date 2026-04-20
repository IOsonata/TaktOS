/**---------------------------------------------------------------------------
@file   mutex_pi.cpp

@brief  Priority inheritance example  C++ API

Same three-thread PI scenario as mutex_pi_c.c using C++ wrapper classes.
Low takes the mutex and holds it for ~20 ms.  High blocks on it.
Medium runs free.  PI causes Low to inherit High's priority, keeping
Medium from running while High waits.

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
        gMutex.Lock(true, TAKTOS_WAIT_FOREVER);
        ++gLowEnteredCritical;

        gStartHighSem.Give(false);
        gStartMediumSem.Give(false);

        for (uint32_t i = 0u; i < 20u; ++i)
        {
            TaktOSThreadSleep(TaktOSCurrentThread(), 1u);
        }

        gMediumIterationsBeforeUnlock = gMediumIterations;
        gLowUnlockTick = TaktOSTickCount();
        ++gLowUnlocked;
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
    gMutex.Init();
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
