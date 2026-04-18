/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Mutex Fairness Test
 * -------------------
 * Measures wait-list fairness and ownership-handoff fairness for a single
 * mutex shared among equal-priority threads.
 *
 * Design
 * ------
 *   4 worker threads, all at TM priority 10.
 *   1 reporter thread at TM priority 2.
 *   No interloper.  All threads are at the same priority so the scheduler
 *   cannot favour any one thread by priority.  Any imbalance in the per-thread
 *   counts reflects a fairness defect in the mutex or scheduler implementation.
 *
 * Worker loop
 * -----------
 *   lock(blocking)  shared++  per_thread[i]++  unlock  yield  repeat
 *
 *   tm_thread_relinquish() after each unlock is essential on a single core:
 *   without it, the unlocking thread immediately re-locks in the same time
 *   slice, the wait list stays empty, and there is no contention to measure.
 *   With the yield, the unlocker surrenders the CPU so other threads can
 *   attempt to lock and queue up.  At steady state, 3 threads are always
 *   blocked in the mutex wait list and 1 is running  the wait list is
 *   exercised on every cycle.
 *
 * What is measured
 * ----------------
 *    Wait-list fairness: equal-priority waiters should be served in FIFO
 *     order; each worker should receive ~25% of total grants.
 *    Ownership-handoff correctness: the shared counter must equal the sum
 *     of all per-thread counters (no lost updates).
 *    Fairness ratio: min_thread_delta / max_thread_delta  a perfect
 *     implementation gives 1.000; any deviation reveals scheduler or
 *     wait-list bias.
 *
 * Separate benchmark
 * ------------------
 *   The interloper (higher-priority mixed-contention) test lives in
 *   mutex_barging_test.c.  That test measures priority-ordering correctness,
 *   not fairness among equal contenders.
 */

#include <stdint.h>
#include "tm_api.h"

#define TM_MUTEX_NUM_WORKERS  4   /* threads 03, all TM priority 10 */
#define TM_MUTEX_REPORTER_ID  4   /* thread 4, TM priority 2          */

volatile uint64_t tm_mutex_shared;
volatile uint64_t tm_mutex_per_thread[TM_MUTEX_NUM_WORKERS];

void tm_mutex_worker_0(void);
void tm_mutex_worker_1(void);
void tm_mutex_worker_2(void);
void tm_mutex_worker_3(void);
void tm_mutex_reporter(void);
void tm_mutex_fairness_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_mutex_fairness_initialize);
}

void tm_mutex_fairness_initialize(void)
{
    TM_CHECK(tm_mutex_create(0));

    TM_CHECK(tm_thread_create(0, 10, tm_mutex_worker_0));
    TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_thread_create(1, 10, tm_mutex_worker_1));
    TM_CHECK(tm_thread_resume(1));
    TM_CHECK(tm_thread_create(2, 10, tm_mutex_worker_2));
    TM_CHECK(tm_thread_resume(2));
    TM_CHECK(tm_thread_create(3, 10, tm_mutex_worker_3));
    TM_CHECK(tm_thread_resume(3));

    /* Reporter at the same TM priority as workers (10), not above them.
     * At TM priority 2 the reporter preempts workers when it wakes, creating
     * a brief measurement disturbance.  At TM priority 10 it joins the
     * round-robin and gets its natural yield slot  no preemption, no
     * disturbance.  Truly lowest priority (TM 31) risks starvation on
     * strict-priority schedulers where workers always occupy the ready queue. */
    TM_CHECK(tm_thread_create(TM_MUTEX_REPORTER_ID, 10, tm_mutex_reporter));
    TM_CHECK(tm_thread_resume(TM_MUTEX_REPORTER_ID));
}

/*  Worker body  */
#define WORKER_BODY(idx)                                  \
    do {                                                  \
        int status;                                       \
        while (1) {                                       \
            status = tm_mutex_lock(0);                    \
            if (status != TM_SUCCESS) break;              \
            tm_mutex_shared++;                            \
            tm_mutex_per_thread[(idx)]++;                 \
            status = tm_mutex_unlock(0);                  \
            if (status != TM_SUCCESS) break;              \
            tm_thread_relinquish();                       \
        }                                                 \
    } while (0)

void tm_mutex_worker_0(void) { WORKER_BODY(0); }
void tm_mutex_worker_1(void) { WORKER_BODY(1); }
void tm_mutex_worker_2(void) { WORKER_BODY(2); }
void tm_mutex_worker_3(void) { WORKER_BODY(3); }

/*  Reporter  */
void tm_mutex_reporter(void)
{
    uint64_t      last_shared = 0;
    uint64_t      last_thread[TM_MUTEX_NUM_WORKERS];
    unsigned long relative_time = 0;
    int           i;

    for (i = 0; i < TM_MUTEX_NUM_WORKERS; i++)
        last_thread[i] = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (unsigned long)tm_test_duration;

        tm_printf("**** Thread-Metric Mutex Fairness Test **** "
                  "Relative Time: %lu\n", relative_time);

        uint64_t cur_shared = tm_mutex_shared;
        uint64_t cur_thread[TM_MUTEX_NUM_WORKERS];
        for (i = 0; i < TM_MUTEX_NUM_WORKERS; i++)
            cur_thread[i] = tm_mutex_per_thread[i];

        /* Correctness: per-thread sum must equal shared delta */
        uint64_t sum_delta = 0;
        uint64_t min_delta = (uint64_t)-1;
        uint64_t max_delta = 0;
        for (i = 0; i < TM_MUTEX_NUM_WORKERS; i++) {
            uint64_t d = cur_thread[i] - last_thread[i];
            sum_delta += d;
            if (d < min_delta) min_delta = d;
            if (d > max_delta) max_delta = d;
        }
        uint64_t shared_delta = cur_shared - last_shared;

        if (shared_delta == 0) {
            tm_printf("ERROR: shared counter stalled!\n");
        } else if (shared_delta != sum_delta) {
            tm_printf("ERROR: lost update! shared=%lu sum_threads=%lu\n",
                      (unsigned long)shared_delta, (unsigned long)sum_delta);
        }

        /* Fairness ratio: 1.000 = perfect, lower = more skewed.
         * Computed as (min * 1000) / max to stay in integer arithmetic.
         * Digits printed individually because tm_printf does not support
         * width/padding specifiers such as %03lu. */
        unsigned long ratio_num = (unsigned long)(min_delta * 1000u);
        unsigned long ratio_den = (max_delta > 0) ? (unsigned long)max_delta : 1u;
        unsigned long fairness  = ratio_num / ratio_den;   /* e.g. 998 means 0.998 */
        unsigned long frac      = fairness % 1000u;

        tm_printf("Total ops this window: %lu\n", (unsigned long)shared_delta);
        for (i = 0; i < TM_MUTEX_NUM_WORKERS; i++)
            tm_printf("  Worker %d: %lu\n", i,
                      (unsigned long)(cur_thread[i] - last_thread[i]));
        tm_printf("  Min / Max: %lu / %lu\n",
                  (unsigned long)min_delta, (unsigned long)max_delta);
        tm_printf("  Fairness ratio: %lu.%lu%lu%lu\n\n",
                  fairness / 1000u,
                  frac / 100u,
                  (frac / 10u) % 10u,
                  frac % 10u);

        last_shared = cur_shared;
        for (i = 0; i < TM_MUTEX_NUM_WORKERS; i++)
            last_thread[i] = cur_thread[i];
    }
    TM_REPORT_FINISH;
}
