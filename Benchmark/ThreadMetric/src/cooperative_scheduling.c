/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Modified: counter variables widened to uint64_t for burn-in testing.
 * Original unsigned long counters overflow at ~10.9 hours (individual) /
 * ~2.2 hours (total) at nRF52832 64 MHz scores, producing false ERROR.
 * Printing uses %lu only  per-window delta always fits in 32 bits.
 */

/* Thread-Metric Component -- Cooperative Scheduling Test */
#include <stdint.h>
#include "tm_api.h"

volatile uint64_t tm_cooperative_thread_0_counter;
volatile uint64_t tm_cooperative_thread_1_counter;
volatile uint64_t tm_cooperative_thread_2_counter;
volatile uint64_t tm_cooperative_thread_3_counter;
volatile uint64_t tm_cooperative_thread_4_counter;

void tm_cooperative_thread_0_entry(void);
void tm_cooperative_thread_1_entry(void);
void tm_cooperative_thread_2_entry(void);
void tm_cooperative_thread_3_entry(void);
void tm_cooperative_thread_4_entry(void);
void tm_cooperative_thread_report(void);
void tm_cooperative_scheduling_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_cooperative_scheduling_initialize);
}

void tm_cooperative_scheduling_initialize(void)
{
    TM_CHECK(tm_thread_create(0, 3, tm_cooperative_thread_0_entry));
    TM_CHECK(tm_thread_create(1, 3, tm_cooperative_thread_1_entry));
    TM_CHECK(tm_thread_create(2, 3, tm_cooperative_thread_2_entry));
    TM_CHECK(tm_thread_create(3, 3, tm_cooperative_thread_3_entry));
    TM_CHECK(tm_thread_create(4, 3, tm_cooperative_thread_4_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_thread_resume(1));
    TM_CHECK(tm_thread_resume(2));
    TM_CHECK(tm_thread_resume(3));
    TM_CHECK(tm_thread_resume(4));
    TM_CHECK(tm_thread_create(5, 2, tm_cooperative_thread_report));
    TM_CHECK(tm_thread_resume(5));
}

void tm_cooperative_thread_0_entry(void)
{
    while (1) { tm_thread_relinquish(); tm_cooperative_thread_0_counter++; }
}
void tm_cooperative_thread_1_entry(void)
{
    while (1) { tm_thread_relinquish(); tm_cooperative_thread_1_counter++; }
}
void tm_cooperative_thread_2_entry(void)
{
    while (1) { tm_thread_relinquish(); tm_cooperative_thread_2_counter++; }
}
void tm_cooperative_thread_3_entry(void)
{
    while (1) { tm_thread_relinquish(); tm_cooperative_thread_3_counter++; }
}
void tm_cooperative_thread_4_entry(void)
{
    while (1) { tm_thread_relinquish(); tm_cooperative_thread_4_counter++; }
}

void tm_cooperative_thread_report(void)
{
    uint64_t      c0, c1, c2, c3, c4, total, average;
    uint64_t      last_total    = 0;
    unsigned long relative_time = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (unsigned long)tm_test_duration;

        tm_printf("**** Thread-Metric Cooperative Scheduling Test **** "
                  "Relative Time: %lu\n", relative_time);

        c0 = tm_cooperative_thread_0_counter;
        c1 = tm_cooperative_thread_1_counter;
        c2 = tm_cooperative_thread_2_counter;
        c3 = tm_cooperative_thread_3_counter;
        c4 = tm_cooperative_thread_4_counter;

        total   = c0 + c1 + c2 + c3 + c4;
        average = total / 5u;

        if (average > 0u &&
            ((c0 < (average - 1u)) || (c0 > (average + 1u)) ||
             (c1 < (average - 1u)) || (c1 > (average + 1u)) ||
             (c2 < (average - 1u)) || (c2 > (average + 1u)) ||
             (c3 < (average - 1u)) || (c3 > (average + 1u)) ||
             (c4 < (average - 1u)) || (c4 > (average + 1u)))) {
            tm_printf("ERROR: Invalid counter value(s). Cooperative counters "
                      "should not be more than 1 different than the average!\n");
        }

        /* Per-window delta always fits in 32 bits (~16.3M at nRF52832) */
        tm_printf("Time Period Total:  %lu\n\n",
                  (unsigned long)(total - last_total));
        last_total = total;
    }
    TM_REPORT_FINISH;
}
