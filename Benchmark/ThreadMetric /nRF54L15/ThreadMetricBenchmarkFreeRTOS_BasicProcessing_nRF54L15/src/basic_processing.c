/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

/* Thread-Metric Component -- Basic Processing Test */
#include "tm_api.h"

volatile unsigned long tm_basic_processing_counter;
volatile unsigned long tm_basic_processing_array[1024];

void tm_basic_processing_thread_0_entry(void);
void tm_basic_processing_thread_report(void);
void tm_basic_processing_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_basic_processing_initialize);
}

void tm_basic_processing_initialize(void)
{
    TM_CHECK(tm_thread_create(0, 10, tm_basic_processing_thread_0_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_thread_create(5, 2, tm_basic_processing_thread_report));
    TM_CHECK(tm_thread_resume(5));
}

void tm_basic_processing_thread_0_entry(void)
{
    int i;
    unsigned long counter_snapshot;

    for (i = 0; i < 1024; i++) {
        tm_basic_processing_array[i] = 0;
    }

    while (1) {
        counter_snapshot = tm_basic_processing_counter;
        for (i = 0; i < 1024; i++) {
            tm_basic_processing_array[i] =
                (tm_basic_processing_array[i] + counter_snapshot) ^
                tm_basic_processing_array[i];
        }
        tm_basic_processing_counter++;
    }
}

void tm_basic_processing_thread_report(void)
{
    unsigned long last_counter = 0;
    unsigned long relative_time = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += tm_test_duration;
        tm_printf("**** Thread-Metric Basic Single Thread Processing Test **** Relative Time: %lu\n", relative_time);
        if (tm_basic_processing_counter == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). Basic processing thread died!\n");
        }
        tm_printf("Time Period Total:  %lu\n\n", tm_basic_processing_counter - last_counter);
        last_counter = tm_basic_processing_counter;
    }
    TM_REPORT_FINISH;
}
