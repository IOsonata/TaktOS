#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* -------------------------------------------------------------------------- *
 * FreeRTOS kernel configuration for NUCLEO-H753ZI / STM32H753ZI @ 64 MHz.
 *
 *  - configCPU_CLOCK_HZ     : 64 MHz (HSI reset default, no PLL bring-up)
 *  - configTICK_RATE_HZ     : 1000 Hz (matches Thread-Metric harness)
 *  - configPRIO_BITS        : 4  (STM32H7 NVIC implementable priority bits)
 *  - PendSV / SVC / SysTick : use the FreeRTOS Cortex-M7 r0p1 port versions,
 *                             linked via the macro remaps at the bottom.
 *                             They override the weak aliases in
 *                             startup_stm32h753.S at link time.
 *
 * The ARM_CM7/r0p1 port is the correct FreeRTOS port for all STM32H7 silicon;
 * the r0p1-specific errata workaround is harmless on later revisions.
 * -------------------------------------------------------------------------- */

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TIME_SLICING                  0
#define configCPU_CLOCK_HZ                      ( ( uint32_t ) 64000000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    32
#define configMINIMAL_STACK_SIZE                128
#define configMAX_TASK_NAME_LEN                 8
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_NEWLIB_REENTRANT              0
#define configUSE_TIMERS                        0

#define configSUPPORT_DYNAMIC_ALLOCATION        0
#define configSUPPORT_STATIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                   0

#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0

/* NVIC priority configuration.
 *   - STM32H7 has 4 implementable priority bits -> 16 discrete levels.
 *   - configKERNEL_INTERRUPT_PRIORITY      = 15 << (8-4) = 0xF0  (lowest).
 *   - configMAX_SYSCALL_INTERRUPT_PRIORITY = 5  << (8-4) = 0x50.
 *   - IRQs with numeric priority >= 0x50 may not call FromISR() APIs;
 *     TIM7_IRQ is at 0xC0 in both tm_port_*.c files, so it is safe.
 */
#define configPRIO_BITS                              4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY              \
        ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY         \
        ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xTaskGetSchedulerState          1

/* Route the three system-exception vector names to the FreeRTOS port's
 * implementations. The startup file declares these as weak aliases of
 * Default_Handler; these macros cause the FreeRTOS port to emit strong
 * symbols under the same names, which wins at link time.                   */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for( ;; ) {} } } while (0)

#endif /* FREERTOS_CONFIG_H */
