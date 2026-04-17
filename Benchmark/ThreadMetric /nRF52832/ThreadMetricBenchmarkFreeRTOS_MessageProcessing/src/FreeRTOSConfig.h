#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

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

#define configPRIO_BITS                         3
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xTaskGetSchedulerState          1

#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for( ;; ) {} } } while (0)

#endif
