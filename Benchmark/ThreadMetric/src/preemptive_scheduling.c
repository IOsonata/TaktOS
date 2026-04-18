/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Modified: counter variables widened to uint64_t to prevent 32-bit overflow
 * during long-duration burn-in testing.  Original unsigned long overflows at
 * ~858 million iterations per thread (~6.9 hours at nRF52832 64 MHz scores).
 *
 * Printing uses %lu only  avoids va_arg(unsigned long long) alignment issues
 * on 32-bit ARM AAPCS with newlib-nano.  Safe because:
 *   - per-window delta is always ~5.2M (fits in uint32_t)
 *   - relative_time cast wraps after ~49 days (acceptable for burn-in)
 * Internal arithmetic stays uint64_t for correct determinism checking.
 */

/* Thread-Metric Component -- Preemptive Scheduling Test */
#include <stdint.h>
#include "tm_api.h"

volatile uint64_t tm_preemptive_thread_0_counter;
volatile uint64_t tm_preemptive_thread_1_counter;
volatile uint64_t tm_preemptive_thread_2_counter;
volatile uint64_t tm_preemptive_thread_3_counter;
volatile uint64_t tm_preemptive_thread_4_counter;

void tm_preemptive_thread_0_entry(void);
void tm_preemptive_thread_1_entry(void);
void tm_preemptive_thread_2_entry(void);
void tm_preemptive_thread_3_entry(void);
void tm_preemptive_thread_4_entry(void);
void tm_preemptive_thread_report(void);
void tm_preemptive_scheduling_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_preemptive_scheduling_initialize);
}

void tm_preemptive_scheduling_initialize(void)
{
    TM_CHECK(tm_thread_create(0, 10, tm_preemptive_thread_0_entry));
    TM_CHECK(tm_thread_create(1, 9,  tm_preemptive_thread_1_entry));
    TM_CHECK(tm_thread_create(2, 8,  tm_preemptive_thread_2_entry));
    TM_CHECK(tm_thread_create(3, 7,  tm_preemptive_thread_3_entry));
    TM_CHECK(tm_thread_create(4, 6,  tm_preemptive_thread_4_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_thread_create(5, 2,  tm_preemptive_thread_report));
    TM_CHECK(tm_thread_resume(5));
}

void tm_preemptive_thread_0_entry(void)
{
    while (1) {
        tm_thread_resume(1);
        tm_preemptive_thread_0_counter++;
    }
}

void tm_preemptive_thread_1_entry(void)
{
    while (1) {
        tm_thread_resume(2);
        tm_preemptive_thread_1_counter++;
        tm_thread_suspend(1);
    }
}

void tm_preemptive_thread_2_entry(void)
{
    while (1) {
        tm_thread_resume(3);
        tm_preemptive_thread_2_counter++;
        tm_thread_suspend(2);
    }
}

void tm_preemptive_thread_3_entry(void)
{
    while (1) {
        tm_thread_resume(4);
        tm_preemptive_thread_3_counter++;
        tm_thread_suspend(3);
    }
}

void tm_preemptive_thread_4_entry(void)
{
    while (1) {
        tm_preemptive_thread_4_counter++;
        tm_thread_suspend(4);
    }
}

void tm_preemptive_thread_report(void)
{
    uint64_t      total;
    uint64_t      last_total    = 0;
    uint64_t      relative_time = 0;
    uint64_t      average;
    uint64_t      c0, c1, c2, c3, c4;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (uint64_t)tm_test_duration;

        /* Cast to unsigned long for %lu  wraps after ~49 days (fine for burn-in) */
        tm_printf("**** Thread-Metric Preemptive Scheduling Test **** "
                  "Relative Time: %lu\n",
                  (unsigned long)relative_time);

        c0 = tm_preemptive_thread_0_counter;
        c1 = tm_preemptive_thread_1_counter;
        c2 = tm_preemptive_thread_2_counter;
        c3 = tm_preemptive_thread_3_counter;
        c4 = tm_preemptive_thread_4_counter;

        total   = c0 + c1 + c2 + c3 + c4;
        average = total / 5u;

        if (average > 0u &&
            ((c0 < (average - 1u)) || (c0 > (average + 1u)) ||
             (c1 < (average - 1u)) || (c1 > (average + 1u)) ||
             (c2 < (average - 1u)) || (c2 > (average + 1u)) ||
             (c3 < (average - 1u)) || (c3 > (average + 1u)) ||
             (c4 < (average - 1u)) || (c4 > (average + 1u)))) {
            tm_printf("ERROR: Invalid counter value(s). "
                      "Preemptive counters should not be more than 1 "
                      "different than the average!\n");
        }

        /* Per-window delta always fits in 32 bits (~5.2M at nRF52832 scores) */
        tm_printf("Time Period Total:  %lu\n\n",
                  (unsigned long)(total - last_total));
        last_total = total;
    }
    TM_REPORT_FINISH;
}
