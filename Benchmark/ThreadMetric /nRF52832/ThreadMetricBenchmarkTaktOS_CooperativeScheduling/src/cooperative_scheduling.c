/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Modified for long-duration stability testing:
 *   - Overflow-safe determinism check: compare counters pairwise against c0
 *     instead of against (total / 5). The original check overflows uint32
 *     when total = c0+c1+c2+c3+c4 wraps at ~7620 seconds @ nRF52832 64MHz,
 *     producing a false ERROR. The pairwise check is mathematically equivalent
 *     and immune to the overflow because individual counters do not wrap until
 *     ~38000 seconds.
 *   - Per-period delta uses wrapping subtraction (already correct in original).
 */
#include "tm_api.h"

volatile unsigned long tm_cooperative_thread_0_counter;
volatile unsigned long tm_cooperative_thread_1_counter;
volatile unsigned long tm_cooperative_thread_2_counter;
volatile unsigned long tm_cooperative_thread_3_counter;
volatile unsigned long tm_cooperative_thread_4_counter;

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
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_0_counter++;
    }
}
void tm_cooperative_thread_1_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_1_counter++;
    }
}
void tm_cooperative_thread_2_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_2_counter++;
    }
}
void tm_cooperative_thread_3_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_3_counter++;
    }
}
void tm_cooperative_thread_4_entry(void)
{
    while (1) {
        tm_thread_relinquish();
        tm_cooperative_thread_4_counter++;
    }
}

void tm_cooperative_thread_report(void)
{
    unsigned long relative_time;
    unsigned long last_c0;
    unsigned long last_total;
    unsigned long c0, c1, c2, c3, c4, total, delta;

    last_c0    = 0;
    last_total = 0;
    relative_time = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time = relative_time + tm_test_duration;

        tm_printf(
            "**** Thread-Metric Cooperative Scheduling Test **** Relative Time: %lu\n",
            relative_time);

        c0 = tm_cooperative_thread_0_counter;
        c1 = tm_cooperative_thread_1_counter;
        c2 = tm_cooperative_thread_2_counter;
        c3 = tm_cooperative_thread_3_counter;
        c4 = tm_cooperative_thread_4_counter;

        /*
         * Overflow-safe determinism check.
         *
         * Original: total = c0+c1+c2+c3+c4; average = total/5;
         *           check each counter against average ± 1.
         * Problem:  total overflows uint32 after ~254 periods (~7620 s at
         *           this throughput), corrupting average and triggering a
         *           false ERROR on every subsequent period.
         *
         * Fix: compare each counter directly against c0 (the reference).
         *      A deterministic round-robin scheduler guarantees all five
         *      counters stay within 1 of each other, which is equivalent
         *      to the original average ± 1 check but uses only individual
         *      counter values that each take ~38000 s to overflow.
         *
         * The per-period throughput uses (total - last_total) which is
         * correct under wrapping arithmetic regardless of overflow.
         */
        total = c0 + c1 + c2 + c3 + c4;
        delta = total - last_total;   /* wrapping subtraction — correct */

        if (c0 > 0 &&
            ((c1 < c0 - 1) || (c1 > c0 + 1) ||
             (c2 < c0 - 1) || (c2 > c0 + 1) ||
             (c3 < c0 - 1) || (c3 > c0 + 1) ||
             (c4 < c0 - 1) || (c4 > c0 + 1))) {
            tm_printf(
                "ERROR: Invalid counter value(s). Cooperative counters should not be more than 1 different than the average!\n");
        }

        tm_printf("Time Period Total:  %lu\n\n", delta);
        last_total = total;
        (void)last_c0;   /* suppress unused warning if needed */
    }

    TM_REPORT_FINISH;
}
