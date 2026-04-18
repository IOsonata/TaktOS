/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

/* Thread-Metric Component -- Preemptive Scheduling Test */
#include "tm_api.h"

volatile unsigned long tm_preemptive_thread_0_counter;
volatile unsigned long tm_preemptive_thread_1_counter;
volatile unsigned long tm_preemptive_thread_2_counter;
volatile unsigned long tm_preemptive_thread_3_counter;
volatile unsigned long tm_preemptive_thread_4_counter;

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
    TM_CHECK(tm_thread_create(1, 9, tm_preemptive_thread_1_entry));
    TM_CHECK(tm_thread_create(2, 8, tm_preemptive_thread_2_entry));
    TM_CHECK(tm_thread_create(3, 7, tm_preemptive_thread_3_entry));
    TM_CHECK(tm_thread_create(4, 6, tm_preemptive_thread_4_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_thread_create(5, 2, tm_preemptive_thread_report));
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
    unsigned long total;
    unsigned long relative_time = 0;
    unsigned long last_total = 0;
    unsigned long average;
    unsigned long c0, c1, c2, c3, c4;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += tm_test_duration;
        tm_printf("**** Thread-Metric Preemptive Scheduling Test **** Relative Time: %lu\n", relative_time);
        c0 = tm_preemptive_thread_0_counter;
        c1 = tm_preemptive_thread_1_counter;
        c2 = tm_preemptive_thread_2_counter;
        c3 = tm_preemptive_thread_3_counter;
        c4 = tm_preemptive_thread_4_counter;
        total = c0 + c1 + c2 + c3 + c4;
        average = total / 5;
        if (average > 0 && ((c0 < (average - 1)) || (c0 > (average + 1)) ||
                            (c1 < (average - 1)) || (c1 > (average + 1)) ||
                            (c2 < (average - 1)) || (c2 > (average + 1)) ||
                            (c3 < (average - 1)) || (c3 > (average + 1)) ||
                            (c4 < (average - 1)) || (c4 > (average + 1)))) {
            tm_printf("ERROR: Invalid counter value(s). Preemptive counters should not be more than 1 different than the average!\n");
        }
        tm_printf("Time Period Total:  %lu\n\n", total - last_total);
        last_total = total;
    }
    TM_REPORT_FINISH;
}
