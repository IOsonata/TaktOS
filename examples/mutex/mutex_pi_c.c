/**---------------------------------------------------------------------------
@file   mutex_pi_c.c

@brief  Priority inheritance example  C API

Three-thread scenario that exercises priority inheritance:
  Low     takes the mutex first, holds it for ~20 ms
  High    blocks on the same mutex shortly after Low; should inherit
           Low's boosted priority, preventing Medium from running ahead
  Medium  runs free once started; should be suppressed while High waits

With priority inheritance active, gMediumIterationsBeforeUnlock should
stay small (Low task runs at High priority, so Medium stays blocked).
Without PI, Medium would accumulate many iterations before Low unlocks.

Inspect the volatile counters in a debugger.

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
#include <stddef.h>
#include <stdint.h>
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

static TaktOSMutex_t gMutex;
static TaktOSSem_t   gStartHighSem;
static TaktOSSem_t   gStartMediumSem;

volatile uint32_t gLowEnteredCritical = 0u;
volatile uint32_t gLowUnlocked = 0u;
volatile uint32_t gMediumIterations = 0u;
volatile uint32_t gMediumIterationsBeforeUnlock = 0u;
volatile uint32_t gHighBlockedCount = 0u;
volatile uint32_t gHighAcquiredCount = 0u;
volatile uint32_t gHighAcquireTick = 0u;
volatile uint32_t gLowUnlockTick = 0u;

static void LowThread(void *arg)
{
    (void)arg;

    for (;;)
    {
        TaktOSMutexLock(&gMutex, true, TAKTOS_WAIT_FOREVER);
        ++gLowEnteredCritical;

        TaktOSSemGive(&gStartHighSem, false);
        TaktOSSemGive(&gStartMediumSem, false);

        for (uint32_t i = 0u; i < 20u; ++i)
        {
            TaktOSThreadSleep(TaktOSCurrentThread(), 1u);
        }

        gMediumIterationsBeforeUnlock = gMediumIterations;
        gLowUnlockTick = TaktOSTickCount();
        ++gLowUnlocked;
        TaktOSMutexUnlock(&gMutex);

        TaktOSThreadSleep(TaktOSCurrentThread(), 50u);
    }
}

static void MediumThread(void *arg)
{
    (void)arg;
    TaktOSSemTake(&gStartMediumSem, true, TAKTOS_WAIT_FOREVER);

    for (;;)
    {
        ++gMediumIterations;
        TaktOSThreadSleep(TaktOSCurrentThread(), 1u);
    }
}

static void HighThread(void *arg)
{
    (void)arg;
    TaktOSSemTake(&gStartHighSem, true, TAKTOS_WAIT_FOREVER);

    ++gHighBlockedCount;
    TaktOSMutexLock(&gMutex, true, TAKTOS_WAIT_FOREVER);
    --gHighBlockedCount;

    ++gHighAcquiredCount;
    gHighAcquireTick = TaktOSTickCount();

    TaktOSMutexUnlock(&gMutex);

    for (;;)
    {
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

int main(void)
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    TaktOSMutexInit(&gMutex);
    TaktOSSemInit(&gStartHighSem, 0u, 1u);
    TaktOSSemInit(&gStartMediumSem, 0u, 1u);

    TaktOSThreadCreate(gLowThreadMem, sizeof(gLowThreadMem),
                       LowThread, NULL, TAKTOS_PRIORITY_LOW);
    TaktOSThreadCreate(gMediumThreadMem, sizeof(gMediumThreadMem),
                       MediumThread, NULL, TAKTOS_PRIORITY_NORMAL);
    TaktOSThreadCreate(gHighThreadMem, sizeof(gHighThreadMem),
                       HighThread, NULL, TAKTOS_PRIORITY_HIGH);

    TaktOSStart();
}
