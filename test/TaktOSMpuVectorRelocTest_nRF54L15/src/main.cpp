/**---------------------------------------------------------------------------
@file   main.cpp

@brief  nRF54L15 Eclipse validation project for MPU handler binding and
        RAM vector relocation  Cortex-M33 / ARMv8-M Mainline.

Sister to TaktOSMpuVectorRelocTest_nRF52832 (Cortex-M4 / ARMv7-M).  Exercises
the same flow on the new ARMv8-M MPU path:
  - RBAR + RLAR encoding (instead of v7-M RBAR + RASR)
  - AP = 10 (RO-priv, XN = 1) guard region  writes fault, reads are benign
  - MAIR0 AttrIndx = 0  Normal WB
  - PSPLIM written alongside MPU region 7 on every switch
  - SVC_Handler_MPU installs both PSPLIM and MPU #7 before the first task runs

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

M33-specific expectations (different from the M4 test):
  - Even WITHOUT the MPU-aware handlers being bound, PSPLIM alone catches
    SP descent past pStackBottom on M33.  This test specifically exercises
    the MPU path  a deliberate indexed WRITE at pStackBottom from a task
    whose SP is above the limit.  PSPLIM cannot catch this; only MPU
    region 7 will fire.  A passing result is therefore a stronger claim
    on M33 than on M4 (M4 has neither PSPLIM nor any other backstop).
  - The fault comes from AP = 10 (RO-priv), not from AP = 000 (no access).
    ARMv8-M does not provide AP = 000; writes fault, privileged reads
    would not.  The FaultThread probe is a write  correct for the
    threat model.

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
#define APP_CORE_CLOCK_HZ                 128000000u
#endif

// ARMv8-M MPU region-7 guard size (32-byte minimum region).  Defined by
// MPU_GUARD_SIZE_BYTES in takt_mpu_v8m.h; duplicated here only for clarity
// at the fault-probe site.
#ifndef APP_MPU_V8M_GUARD_SIZE_BYTES
#define APP_MPU_V8M_GUARD_SIZE_BYTES      32u
#endif

#define APP_VECTOR_WORD_COUNT            128u
#define APP_VECTOR_ALIGN_BYTES           512u

#define ARM_VTOR_ADDR                    0xE000ED08u
#define ARM_CFSR_ADDR                    0xE000ED28u
#define ARM_MMFAR_ADDR                   0xE000ED34u

#define ARM_VECTOR_SVC_INDEX            11u
#define ARM_VECTOR_PENDSV_INDEX         14u
#define ARM_VECTOR_SYSTICK_INDEX        15u

static uint32_t gRamVectorTable[APP_VECTOR_WORD_COUNT]
    __attribute__((aligned(APP_VECTOR_ALIGN_BYTES)));

static uint8_t gProducerThreadMem[TAKTOS_THREAD_STACK_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));
static uint8_t gConsumerThreadMem[TAKTOS_THREAD_STACK_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));
static uint8_t gFaultThreadMem[TAKTOS_THREAD_STACK_SIZE(512u)]
    __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)));

static TaktOSSem_t gSem;
static hTaktOSThread_t gFaultThread = nullptr;

volatile uint32_t gFlashVectorBase = 0u;
volatile uint32_t gRamVectorBase = 0u;
volatile uint32_t gVectorRelocated = 0u;
volatile uint32_t gVectorSlotSVC = 0u;
volatile uint32_t gVectorSlotPendSV = 0u;
volatile uint32_t gVectorSlotSysTick = 0u;
volatile uint32_t gMpuHandlersBound = 0u;
volatile uint32_t gProducedCount = 0u;
volatile uint32_t gConsumedCount = 0u;
volatile uint32_t gLastConsumerTick = 0u;
volatile uint32_t gTriggerMpuFault = 0u;
volatile uint32_t gMemManageFaultCount = 0u;
volatile uint32_t gMemManageCfsr = 0u;
volatile uint32_t gMemManageMmar = 0u;

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

    gFlashVectorBase = (uint32_t)(uintptr_t)pSrc;

    for (uint32_t i = 0u; i < APP_VECTOR_WORD_COUNT; ++i)
    {
        gRamVectorTable[i] = pSrc[i];
    }

    gRamVectorBase = (uint32_t)(uintptr_t)gRamVectorTable;
    MmioWrite32(ARM_VTOR_ADDR, gRamVectorBase);
    ArmSyncBarrier();

    gVectorRelocated = MmioRead32(ARM_VTOR_ADDR) == gRamVectorBase ? 1u : 0u;
}

static void CaptureKernelVectorSlots(void)
{
    gVectorSlotSVC = gRamVectorTable[ARM_VECTOR_SVC_INDEX];
    gVectorSlotPendSV = gRamVectorTable[ARM_VECTOR_PENDSV_INDEX];
    gVectorSlotSysTick = gRamVectorTable[ARM_VECTOR_SYSTICK_INDEX];

    gMpuHandlersBound =
        gVectorSlotSVC == (uint32_t)(uintptr_t)SVC_Handler_MPU &&
        gVectorSlotPendSV == (uint32_t)(uintptr_t)PendSV_Handler_MPU &&
        gVectorSlotSysTick == (uint32_t)(uintptr_t)TaktKernelTickHandler ? 1u : 0u;
}

static void ProducerThread(void *pArg)
{
    (void)pArg;

    for (;;)
    {
        gProducedCount += 1u;
        TaktOSSemGive(&gSem, false);
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

static void ConsumerThread(void *pArg)
{
    (void)pArg;

    for (;;)
    {
        TaktOSSemTake(&gSem, true, TAKTOS_WAIT_FOREVER);
        gConsumedCount += 1u;
        gLastConsumerTick = TaktOSTickCount();
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
            gTriggerMpuFault = 1u;
        }

        if (gTriggerMpuFault != 0u)
        {
            // Indexed write into the 32-byte guard region.  SP is above
            // pStackBottom, so PSPLIM does not fire.  MPU region 7 with
            // AP = 10 (RO-priv) is the only protection that catches this
            // on ARMv8-M  exactly what the M33 MPU path is supposed to
            // detect.
            volatile uint32_t *pGuardBase =
                (volatile uint32_t*)((TaktOSThread_t*)TaktOSCurrentThread())->pStackBottom;

            *pGuardBase = 0xBAD00BADu;
        }
    }
}

int main(void)
{
    RelocateVectorTableToRam();

    if (TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR,
                   (uintptr_t)gRamVectorTable) != TAKTOS_OK)
    {
        for (;;)
        {
        }
    }

    CaptureKernelVectorSlots();

    TaktOSSemInit(&gSem, 0u, 1u);

    TaktOSThreadCreate(gProducerThreadMem, sizeof(gProducerThreadMem),
                       ProducerThread, NULL, TAKTOS_PRIORITY_LOW);
    TaktOSThreadCreate(gConsumerThreadMem, sizeof(gConsumerThreadMem),
                       ConsumerThread, NULL, TAKTOS_PRIORITY_NORMAL);
    gFaultThread = TaktOSThreadCreate(gFaultThreadMem, sizeof(gFaultThreadMem),
                                      FaultThread, NULL, TAKTOS_PRIORITY_LOWEST);

    (void)gFaultThread;

    TaktOSStart();

    for (;;)
    {
    }
}

extern "C" void MemoryManagement_Handler(void)
{
    gMemManageFaultCount += 1u;
    gMemManageCfsr = MmioRead32(ARM_CFSR_ADDR);
    gMemManageMmar = MmioRead32(ARM_MMFAR_ADDR);

    for (;;)
    {
    }
}
