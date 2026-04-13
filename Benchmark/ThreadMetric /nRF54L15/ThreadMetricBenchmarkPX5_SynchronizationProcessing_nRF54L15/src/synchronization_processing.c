/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

/* Thread-Metric Component -- Synchronization Processing Test */
#include "tm_api.h"

volatile unsigned long tm_synchronization_processing_counter;

void tm_synchronization_processing_thread_0_entry(void);
void tm_synchronization_processing_thread_report(void);
void tm_synchronization_processing_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_synchronization_processing_initialize);
}

void tm_synchronization_processing_initialize(void)
{
    TM_CHECK(tm_thread_create(0, 10, tm_synchronization_processing_thread_0_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_semaphore_create(0));
    TM_CHECK(tm_thread_create(5, 2, tm_synchronization_processing_thread_report));
    TM_CHECK(tm_thread_resume(5));
}

void tm_synchronization_processing_thread_0_entry(void)
{
    int status;
    while (1) {
        status = tm_semaphore_get(0);
        if (status != TM_SUCCESS) break;
        status = tm_semaphore_put(0);
        if (status != TM_SUCCESS) break;
        tm_synchronization_processing_counter++;
    }
}

void tm_synchronization_processing_thread_report(void)
{
    unsigned long last_counter = 0;
    unsigned long relative_time = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += tm_test_duration;
        tm_printf("**** Thread-Metric Synchronization Processing Test **** Relative Time: %lu\n", relative_time);
        if (tm_synchronization_processing_counter == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). Error getting/putting semaphore!\n");
        }
        tm_printf("Time Period Total:  %lu\n\n", tm_synchronization_processing_counter - last_counter);
        last_counter = tm_synchronization_processing_counter;
    }
    TM_REPORT_FINISH;
}
