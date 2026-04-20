/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

/* Thread-Metric Component -- Memory Allocation Test */
#include "tm_api.h"

volatile unsigned long tm_memory_allocation_counter;

void tm_memory_allocation_thread_0_entry(void);
void tm_memory_allocation_thread_report(void);
void tm_memory_allocation_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_memory_allocation_initialize);
}

void tm_memory_allocation_initialize(void)
{
    TM_CHECK(tm_thread_create(0, 10, tm_memory_allocation_thread_0_entry));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_memory_pool_create(0));
    TM_CHECK(tm_thread_create(5, 2, tm_memory_allocation_thread_report));
    TM_CHECK(tm_thread_resume(5));
}

void tm_memory_allocation_thread_0_entry(void)
{
    int status;
    unsigned char *memory_ptr;

    while (1) {
        status = tm_memory_pool_allocate(0, &memory_ptr);
        if (status != TM_SUCCESS) break;
        status = tm_memory_pool_deallocate(0, memory_ptr);
        if (status != TM_SUCCESS) break;
        tm_memory_allocation_counter++;
    }
}

void tm_memory_allocation_thread_report(void)
{
    unsigned long last_counter = 0;
    unsigned long relative_time = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += tm_test_duration;
        tm_printf("**** Thread-Metric Memory Allocation Test **** Relative Time: %lu\n", relative_time);
        if (tm_memory_allocation_counter == last_counter) {
            tm_printf("ERROR: Invalid counter value(s). Error allocating/deallocating memory!\n");
        }
        tm_printf("Time Period Total:  %lu\n\n", tm_memory_allocation_counter - last_counter);
        last_counter = tm_memory_allocation_counter;
    }
    TM_REPORT_FINISH;
}
