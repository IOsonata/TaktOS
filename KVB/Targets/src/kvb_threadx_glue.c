/**-------------------------------------------------------------------------
 * @file    kvb_threadx_glue.c
 *
 * @brief   Small C-side glue for ThreadX integration on KVB targets.
 *
 *   PendSV_Handler dispatch.
 *
 *      Eclipse ThreadX's GNU Cortex-M ports export `PendSV_Handler`
 *      directly as the global symbol for the context-switch handler.
 *      IOsonata's vector table declares `PendSV_Handler` as a weak
 *      symbol; the linker resolves to ThreadX's strong implementation
 *      automatically.  No alias or wrapper is needed in this file.
 *
 *   Stack-overflow halt path.
 *
 *      Activated when TX_ENABLE_STACK_CHECKING is defined in tx_user.h
 *      AND a corruption is detected at a context-switch point.
 *      The handler runs on the OFFENDING THREAD'S ALREADY-CORRUPTED
 *      STACK, so it MUST NOT make any kernel calls (no semaphore,
 *      mutex, queue, yield, or sleep), and MUST NOT call any logging
 *      function that yields internally.
 *
 *      An earlier version of this handler called kvb_platform_log_write
 *      to print the offending thread's name.  IOsonata's UART driver
 *      yields the thread when waiting for FIFO space, which re-enters
 *      the scheduler on a corrupted stack and triggers a SECOND stack
 *      error during _tx_thread_relinquish - producing a wild PC jump
 *      and HardFault.  Removed.
 *
 *      The current handler:
 *        1. Captures the offending thread's name into a global volatile
 *           buffer (a plain inline byte copy, no kernel calls).
 *        2. Sets a global flag.
 *        3. Disables IRQs (CPSID I).
 *        4. Issues BKPT #0 so an attached debugger halts here cleanly.
 *        5. Spins forever, in case no debugger is attached.
 *
 *      To diagnose after a halt: attach the debugger, look at the
 *      values of g_kvb_overflow_active and g_kvb_overflow_thread_name.
 * -------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#include "tx_api.h"

/* Public diagnostic globals.  Inspected from a debugger after a stack
 * overflow halt.  Volatile because the spin loop never touches them
 * again - we want the debugger to see whatever the handler stored,
 * not a register-cached optimisation. */
volatile int  g_kvb_overflow_active = 0;
volatile char g_kvb_overflow_thread_name[32];

static void kvb_threadx_stack_error_handler_impl(TX_THREAD *thread)
{
    /* Step 1: snapshot the offending thread name into a global, using
     * only inline byte copies - no library calls, no kernel calls. */
    g_kvb_overflow_active = 1;

    if (thread != NULL && thread->tx_thread_name != NULL) {
        const char *src = thread->tx_thread_name;
        unsigned i;
        for (i = 0u; i < (sizeof(g_kvb_overflow_thread_name) - 1u); ++i) {
            char c = src[i];
            g_kvb_overflow_thread_name[i] = c;
            if (c == '\0') {
                break;
            }
        }
        g_kvb_overflow_thread_name[sizeof(g_kvb_overflow_thread_name) - 1u] = '\0';
    } else {
        const char unknown[] = "<unknown>";
        unsigned i;
        for (i = 0u; i < sizeof(unknown); ++i) {
            g_kvb_overflow_thread_name[i] = unknown[i];
        }
    }

    /* Step 2: disable IRQs so PendSV cannot fire while we are halted. */
    __asm volatile ("cpsid i" ::: "memory");

    /* Step 3: breakpoint for an attached debugger.  Falls through to
     * the spin loop on a system with no debugger. */
    __asm volatile ("bkpt #0");

    for (;;) {
        /* spin */
    }
}

/* Register the stack-error handler with ThreadX.  Called from
 * tx_application_define (in kvb_port_threadx.c) at startup, kept here
 * so the registration symbol lives next to the implementation. */
void kvb_threadx_install_stack_error_handler(void)
{
    (void)tx_thread_stack_error_notify(kvb_threadx_stack_error_handler_impl);
}
