/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Modified: counter widened to uint64_t for burn-in testing.
 * Original unsigned long overflows at ~4.0 hours at nRF52832 64 MHz scores.
 * Printing uses %lu only  per-window delta always fits in 32 bits.
 */

/* Thread-Metric Component -- Message Processing Test */
#include <stdint.h>
#include "tm_api.h"

volatile uint64_t tm_message_processing_counter;
unsigned long     tm_message_sent[4];
unsigned long     tm_message_received[4];

void tm_message_processing_thread_0_entry(void);
void tm_message_processing_thread_report(void);
void tm_message_processing_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_message_processing_initialize);
}

void tm_message_processing_initialize(void)
{
    TM_CHECK(tm_thread_create(0, 10, tm_message_processing_thread_0_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_queue_create(0));
    TM_CHECK(tm_thread_create(5, 2, tm_message_processing_thread_report));
    TM_CHECK(tm_thread_resume(5));
}

void tm_message_processing_thread_0_entry(void)
{
    tm_message_sent[0] = 0x11112222;
    tm_message_sent[1] = 0x33334444;
    tm_message_sent[2] = 0x55556666;
    tm_message_sent[3] = 0x77778888;

    while (1) {
        if (tm_queue_send(0, tm_message_sent) != TM_SUCCESS) break;
        if (tm_queue_receive(0, tm_message_received) != TM_SUCCESS) break;
        if (tm_message_received[3] != tm_message_sent[3]) break;
        tm_message_sent[3]++;
        tm_message_processing_counter++;
    }
}

void tm_message_processing_thread_report(void)
{
    uint64_t      last_counter  = 0;
    unsigned long relative_time = 0;
    uint64_t      current;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (unsigned long)tm_test_duration;

        tm_printf("**** Thread-Metric Message Processing Test **** "
                  "Relative Time: %lu\n", relative_time);

        current = tm_message_processing_counter;

        if (current == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). "
                      "Error sending/receiving messages!\n");
        }

        /* Per-window delta always fits in 32 bits (~9M at nRF52832) */
        tm_printf("Time Period Total:  %lu\n\n",
                  (unsigned long)(current - last_counter));
        last_counter = current;
    }
    TM_REPORT_FINISH;
}
