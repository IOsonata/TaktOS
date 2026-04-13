#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 3
#endif

#define configUSE_PREEMPTION                        1
#define configUSE_TIME_SLICING                      1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION     0
#define configUSE_IDLE_HOOK                         0
#define configUSE_TICK_HOOK                         0
#define configCPU_CLOCK_HZ                          ( ( uint32_t ) 128000000 )
#define configTICK_RATE_HZ                          ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                        32
#define configMINIMAL_STACK_SIZE                    256
#define configMAX_TASK_NAME_LEN                     8
#define configUSE_16_BIT_TICKS                      0
#define configIDLE_SHOULD_YIELD                     1
#define configUSE_MUTEXES                           1
#define configUSE_RECURSIVE_MUTEXES                 0
#define configUSE_COUNTING_SEMAPHORES               1
#define configUSE_TASK_NOTIFICATIONS                0
#define configQUEUE_REGISTRY_SIZE                   0
#define configGENERATE_RUN_TIME_STATS               0
#define configUSE_TRACE_FACILITY                    0
#define configCHECK_FOR_STACK_OVERFLOW              0
#define configUSE_MALLOC_FAILED_HOOK                0
#define configSUPPORT_STATIC_ALLOCATION             1
#define configSUPPORT_DYNAMIC_ALLOCATION            0
#define configKERNEL_PROVIDED_STATIC_MEMORY         1
#define configUSE_TIMERS                            0
#define configUSE_CO_ROUTINES                       0
#define configENABLE_TRUSTZONE                      0
#define configENABLE_MPU                            0
#define configENABLE_FPU                            1
#define configENABLE_MVE                            0
#define configRUN_FREERTOS_SECURE_ONLY              1
#define configNUMBER_OF_CORES                       1
#define configUSE_NEWLIB_REENTRANT                  0
#define configASSERT( x )                           do { if( !( x ) ) { __disable_irq(); for( ;; ) {} } } while( 0 )

#define configPRIO_BITS                             __NVIC_PRIO_BITS
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     ( ( 1U << configPRIO_BITS ) - 1U )
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2
#define configKERNEL_INTERRUPT_PRIORITY             ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY        ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define INCLUDE_vTaskPrioritySet                    0
#define INCLUDE_uxTaskPriorityGet                   0
#define INCLUDE_vTaskDelete                         0
#define INCLUDE_vTaskSuspend                        1
#define INCLUDE_xTaskDelayUntil                     0
#define INCLUDE_vTaskDelay                          1
#define INCLUDE_xTaskGetIdleTaskHandle              0
#define INCLUDE_xTaskAbortDelay                     0
#define INCLUDE_xQueueGetMutexHolder                0
#define INCLUDE_xSemaphoreGetMutexHolder            0
#define INCLUDE_xTaskGetHandle                      0
#define INCLUDE_uxTaskGetStackHighWaterMark         0
#define INCLUDE_uxTaskGetStackHighWaterMark2        0
#define INCLUDE_eTaskGetState                       0
#define INCLUDE_xTaskResumeFromISR                  1
#define INCLUDE_xTaskGetSchedulerState              1
#define INCLUDE_xTaskGetCurrentTaskHandle           1

#ifdef __cplusplus
}
#endif

#define configCHECK_HANDLER_INSTALLATION   0

#endif /* FREERTOS_CONFIG_H */
