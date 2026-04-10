/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#include "tm_api.h"

volatile unsigned long tm_message_processing_counter;
unsigned long tm_message_sent[4];
unsigned long tm_message_received[4];

void tm_message_processing_thread_0_entry(void);
void tm_message_processing_thread_report(void);
void tm_message_processing_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_message_processing_initialize);
}

void tm_message_processing_initialize(void)
{
    TM_CHECK(tm_queue_create(0));
    TM_CHECK(tm_thread_create(0, 10, tm_message_processing_thread_0_entry));
    TM_CHECK(tm_thread_resume(0));
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
        if (tm_queue_send(0, tm_message_sent) != TM_SUCCESS)
            break;
        if (tm_queue_receive(0, tm_message_received) != TM_SUCCESS)
            break;
        if (tm_message_received[3] != tm_message_sent[3])
            break;
        tm_message_sent[3]++;
        tm_message_processing_counter++;
    }
}

void tm_message_processing_thread_report(void)
{
    unsigned long last_counter = 0;
    unsigned long relative_time = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += tm_test_duration;
        tm_printf("**** Thread-Metric Message Processing Test **** Relative Time: %lu\n", relative_time);
        if (tm_message_processing_counter == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). Error sending/receiving messages!\n");
        }
        tm_printf("Time Period Total:  %lu\n\n", tm_message_processing_counter - last_counter);
        last_counter = tm_message_processing_counter;
    }
    TM_REPORT_FINISH;
}
