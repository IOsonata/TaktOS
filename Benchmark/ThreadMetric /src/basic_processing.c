/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Modified: counter widened to uint64_t for burn-in testing.
 * Original unsigned long overflows at ~287 hours (12 days) at nRF52832
 * 64 MHz scores (~124,658 per window).
 * Printing uses %lu only — per-window delta always fits in 32 bits.
 */

/* Thread-Metric Component -- Basic Processing Test */
#include <stdint.h>
#include "tm_api.h"

volatile uint64_t      tm_basic_processing_counter;
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
    int           i;
    unsigned long counter_snapshot;

    for (i = 0; i < 1024; i++) {
        tm_basic_processing_array[i] = 0;
    }

    while (1) {
        /* counter_snapshot is used as a 32-bit workload value — low 32 bits
         * of the counter are sufficient; the array computation is unchanged. */
        counter_snapshot = (unsigned long)tm_basic_processing_counter;
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
    uint64_t      last_counter  = 0;
    unsigned long relative_time = 0;
    uint64_t      current;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (unsigned long)tm_test_duration;

        tm_printf("**** Thread-Metric Basic Single Thread Processing Test **** "
                  "Relative Time: %lu\n", relative_time);

        current = tm_basic_processing_counter;

        if (current == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). "
                      "Basic processing thread died!\n");
        }

        /* Per-window delta always fits in 32 bits (~124K at nRF52832) */
        tm_printf("Time Period Total:  %lu\n\n",
                  (unsigned long)(current - last_counter));
        last_counter = current;
    }
    TM_REPORT_FINISH;
}
