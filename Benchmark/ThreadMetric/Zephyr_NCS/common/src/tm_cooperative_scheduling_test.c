/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/**   Cooperative Scheduling Test                                         */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    tm_cooperative_scheduling_test                      PORTABLE C      */
/*                                                           6.1.7        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the cooperative scheduling test.                  */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  10-15-2021     William E. Lamie         Initial Version 6.1.7         */
/*                                                                        */
/**************************************************************************/
#include "tm_api.h"

/* Define the counters used in the demo application...  */

unsigned long tm_cooperative_thread_0_counter;
unsigned long tm_cooperative_thread_1_counter;
unsigned long tm_cooperative_thread_2_counter;
unsigned long tm_cooperative_thread_3_counter;
unsigned long tm_cooperative_thread_4_counter;

/* Define the test thread prototypes.  */

void tm_cooperative_thread_0_entry(void *p1, void *p2, void *p3);
void tm_cooperative_thread_1_entry(void *p1, void *p2, void *p3);
void tm_cooperative_thread_2_entry(void *p1, void *p2, void *p3);
void tm_cooperative_thread_3_entry(void *p1, void *p2, void *p3);
void tm_cooperative_thread_4_entry(void *p1, void *p2, void *p3);

/* Define the reporting function prototype.  */

void tm_cooperative_thread_report(void);

/* Define the initialization prototype.  */

void tm_cooperative_scheduling_initialize(void);

/* Define main entry point.  */

int main(void)
{

	/* Initialize the test.  */
	tm_initialize(tm_cooperative_scheduling_initialize);

	return 0;
}

/* Define the cooperative scheduling test initialization.  */

void tm_cooperative_scheduling_initialize(void)
{
	int prio = CONFIG_MAIN_THREAD_PRIORITY;

	/* Create all 5 threads at the same priority as the main thread.  */
	tm_thread_create(0, prio, tm_cooperative_thread_0_entry);
	tm_thread_create(1, prio, tm_cooperative_thread_1_entry);
	tm_thread_create(2, prio, tm_cooperative_thread_2_entry);
	tm_thread_create(3, prio, tm_cooperative_thread_3_entry);
	tm_thread_create(4, prio, tm_cooperative_thread_4_entry);

	/* Resume all 5 threads.  */
	tm_thread_resume(0);
	tm_thread_resume(1);
	tm_thread_resume(2);
	tm_thread_resume(3);
	tm_thread_resume(4);

	tm_cooperative_thread_report();
}

/* Define the first cooperative thread.  */
void tm_cooperative_thread_0_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Relinquish to all other threads at same priority.  */
		tm_thread_relinquish();

		/* Increment this thread's counter.  */
		tm_cooperative_thread_0_counter++;
	}
}

/* Define the second cooperative thread.  */
void tm_cooperative_thread_1_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Relinquish to all other threads at same priority.  */
		tm_thread_relinquish();

		/* Increment this thread's counter.  */
		tm_cooperative_thread_1_counter++;
	}
}

/* Define the third cooperative thread.  */
void tm_cooperative_thread_2_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Relinquish to all other threads at same priority.  */
		tm_thread_relinquish();

		/* Increment this thread's counter.  */
		tm_cooperative_thread_2_counter++;
	}
}

/* Define the fourth cooperative thread.  */
void tm_cooperative_thread_3_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Relinquish to all other threads at same priority.  */
		tm_thread_relinquish();

		/* Increment this thread's counter.  */
		tm_cooperative_thread_3_counter++;
	}
}

/* Define the fifth cooperative thread.  */
void tm_cooperative_thread_4_entry(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	while (1) {

		/* Relinquish to all other threads at same priority.  */
		tm_thread_relinquish();

		/* Increment this thread's counter.  */
		tm_cooperative_thread_4_counter++;
	}
}

/* Define the cooperative test reporting function.  */
void tm_cooperative_thread_report(void)
{
    unsigned long total;
    unsigned long relative_time = 0;
    unsigned long last_total = 0;

    while (1) {
        unsigned long c0, c1, c2, c3, c4;
        unsigned long minc, maxc;

        tm_thread_sleep(TM_TEST_DURATION);
        relative_time += TM_TEST_DURATION;

        /* Snapshot once. */
        c0 = tm_cooperative_thread_0_counter;
        c1 = tm_cooperative_thread_1_counter;
        c2 = tm_cooperative_thread_2_counter;
        c3 = tm_cooperative_thread_3_counter;
        c4 = tm_cooperative_thread_4_counter;

        total = c0 + c1 + c2 + c3 + c4;

        minc = c0;
        maxc = c0;
        if (c1 < minc) minc = c1; if (c1 > maxc) maxc = c1;
        if (c2 < minc) minc = c2; if (c2 > maxc) maxc = c2;
        if (c3 < minc) minc = c3; if (c3 > maxc) maxc = c3;
        if (c4 < minc) minc = c4; if (c4 > maxc) maxc = c4;

        printf("**** Thread-Metric Cooperative Scheduling Test **** Relative Time: %lu\n",
               relative_time);

        printf("tm_cooperative_thread_0_counter: %lu\n", c0);
        printf("tm_cooperative_thread_1_counter: %lu\n", c1);
        printf("tm_cooperative_thread_2_counter: %lu\n", c2);
        printf("tm_cooperative_thread_3_counter: %lu\n", c3);
        printf("tm_cooperative_thread_4_counter: %lu\n", c4);

        if ((maxc - minc) > 1) {
            printf("ERROR: Invalid counter value(s). Cooperative counters should not "
                   "be more than 1 different from each other!\n");
        }

        printf("Time Period Total:  %lu\n\n", total - last_total);
        last_total = total;
    }
}