/**---------------------------------------------------------------------------
@file   mpu_guard.c

@brief  TaktOS MPU stack guard example  C API

ProducerThread posts a semaphore every 100 ms; ConsumerThread takes it.
The MPU guard region sits at the bottom of each thread stack. Any write
below the guard fires a MemManage fault, caught by MemManage_Handler,
which increments gMemManageFaultCount and halts. Pass a nonzero
application-owned writable handler table base to TaktOSInit() if the target
port needs dynamic handler patching; 0u keeps the default statically linked
handler path.

Stack buffer alignment must match TAKTOS_STACK_GUARD_ALIGN:
  256 bytes on Cortex-M0/M0+ (ARMv6-M minimum MPU region size)
   32 bytes on Cortex-M4/M7/M33/M55

Inspect the volatile counters in a debugger; no UART or board driver needed.

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

#ifndef TAKTOS_STACK_GUARD_ALIGN
#  if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)
#    define TAKTOS_STACK_GUARD_ALIGN 256u
#  else
#    define TAKTOS_STACK_GUARD_ALIGN 32u
#  endif
#endif

static uint8_t gConsumerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));
static uint8_t gProducerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));

static TaktOSSem_t gSem;

volatile uint32_t gProducedCount = 0u;
volatile uint32_t gConsumedCount = 0u;
volatile uint32_t gLastWakeTick = 0u;
volatile uint32_t gMemManageFaultCount = 0u;

static void ConsumerThread(void *arg)
{
    (void)arg;
    for (;;)
    {
        TaktOSSemTake(&gSem, true, TAKTOS_WAIT_FOREVER);
        ++gConsumedCount;
        gLastWakeTick = TaktOSTickCount();
    }
}

static void ProducerThread(void *arg)
{
    (void)arg;
    for (;;)
    {
        ++gProducedCount;
        TaktOSSemGive(&gSem, false);
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

void TaktOS_MPU_Example(uint32_t coreClockHz)
{
    TaktOSInit(coreClockHz, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    TaktOSSemInit(&gSem, 0u, 1u);

    TaktOSThreadCreate(gConsumerThreadMem, sizeof(gConsumerThreadMem),
                       ConsumerThread, NULL, TAKTOS_PRIORITY_NORMAL);
    TaktOSThreadCreate(gProducerThreadMem, sizeof(gProducerThreadMem),
                       ProducerThread, NULL, TAKTOS_PRIORITY_LOW);

    TaktOSStart();
}

int main(void)
{
    TaktOS_MPU_Example(APP_CORE_CLOCK_HZ);
}

void MemManage_Handler(void)
{
    ++gMemManageFaultCount;
    for (;;)
    {
    }
}
