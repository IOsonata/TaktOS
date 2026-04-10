/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
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
    unsigned long total;
    unsigned long relative_time;
    unsigned long last_total;
    unsigned long average;
    unsigned long c0, c1, c2, c3, c4;

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

        total = c0 + c1 + c2 + c3 + c4;
        average = total / 5;

        if (average > 0 && ((c0 < (average - 1)) || (c0 > (average + 1)) ||
                            (c1 < (average - 1)) || (c1 > (average + 1)) ||
                            (c2 < (average - 1)) || (c2 > (average + 1)) ||
                            (c3 < (average - 1)) || (c3 > (average + 1)) ||
                            (c4 < (average - 1)) || (c4 > (average + 1)))) {
            tm_printf(
                "ERROR: Invalid counter value(s). Cooperative counters should not be more than 1 different than the average!\n");
        }

        tm_printf("Time Period Total:  %lu\n\n", total - last_total);
        last_total = total;
    }

    TM_REPORT_FINISH;
}
