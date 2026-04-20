/**---------------------------------------------------------------------------
@file   basic_c.c

@brief  Basic TaktOS example  C API

Two threads communicate through a binary semaphore.
SignalThread gives the semaphore every 100 ms; WorkerThread takes it and
records the wakeup tick.  Inspect the volatile counters in a debugger to
verify signalling rate and lag.

Shows:
  TaktOSInit / TaktOSStart
  TaktOSThreadCreate
  TaktOSSemInit / TaktOSSemGive / TaktOSSemTake
  TaktOSThreadSleep

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

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

static uint8_t gSignalThreadMem[TAKTOS_THREAD_MEM_SIZE(384u)] __attribute__((aligned(4)));
static uint8_t gWorkerThreadMem[TAKTOS_THREAD_MEM_SIZE(384u)] __attribute__((aligned(4)));

static TaktOSSem_t gWorkSem;

volatile uint32_t gSignalCount = 0u;
volatile uint32_t gWorkCount = 0u;
volatile uint32_t gLastWakeTick = 0u;
volatile uint32_t gMaxObservedLagTicks = 0u;

static void SignalThread(void *arg)
{
    (void)arg;
    for (;;)
    {
        ++gSignalCount;
        TaktOSSemGive(&gWorkSem, false);
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

static void WorkerThread(void *arg)
{
    (void)arg;
    for (;;)
    {
        TaktOSSemTake(&gWorkSem, true, TAKTOS_WAIT_FOREVER);

        ++gWorkCount;
        gLastWakeTick = TaktOSTickCount();

        if (gSignalCount >= gWorkCount)
        {
            uint32_t lag = gSignalCount - gWorkCount;
            if (lag > gMaxObservedLagTicks)
            {
                gMaxObservedLagTicks = lag;
            }
        }
    }
}

int main(void)
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    TaktOSSemInit(&gWorkSem, 0u, 1u);

    TaktOSThreadCreate(gSignalThreadMem, sizeof(gSignalThreadMem),
                       SignalThread, NULL, TAKTOS_PRIORITY_LOW);
    TaktOSThreadCreate(gWorkerThreadMem, sizeof(gWorkerThreadMem),
                       WorkerThread, NULL, TAKTOS_PRIORITY_NORMAL);

    TaktOSStart();
}
