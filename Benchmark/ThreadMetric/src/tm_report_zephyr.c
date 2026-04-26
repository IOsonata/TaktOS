/**-------------------------------------------------------------------------
 * @file    tm_report_zephyr.c
 *
 * @brief   Thread-Metric reporter for Zephyr  uses Zephyr's printk.
 *
 * Replaces tm_report.cpp for Zephyr builds. Same symbol set, same semantics,
 * but uses printk (Zephyr's panic-safe console output) instead of the
 * IOsonata BoardConsoleWrite path used by FreeRTOS / TaktOS / ThreadX.
 *
 * Do NOT also link tm_report.cpp  symbol clash on tm_printf, tm_putchar,
 * tm_check_fail, tm_report_init, tm_report_finish, tm_test_duration,
 * tm_test_cycles.
 * -------------------------------------------------------------------------*/
#include <stdarg.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "tm_api.h"

int tm_test_duration = TM_TEST_DURATION;
int tm_test_cycles   = TM_TEST_CYCLES;

void tm_report_init(void)             {}
void tm_report_init_argv(int argc, char **argv) { (void)argc; (void)argv; }
void tm_report_finish(void)           {}

void tm_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintk(fmt, ap);
    va_end(ap);
}

void tm_putchar(int c)
{
    printk("%c", (char)c);
}

void tm_check_fail(const char *msg)
{
    printk("%s", msg);
    for (;;) { /* halt */ }
}
