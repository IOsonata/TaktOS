/**-------------------------------------------------------------------------
 * @file	tm_report.cpp
 *
 * @brief	Thread-Metric reporter  direct IOsonata UART, no retarget.
 *
 * Replaces Microsoft's tm_report.c (same directory, same harness function
 * set). Keep all other Microsoft test modules unchanged  basic_processing.c,
 * cooperative_scheduling.c, memory_allocation.c, message_processing.c,
 * mutex_processing.c, mutex_barging_test.c, preemptive_scheduling.c,
 * synchronization_processing.c. Each Eclipse project picks exactly one
 * of those test modules plus this tm_report.cpp. Do NOT also link
 * Microsoft's tm_report.c  symbol clash on tm_printf, tm_putchar,
 * tm_check_fail, tm_report_init, tm_report_finish, tm_test_duration,
 * tm_test_cycles.
 *
 * Uses the C++ UART object g_Uart. The C-API functions UARTvprintf /
 * UARTTx accept g_Uart via the implicit operator UARTDev_t*() defined
 * on the UART class (uart.h line 249).
 * -------------------------------------------------------------------------*/
#include <stdarg.h>

#include "coredev/uart.h"
#include "tm_api.h"

extern UART g_Uart;

int tm_test_duration = TM_TEST_DURATION;
int tm_test_cycles   = TM_TEST_CYCLES;

extern "C" void tm_report_init(void)             {}
extern "C" void tm_report_init_argv(int, char**) {}
extern "C" void tm_report_finish(void)           {}

extern "C" void tm_printf(const char *fmt, ...)
{
    /* va_list path  UART::printf is variadic-only, so we go through the
     * C-API UARTvprintf. g_Uart's operator UARTDev_t*() supplies the arg. */
    va_list ap;
    va_start(ap, fmt);
    UARTvprintf(g_Uart, fmt, ap);
    va_end(ap);
}

#if 0
extern "C" void tm_putchar(int c)
{
   // uint8_t ch = (uint8_t)c;
    (void)g_Uart.Tx((uint8_t*)&c, 1);
}
#endif

extern "C" void tm_check_fail(const char *msg)
{
    g_Uart.printf("%s", msg);
    for (;;) { /* halt */ }
}
