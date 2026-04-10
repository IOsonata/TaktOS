/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Modified: counter widened to uint64_t for burn-in testing.
 * Original unsigned long overflows at ~1.7 hours at nRF52832 64 MHz scores.
 * Printing uses %lu only — per-window delta always fits in 32 bits.
 */

/* Thread-Metric Component -- Synchronization Processing Test */
#include <stdint.h>
#include "tm_api.h"

volatile uint64_t tm_synchronization_processing_counter;

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
    uint64_t      last_counter  = 0;
    unsigned long relative_time = 0;
    uint64_t      current;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (unsigned long)tm_test_duration;

        tm_printf("**** Thread-Metric Synchronization Processing Test **** "
                  "Relative Time: %lu\n", relative_time);

        current = tm_synchronization_processing_counter;

        if (current == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). "
                      "Error getting/putting semaphore!\n");
        }

        /* Per-window delta always fits in 32 bits (~21M at nRF52832) */
        tm_printf("Time Period Total:  %lu\n\n",
                  (unsigned long)(current - last_counter));
        last_counter = current;
    }
    TM_REPORT_FINISH;
}
