#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* -------------------------------------------------------------------------- *
 *  FreeRTOS kernel configuration for nRF52-DK (PCA10040) / nRF52832 @ 64 MHz.
 *
 *  Cortex-M4F (ARMv7E-M) with FPU and MPU.  This config disables the FPU
 *  context save by default — KVB tests are integer-only, and saving S0-S15
 *  on every context switch costs ~30 cycles even when the task never uses
 *  the FPU.  Tasks that need FPU must enable per-task lazy stacking via
 *  portTASK_USES_FLOATING_POINT() inside the task entry; KVB does not.
 *
 *  Modeled on the STM32L475VG TM reference and the STM32F0308 KVB build,
 *  adapted for M4 by:
 *    - using BASEPRI for the syscall mask (configMAX_SYSCALL_INTERRUPT_PRIORITY)
 *    - enabling configUSE_PORT_OPTIMISED_TASK_SELECTION (CLZ available)
 *    - matching the proven Benchmark/ThreadMetric/nRF52832 FreeRTOS
 *      reference: pure static allocation
 *      (configSUPPORT_DYNAMIC_ALLOCATION=0 + configSUPPORT_STATIC_ALLOCATION=1
 *       + configTOTAL_HEAP_SIZE=0).  No heap_4 needed.
 *    - matching tick rate / 1000 Hz used by KVB / TaktOS / ThreadX counterparts
 * -------------------------------------------------------------------------- */

#include <stdint.h>

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configCPU_CLOCK_HZ                      ( ( uint32_t ) 64000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                128   /* words; 512 B idle stack */
#define configMAX_TASK_NAME_LEN                 8
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_NEWLIB_REENTRANT              0
#define configUSE_TIMERS                        0   /* timer task off, saves SRAM */

/* CLZ is part of base ARMv7E-M, so the bitmap-priority optimisation
 * applies on M4.  Saves ~12 cycles per scheduler entry vs the generic
 * port at the cost of one CLZ + bit test per ready-state update. */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* -------------------------------------------------------------------------- *
 *  ARMv7E-M / Cortex-M4 specifics.
 *
 *  configMAX_SYSCALL_INTERRUPT_PRIORITY: priority threshold below which an
 *  ISR may call FreeRTOS API.  Nordic SoftDevice traditionally claims
 *  priorities 0,1,4 — KVB does NOT use the SoftDevice, so the upper bits
 *  are free.  We set 5 (encoded as (5 << 5) = 0xa0 with NVIC_PRIO_BITS=3)
 *  to leave room for higher-than-OS interrupts if the user adds them.
 *
 *  configKERNEL_INTERRUPT_PRIORITY: priority of PendSV / SysTick — must be
 *  the LOWEST (255 = (7 << 5) = 0xe0 with NVIC_PRIO_BITS=3).  This lets any
 *  other ISR preempt the kernel handlers, which is the standard FreeRTOS
 *  convention for ARM Cortex-M.
 *
 *  FPU: this build uses the ARM_CM4F port (portable/GCC/ARM_CM4F).  That
 *  port unconditionally enables the FPU in vPortEnableFPU during scheduler
 *  start, regardless of any configENABLE_FPU macro.  KVB tests are
 *  integer-only, so the FPU never actually gets touched after enable; the
 *  cost is one CR write at boot.  We compile with -mfloat-abi=hard +
 *  -mfpu=fpv4-sp-d16, matching the TaktOS / ThreadX / Zephyr KVB ports
 *  on this target (see Eclipse .cproject `fpu.abi.hard`).
 *
 *  Lazy stacking: portmacro.h's portTASK_USES_FLOATING_POINT() macro lets
 *  individual tasks opt in to FPU context save on context switch.  KVB's
 *  tasks don't call it, so PendSV does not save S0–S31 on switch — the
 *  FPCA bit stays clear and the lazy-stacking hardware path is a no-op.
 *  The unified-port configENABLE_FPU / configENABLE_MPU / configENABLE_TRUSTZONE
 *  macros are NOT consumed by ARM_CM4F and are intentionally omitted here.
 * -------------------------------------------------------------------------- */
#define configPRIO_BITS                         3      /* nRF52 uses 3 priority bits */
#define configKERNEL_INTERRUPT_PRIORITY         (7 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (5 << (8 - configPRIO_BITS))

#define configSUPPORT_DYNAMIC_ALLOCATION        0
#define configSUPPORT_STATIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) 0 )

/* In FreeRTOS-Kernel V11+, configKERNEL_PROVIDED_STATIC_MEMORY defaults to
 * 1, which makes tasks.c emit its own vApplicationGetIdleTaskMemory and
 * vApplicationGetTimerTaskMemory.  KVB ships an application-side
 * vApplicationGetIdleTaskMemory in src/kvb_freertos_hooks.c (its TCB and
 * stack are dimensioned to KVB needs), so we disable the kernel-provided
 * default to avoid a multiple-definition link error. */
#define configKERNEL_PROVIDED_STATIC_MEMORY     0

#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xSemaphoreGetMutexHolder        1   /* needed for kvb_mutex_unlock non-owner check */

/* -------------------------------------------------------------------------- *
 * Vector table aliasing.
 *
 * IOsonata's nRF52832 startup file declares SVC_Handler / PendSV_Handler /
 * SysTick_Handler as weak aliases of Default_Handler.  These macros redirect
 * FreeRTOS's internally-named handlers (vPortSVCHandler, etc.) to those
 * standard CMSIS names so the strong FreeRTOS implementations override the
 * weak IOsonata defaults at link time.  Same pattern as the STM32F0308 KVB
 * build and the TM FreeRTOS STM32L475VG reference.
 * -------------------------------------------------------------------------- */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for( ;; ) {} } } while (0)

#endif /* FREERTOS_CONFIG_H */
