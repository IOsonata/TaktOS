/**-------------------------------------------------------------------------
 * @file    kvb_freertos_hooks.c
 *
 * @brief   FreeRTOS application hooks for KVB FreeRTOS-based projects.
 *
 * Provides:
 *   - vApplicationGetIdleTaskMemory  (only under configSUPPORT_STATIC_ALLOCATION=1)
 *   - vApplicationStackOverflowHook  (only under configCHECK_FOR_STACK_OVERFLOW != 0)
 *
 * Timer task is disabled (configUSE_TIMERS=0 in FreeRTOSConfig.h), so
 * vApplicationGetTimerTaskMemory is not provided.  Defining it would
 * make the linker pull in the timer task code path even though no
 * timer ever fires.
 *
 * This file is shared across all FreeRTOS-based KVB targets (STM32F0308,
 * future M4/M33 ports).  Per-MCU files supply only the things that
 * actually differ: pin map, clock source, identification strings.
 * -------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "kvb_platform_port.h"
#include <stddef.h>
#include <string.h>

/* The idle-task memory provider hook is only meaningful when
 * configSUPPORT_STATIC_ALLOCATION = 1.  Under dynamic-allocation builds
 * (heap_4.c present, configTOTAL_HEAP_SIZE > 0) FreeRTOS allocates the
 * idle task internally, so this hook is dead code in that mode and we
 * guard it out completely to avoid pulling unused buffers into BSS. */
#if (configSUPPORT_STATIC_ALLOCATION == 1)

/* Static idle task buffers.  The minimum stack size for the idle task is
 * configMINIMAL_STACK_SIZE (defined in FreeRTOSConfig.h).  On M0 with the
 * 64-word minimum, this is 256 B — the smallest stack any task in the
 * system gets, which is fine because the idle task only runs in the
 * scheduler-has-nothing-else-to-do gap and has a trivial body.
 *
 * Both the TCB and the stack must be aligned to portBYTE_ALIGNMENT
 * (8 bytes on Cortex-M).  Without this, the idle task creation hits
 * the same M0 hardfault that any other static task hits when the
 * caller-supplied buffer is only 4-byte aligned.  See FreeRTOS bug
 * #144 for the full pattern.  The kernel-port and types-header use
 * the same attribute on every static buffer KVB hands to FreeRTOS. */
static StaticTask_t s_idle_tcb __attribute__((aligned(portBYTE_ALIGNMENT)));
static StackType_t  s_idle_stack[configMINIMAL_STACK_SIZE]
    __attribute__((aligned(portBYTE_ALIGNMENT)));

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &s_idle_tcb;
    *ppxIdleTaskStackBuffer = s_idle_stack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

#endif /* configSUPPORT_STATIC_ALLOCATION == 1 */


/* Stack overflow diagnostic hook.  When configCHECK_FOR_STACK_OVERFLOW
 * is non-zero, FreeRTOS calls this from the context-switch path the
 * moment it detects either:
 *   level 1 — the outgoing task's PSP has descended past the stack base
 *   level 2 — the canary pattern at the bottom of the stack has been
 *             overwritten (set by the kernel at task creation)
 *
 * We log the offending task name to UART so the failure is identifiable,
 * then halt with interrupts disabled so the message is not overwritten.
 * Calling kvb_platform_log_write from this context is safe because UART
 * output is purely register-mapped polled writes — no FreeRTOS APIs,
 * no interrupts, no malloc. */
#if (configCHECK_FOR_STACK_OVERFLOW != 0)

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;

    static const char prefix[] = "[KVB] STACK_OVERFLOW task=";
    static const char suffix[] = "\r\n";

    kvb_platform_log_write(prefix, sizeof(prefix) - 1u);
    if (pcTaskName != NULL) {
        size_t name_len = strnlen(pcTaskName, configMAX_TASK_NAME_LEN);
        kvb_platform_log_write(pcTaskName, name_len);
    }
    kvb_platform_log_write(suffix, sizeof(suffix) - 1u);
    kvb_platform_log_flush();

    taskDISABLE_INTERRUPTS();
    for (;;) { /* spin */ }
}

#endif /* configCHECK_FOR_STACK_OVERFLOW != 0 */
