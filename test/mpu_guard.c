/**---------------------------------------------------------------------------
@file   mpu_guard.c

@brief  TaktOS MPU stack guard example

Demonstrates the MPU-aware handler path when the application supplies a writable handler table base to TaktOSInit().
The MPU detects a stack overflow before the guard canary is corrupted,
raising a MemManage fault (CFSR.MSTKERR or CFSR.DACCVIOL) immediately.

Requirements:
  - Application-owned writable handler table (e.g. RAM vector table on ARM)
  - Target: Cortex-M0+, M4F, or M7 for the MPU-aware ARM handler path
  - Stack buffers must be 32-byte aligned: __attribute__((aligned(32)))
    ensures the 32-byte guard region base aligns to an MPU boundary.
    4-byte alignment is sufficient when MPU is disabled.

How it works:
  TaktOSThreadCreate() computes pStackBottom as the 32-byte-aligned floor
  of the stack area. PendSV writes this as MPU region 7 (no-access, 32 bytes)
  on every context switch. If any task's SP descends into its guard region,
  the MPU faults before the write reaches the guard canary, giving a precise,
  deterministic overflow report.

  All kernel code and ISRs continue to operate via the privileged background
  memory map (PRIVDEFENA=1). Only the task's own stack guard is enforced.

MemManage fault handler:
  The application must provide a MemManage_Handler. Read SCB->CFSR
  (0xE000ED28) and SCB->MMFAR (0xE000ED34) to identify the faulting task
  and faulting address. Call TaktOSStackOverflowHandler() or log and reset.
  See the commented example below.

@author Hoang Nguyen Hoan
@date   Apr. 2026

@license MIT  TaktOS
----------------------------------------------------------------------------*/
#include <stdint.h>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"

/*  Stack buffers  32-byte aligned for MPU guard alignment  */
static uint8_t gTask1Mem[TAKTOS_THREAD_MEM_SIZE(512)] __attribute__((aligned(32)));
static uint8_t gTask2Mem[TAKTOS_THREAD_MEM_SIZE(512)] __attribute__((aligned(32)));

static TaktOSSem_t gSem;

/*  Task 1  normal operation  */
static void Task1(void *arg)
{
    (void)arg;
    for (;;)
    {
        TaktOSSemTake(&gSem, true, TAKTOS_WAIT_FOREVER);
        /* process event */
    }
}

/*  Task 2  signals Task 1  */
static void Task2(void *arg)
{
    (void)arg;
    for (;;)
    {
        TaktOSSemGive(&gSem, false);
        TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
    }
}

/*  MemManage fault handler  */
/* Override the weak default. Read CFSR to distinguish stack overflow
 * (MSTKERR, bit 4) from data access violation (DACCVIOL, bit 1).
 * MMFAR holds the faulting address when MMARVALID (bit 7) is set.
 *
 *  void MemManage_Handler(void)
 *  {
 *      volatile uint32_t cfsr = *((volatile uint32_t*)0xE000ED28u);
 *      volatile uint32_t mmfar = *((volatile uint32_t*)0xE000ED34u);
 *      (void)cfsr; (void)mmfar;
 *      TaktOSStackOverflowHandler(TaktOSCurrentThread());
 *      for (;;) {}
 *  }
 */

/*  main  */
int main(void)
{
    BSP_SystemInit();

    TaktOSInit(SystemCoreClock, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    /* Pass the application RAM vector-table base instead of 0u to bind the
     * MPU-aware PendSV / SVC handlers. 0u keeps the default no-MPU path. */

    TaktOSSemInit(&gSem, 0u, 1u);

    TaktOSThreadCreate(gTask1Mem, sizeof(gTask1Mem), Task1, NULL, 10u);
    TaktOSThreadCreate(gTask2Mem, sizeof(gTask2Mem), Task2, NULL,  5u);

    TaktOSStart();
    /* never reached */
    return 0;
}
