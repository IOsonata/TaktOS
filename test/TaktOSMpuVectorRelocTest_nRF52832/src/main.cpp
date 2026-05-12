/**---------------------------------------------------------------------------
@file   main.cpp

@brief  nRF52832 Eclipse validation project for MPU handler binding and
        RAM vector relocation.

The application owns the RAM vector table. It copies the active table from
flash to RAM, switches VTOR to the RAM table, and passes that base address to
TaktOSInit(). The ARM port then patches only the reserved kernel slots:
  - SVCall
  - PendSV
  - SysTick

Validation flow in debugger:
  1. Run and confirm gVectorRelocated == 1 and gMpuHandlersBound == 1.
  2. Confirm gProducedCount / gConsumedCount keep increasing.
  3. After about 10 FaultThread wake cycles, the test auto-sets
     gTriggerMpuFault = 1.
  4. FaultThread writes to its MPU guard base. MemoryManagement_Handler
     increments gMemManageFaultCount and halts.

No UART or board-specific peripheral driver is required.

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
#include "TaktKernel.h"     // internal access to pStackBottom for the fault probe

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ                 64000000u
#endif

#define APP_VECTOR_WORD_COUNT            128u
#define APP_VECTOR_ALIGN_BYTES           512u

#define ARM_VTOR_ADDR                    0xE000ED08u
#define ARM_CFSR_ADDR                    0xE000ED28u
#define ARM_MMFAR_ADDR                   0xE000ED34u

#define ARM_VECTOR_SVC_INDEX            11u
#define ARM_VECTOR_PENDSV_INDEX         14u
#define ARM_VECTOR_SYSTICK_INDEX        15u

static uint32_t g_RamVectorTable[APP_VECTOR_WORD_COUNT]
    __attribute__((aligned(APP_VECTOR_ALIGN_BYTES)));

static uint8_t g_ProducerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));
static uint8_t g_ConsumerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));
static uint8_t g_FaultThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));

static TaktOSSem_t g_Sem;
static hTaktOSThread_t g_FaultThread = nullptr;

volatile uint32_t g_FlashVectorBase = 0u;
volatile uint32_t g_RamVectorBase = 0u;
volatile uint32_t g_VectorRelocated = 0u;
volatile uint32_t g_VectorSlotSVC = 0u;
volatile uint32_t g_VectorSlotPendSV = 0u;
volatile uint32_t g_VectorSlotSysTick = 0u;
volatile uint32_t g_MpuHandlersBound = 0u;
volatile uint32_t g_ProducedCount = 0u;
volatile uint32_t g_ConsumedCount = 0u;
volatile uint32_t g_LastConsumerTick = 0u;
volatile uint32_t g_TriggerMpuFault = 0u;
volatile uint32_t g_MemManageFaultCount = 0u;
volatile uint32_t g_MemManageCfsr = 0u;
volatile uint32_t g_MemManageMmar = 0u;

extern "C" void PendSV_Handler_MPU(void);
extern "C" void SVC_Handler_MPU(void);
extern "C" void TaktKernelTickHandler(void);

static inline uint32_t MmioRead32(uintptr_t Addr)
{
    return *((volatile uint32_t*)Addr);
}

static inline void MmioWrite32(uintptr_t Addr, uint32_t Value)
{
    *((volatile uint32_t*)Addr) = Value;
}

static inline void ArmSyncBarrier(void)
{
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");
}

static void RelocateVectorTableToRam(void)
{
    uint32_t *pSrc = (uint32_t*)(uintptr_t)MmioRead32(ARM_VTOR_ADDR);

    if (pSrc == nullptr)
    {
        pSrc = (uint32_t*)0u;
    }

    g_FlashVectorBase = (uint32_t)(uintptr_t)pSrc;

    for (uint32_t i = 0u; i < APP_VECTOR_WORD_COUNT; ++i)
    {
        g_RamVectorTable[i] = pSrc[i];
    }

    g_RamVectorBase = (uint32_t)(uintptr_t)g_RamVectorTable;
    MmioWrite32(ARM_VTOR_ADDR, g_RamVectorBase);
    ArmSyncBarrier();

    g_VectorRelocated = MmioRead32(ARM_VTOR_ADDR) == g_RamVectorBase ? 1u : 0u;
}

static void CaptureKernelVectorSlots(void)
{
    g_VectorSlotSVC = g_RamVectorTable[ARM_VECTOR_SVC_INDEX];
    g_VectorSlotPendSV = g_RamVectorTable[ARM_VECTOR_PENDSV_INDEX];
    g_VectorSlotSysTick = g_RamVectorTable[ARM_VECTOR_SYSTICK_INDEX];

    g_MpuHandlersBound =
        g_VectorSlotSVC == (uint32_t)(uintptr_t)SVC_Handler_MPU &&
        g_VectorSlotPendSV == (uint32_t)(uintptr_t)PendSV_Handler_MPU &&
        g_VectorSlotSysTick == (uint32_t)(uintptr_t)TaktKernelTickHandler ? 1u : 0u;
}

static void ProducerThread(void *pArg)
{
    (void)pArg;

    for (;;)
    {
        (void)__sync_add_and_fetch(&g_ProducedCount, 1u);
        TaktOSSemGive(&g_Sem, false);
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

static void ConsumerThread(void *pArg)
{
    (void)pArg;

    for (;;)
    {
        TaktOSSemTake(&g_Sem, true, TAKTOS_WAIT_FOREVER);
        (void)__sync_add_and_fetch(&g_ConsumedCount, 1u);
        g_LastConsumerTick = TaktOSTickCount();
    }
}

static void FaultThread(void *pArg)
{
    (void)pArg;

    static uint32_t sDelay = 0;

    for (;;)
    {
        TaktOSThreadSleep(TaktOSCurrentThread(), 50);
        sDelay++;

        if (sDelay >= 10u)
        {
            g_TriggerMpuFault = 1u;
        }

        if (g_TriggerMpuFault != 0u)
        {
            volatile uint32_t *pGuardBase =
                (volatile uint32_t*)((TaktOSThread_t*)TaktOSCurrentThread())->pStackBottom;

            *pGuardBase = 0xBAD00BADu;
        }
    }
}

int main(void)
{
    RelocateVectorTableToRam();

    TaktOSCfg_t cfg = {
        .KernClockHz     = APP_CORE_CLOCK_HZ,
        .HandlerBaseAddr = (uintptr_t)g_RamVectorTable,
    };
    if (TaktOSInit(&cfg) != TAKTOS_OK)
    {
        for (;;)
        {
        }
    }

    CaptureKernelVectorSlots();

    TaktOSSemInit(&g_Sem, 0u, 1u);

    TaktOSThreadCreate(g_ProducerThreadMem, sizeof(g_ProducerThreadMem),
                       ProducerThread, NULL, TAKTOS_PRIORITY_LOW);
    TaktOSThreadCreate(g_ConsumerThreadMem, sizeof(g_ConsumerThreadMem),
                       ConsumerThread, NULL, TAKTOS_PRIORITY_NORMAL);
    g_FaultThread = TaktOSThreadCreate(g_FaultThreadMem, sizeof(g_FaultThreadMem),
                                       FaultThread, NULL, TAKTOS_PRIORITY_LOWEST);

    (void)g_FaultThread;

    TaktOSStart();

    for (;;)
    {
    }
}

extern "C" void MemoryManagement_Handler(void)
{
    (void)__sync_add_and_fetch(&g_MemManageFaultCount, 1u);
    g_MemManageCfsr = MmioRead32(ARM_CFSR_ADDR);
    g_MemManageMmar = MmioRead32(ARM_MMFAR_ADDR);

    for (;;)
    {
    }
}
