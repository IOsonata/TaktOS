/**---------------------------------------------------------------------------
@file   basic_cpp.cpp

@brief  Basic TaktOS example  C++ API

Same scenario as basic_c.c using the C++ wrapper classes.
SignalThread gives the semaphore every 100 ms; WorkerThread takes it and
records the wakeup tick.

Shows:
  TaktOSThread::Create
  TaktOSSem::Init / Give / Take
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
#include <cstddef>
#include <cstdint>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

static uint8_t gSignalThreadMem[TAKTOS_THREAD_MEM_SIZE(384u)] __attribute__((aligned(4)));
static uint8_t gWorkerThreadMem[TAKTOS_THREAD_MEM_SIZE(384u)] __attribute__((aligned(4)));

static TaktOSThread gSignalThread;
static TaktOSThread gWorkerThread;
static TaktOSSem    gWorkSem;

volatile uint32_t gSignalCount = 0u;
volatile uint32_t gWorkCount = 0u;
volatile uint32_t gLastWakeTick = 0u;
volatile uint32_t gMaxObservedLag = 0u;

static void SignalThreadEntry(void *)
{
    for (;;)
    {
        ++gSignalCount;
        gWorkSem.Give(false);
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

static void WorkerThreadEntry(void *)
{
    for (;;)
    {
        gWorkSem.Take(true, TAKTOS_WAIT_FOREVER);

        ++gWorkCount;
        gLastWakeTick = TaktOSTickCount();

        if (gSignalCount >= gWorkCount)
        {
            uint32_t lag = gSignalCount - gWorkCount;
            if (lag > gMaxObservedLag)
            {
                gMaxObservedLag = lag;
            }
        }
    }
}

int main()
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    gWorkSem.Init(0u, 1u);

    gSignalThread.Create(gSignalThreadMem, sizeof(gSignalThreadMem),
                         SignalThreadEntry, nullptr, TAKTOS_PRIORITY_LOW);
    gWorkerThread.Create(gWorkerThreadMem, sizeof(gWorkerThreadMem),
                         WorkerThreadEntry, nullptr, TAKTOS_PRIORITY_NORMAL);

    TaktOSStart();
}
