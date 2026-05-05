#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* -------------------------------------------------------------------------- *
 *  FreeRTOS kernel configuration for STM32F0308-DISCO / STM32F030R8 @ 48 MHz.
 *
 *  Dynamic allocation build.  STM32F030R8 has only 8 KB of SRAM, so the
 *  FreeRTOS port keeps the 4 KB heap_4 arena but does not also reserve the
 *  caller-provided static KVB stack buffers.  The shared KVB tests still
 *  pass their requested stack sizes through the port macros; xTaskCreate()
 *  receives those sizes directly and allocates the actual stacks from the
 *  FreeRTOS heap.
 *
 *  Cortex-M0 (ARMv6-M).  No NVIC priority bits worth subdividing for the
 *  syscall mask — FreeRTOS uses the standard PendSV/SVC/SysTick priority
 *  scheme.
 *
 *  Modeled on the STM32L475VG TM reference; adapted for M0 by:
 *    - dropping configMAX_SYSCALL_INTERRUPT_PRIORITY (M0 has no BASEPRI;
 *      FreeRTOS-CM0 uses CPSID/CPSIE for the syscall barrier instead)
 *    - dropping configUSE_PORT_OPTIMISED_TASK_SELECTION (depends on CLZ
 *      which the M0 base ISA does not have)
 *    - shrinking configMINIMAL_STACK_SIZE to fit 8 KB SRAM
 *    - matching tick rate and 1000 Hz used by KVB / TaktOS counterpart.
 * -------------------------------------------------------------------------- */

#include <stdint.h>

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  0
#define configCPU_CLOCK_HZ                      ( ( uint32_t ) 48000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                64    /* words; 256 B idle stack */
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

/* -------------------------------------------------------------------------- *
 *  Newer FreeRTOS-Kernel ARM_CMx ports require these to be defined
 *  explicitly (the portmacro.h in V11.2+ / GitHub main checks each
 *  with #ifndef ... #error).  On Cortex-M0 (ARMv6-M) all three are 0:
 *
 *    - configENABLE_MPU       : Cortex-M0 has no MPU at all (M0+ has one,
 *                               M0 does not).  STM32F030 is plain M0.
 *    - configENABLE_FPU       : Cortex-M0 has no FPU.
 *    - configENABLE_TRUSTZONE : Cortex-M0 has no TrustZone.
 * -------------------------------------------------------------------------- */
#define configENABLE_MPU                        0
#define configENABLE_FPU                        0
#define configENABLE_TRUSTZONE                  0

#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) 4096 )   /* 4 KB heap_4 arena */

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
 * IOsonata's STM32F030 startup file declares SVC_Handler / PendSV_Handler /
 * SysTick_Handler as weak aliases of Default_Handler.  These macros redirect
 * FreeRTOS's internally-named handlers (vPortSVCHandler, etc.) to those
 * standard CMSIS names so the strong FreeRTOS implementations override the
 * weak IOsonata defaults at link time.  Same pattern as the TM FreeRTOS
 * STM32L475VG reference.
 * -------------------------------------------------------------------------- */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for( ;; ) {} } } while (0)

#endif /* FREERTOS_CONFIG_H */
