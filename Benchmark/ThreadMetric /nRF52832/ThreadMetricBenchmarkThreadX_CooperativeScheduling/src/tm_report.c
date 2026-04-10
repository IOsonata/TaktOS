/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "tm_api.h"

int tm_test_duration = TM_TEST_DURATION;
int tm_test_cycles = TM_TEST_CYCLES;

void tm_report_init(void)
{
}

void tm_report_init_argv(int argc, char **argv)
{
    (void) argc;
    (void) argv;
}

static void tm_print_unsigned_long(unsigned long val)
{
    char buf[21];
    int i = 0;

    if (val == 0) {
        tm_putchar('0');
        return;
    }

    while (val > 0) {
        buf[i++] = (char) ('0' + (val % 10));
        val /= 10;
    }

    while (i > 0)
        tm_putchar(buf[--i]);
}

static void tm_print_int(int val)
{
    if (val < 0) {
        tm_putchar('-');
        tm_print_unsigned_long((unsigned long) (-(unsigned) val));
    } else {
        tm_print_unsigned_long((unsigned long) val);
    }
}

void tm_printf(const char *fmt, ...)
{
    va_list ap;
    const char *s;

    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            tm_putchar(*fmt++);
            continue;
        }
        fmt++;
        switch (*fmt) {
        case 'l':
            fmt++;
            if (*fmt == 'u') {
                tm_print_unsigned_long(va_arg(ap, unsigned long));
            } else {
                tm_putchar('%');
                tm_putchar('l');
                if (*fmt == '\0')
                    goto done;
                tm_putchar(*fmt);
            }
            break;
        case 'd':
            tm_print_int(va_arg(ap, int));
            break;
        case 's':
            s = va_arg(ap, char *);
            if (s == NULL)
                s = "(null)";
            while (*s)
                tm_putchar(*s++);
            break;
        case '%':
            tm_putchar('%');
            break;
        case '\0':
            goto done;
        default:
            tm_putchar('%');
            tm_putchar(*fmt);
            break;
        }
        fmt++;
    }
done:
    va_end(ap);
}

void tm_report_finish(void)
{
    for (;;) {}
}

void tm_check_fail(const char *msg)
{
    while (*msg)
        tm_putchar(*msg++);
    for (;;) {}
}
