/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 *
 * Mutex Priority / Barging Test
 * ------------------------------
 * Measures priority-ordered wakeup correctness and the overhead of
 * high-priority insertions into the mutex wait list under sustained
 * mixed-priority contention.
 *
 * Design
 * ------
 *   4 worker threads  at TM priority 10   (IDs 03)
 *   4 interloper threads at TM priority 3  (IDs 47)
 *   1 reporter thread at TM priority 10   (ID 8)
 *
 * Why 4 interlopers, not 1
 * ------------------------
 * With 1-tick (1 ms) sleep per interloper cycle, each interloper generates
 * ~1000 barge events per second.  Workers running at ~200 K grants/s combined
 * would reduce a single interloper to ~0.5% of total grants  perceptible but
 * still light pressure on the barging path.  Using 4 interlopers raises the
 * barge event rate to ~4000/s (~120 K per 30-s window) and exercises the
 * priority-ordered wait-list insertion on roughly 12% of all grants.
 *
 * Practical ceiling on barge fraction
 * ------------------------------------
 * Reaching >10% barge share within a tick-based framework would require
 * either sub-tick busy-wait (platform-specific, ugly) or synthetic delays
 * inside the critical section to slow workers down.  At 1 ms/interloper
 * sleep and 4 interlopers, ~120 K priority insertions per 30-s window is a
 * 4000 increase over the earlier 1-second design and puts real pressure on
 * TaktWaitListInsert (priority-ordered), TaktWaitListPop, and the unlock
 * slow-path preemption decision  without starving the workers or requiring
 * artificial busy-loops.
 *
 * Worker loop
 * -----------
 *   lock(blocking)  shared++  per_thread[i]++  unlock  yield  repeat
 *
 * Interloper loop
 * ---------------
 *   lock(blocking)  shared++  per_thread[4+i]++  unlock  yield 
 *   sleep(1 tick)  repeat
 *
 *   The 1-tick sleep prevents starvation while enabling ~1000 barge events/s
 *   per interloper.  When an interloper wakes, it is inserted at the HEAD of
 *   the mutex wait list (priority 3 > priority 10), so it always receives the
 *   mutex before any worker  exercising the priority-ordered path on every
 *   interloper cycle.
 *
 * Correctness invariant
 * ---------------------
 *   tm_barge_shared == sum(tm_barge_per_thread[0..7])   at every report window
 *
 * Expected output
 * ---------------
 *   Workers:     ~24% each of worker-subtotal (fair share among 4 peers)
 *   Interlopers: ~1% each of total grants, but always served BEFORE workers
 *                when they compete  interloper share comes from the barging
 *                path, not from the equal-priority round-robin.
 */

#include <stdint.h>
#include "tm_api.h"

#define TM_BARGE_NUM_WORKERS      4   /* threads 03, TM priority 10 */
#define TM_BARGE_NUM_INTERLOPERS  4   /* threads 47, TM priority  3 */
#define TM_BARGE_REPORTER_ID      8   /* thread  8,   TM priority 10 */
#define TM_BARGE_TOTAL_CONTENDERS (TM_BARGE_NUM_WORKERS + TM_BARGE_NUM_INTERLOPERS)

volatile uint64_t tm_barge_shared;
volatile uint64_t tm_barge_per_thread[TM_BARGE_TOTAL_CONTENDERS];

void tm_barge_worker_0(void);
void tm_barge_worker_1(void);
void tm_barge_worker_2(void);
void tm_barge_worker_3(void);
void tm_barge_interloper_0(void);
void tm_barge_interloper_1(void);
void tm_barge_interloper_2(void);
void tm_barge_interloper_3(void);
void tm_barge_reporter(void);
void tm_mutex_barging_initialize(void);

void tm_main(void)
{
    tm_initialize(tm_mutex_barging_initialize);
}

void tm_mutex_barging_initialize(void)
{
    TM_CHECK(tm_mutex_create(0));

    /* Workers */
    TM_CHECK(tm_thread_create(0, 10, tm_barge_worker_0)); TM_CHECK(tm_thread_resume(0));
    TM_CHECK(tm_thread_create(1, 10, tm_barge_worker_1)); TM_CHECK(tm_thread_resume(1));
    TM_CHECK(tm_thread_create(2, 10, tm_barge_worker_2)); TM_CHECK(tm_thread_resume(2));
    TM_CHECK(tm_thread_create(3, 10, tm_barge_worker_3)); TM_CHECK(tm_thread_resume(3));

    /* Interlopers: higher priority, 1-tick sleep between cycles */
    TM_CHECK(tm_thread_create(4, 3, tm_barge_interloper_0)); TM_CHECK(tm_thread_resume(4));
    TM_CHECK(tm_thread_create(5, 3, tm_barge_interloper_1)); TM_CHECK(tm_thread_resume(5));
    TM_CHECK(tm_thread_create(6, 3, tm_barge_interloper_2)); TM_CHECK(tm_thread_resume(6));
    TM_CHECK(tm_thread_create(7, 3, tm_barge_interloper_3)); TM_CHECK(tm_thread_resume(7));

    /* Reporter: same priority as workers  joins round-robin, no preemption */
    TM_CHECK(tm_thread_create(TM_BARGE_REPORTER_ID, 10, tm_barge_reporter));
    TM_CHECK(tm_thread_resume(TM_BARGE_REPORTER_ID));
}

/*  Worker body  */
#define WORKER_BODY(idx)                                  \
    do {                                                  \
        int status;                                       \
        while (1) {                                       \
            status = tm_mutex_lock(0);                    \
            if (status != TM_SUCCESS) break;              \
            tm_barge_shared++;                            \
            tm_barge_per_thread[(idx)]++;                 \
            status = tm_mutex_unlock(0);                  \
            if (status != TM_SUCCESS) break;              \
            tm_thread_relinquish();                       \
        }                                                 \
    } while (0)

void tm_barge_worker_0(void) { WORKER_BODY(0); }
void tm_barge_worker_1(void) { WORKER_BODY(1); }
void tm_barge_worker_2(void) { WORKER_BODY(2); }
void tm_barge_worker_3(void) { WORKER_BODY(3); }

/*  Interloper body  */
/* 1-tick sleep prevents starvation; on wakeup the interloper is inserted at
 * the HEAD of the wait list (priority 3 > 10), exercising the priority-
 * ordered insertion path on every cycle. */
#define INTERLOPER_BODY(idx)                              \
    do {                                                  \
        int status;                                       \
        while (1) {                                       \
            status = tm_mutex_lock(0);                    \
            if (status != TM_SUCCESS) break;              \
            tm_barge_shared++;                            \
            tm_barge_per_thread[TM_BARGE_NUM_WORKERS + (idx)]++; \
            status = tm_mutex_unlock(0);                  \
            if (status != TM_SUCCESS) break;              \
            tm_thread_relinquish();                       \
            tm_thread_sleep_ticks(1); /* 1 ms at 1 kHz */\
        }                                                 \
    } while (0)

void tm_barge_interloper_0(void) { INTERLOPER_BODY(0); }
void tm_barge_interloper_1(void) { INTERLOPER_BODY(1); }
void tm_barge_interloper_2(void) { INTERLOPER_BODY(2); }
void tm_barge_interloper_3(void) { INTERLOPER_BODY(3); }

/*  Reporter  */
void tm_barge_reporter(void)
{
    uint64_t      last_shared = 0;
    uint64_t      last_thread[TM_BARGE_TOTAL_CONTENDERS];
    unsigned long relative_time = 0;
    int           i;

    for (i = 0; i < TM_BARGE_TOTAL_CONTENDERS; i++)
        last_thread[i] = 0;

    TM_REPORT_LOOP
    {
        tm_thread_sleep(tm_test_duration);
        relative_time += (unsigned long)tm_test_duration;

        tm_printf("**** Thread-Metric Mutex Barging Test **** "
                  "Relative Time: %lu\n", relative_time);

        uint64_t cur_shared = tm_barge_shared;
        uint64_t cur_thread[TM_BARGE_TOTAL_CONTENDERS];
        for (i = 0; i < TM_BARGE_TOTAL_CONTENDERS; i++)
            cur_thread[i] = tm_barge_per_thread[i];

        /* Correctness */
        uint64_t sum_delta = 0;
        for (i = 0; i < TM_BARGE_TOTAL_CONTENDERS; i++)
            sum_delta += cur_thread[i] - last_thread[i];
        uint64_t shared_delta = cur_shared - last_shared;

        if (shared_delta == 0)
            tm_printf("ERROR: shared counter stalled!\n");
        else if (shared_delta != sum_delta)
            tm_printf("ERROR: lost update! shared=%lu sum_threads=%lu\n",
                      (unsigned long)shared_delta, (unsigned long)sum_delta);

        /* Worker totals */
        uint64_t worker_total = 0;
        for (i = 0; i < TM_BARGE_NUM_WORKERS; i++)
            worker_total += cur_thread[i] - last_thread[i];
        uint64_t barge_total = shared_delta - worker_total;

        /* Barge fraction x1000 */
        unsigned long barge_frac  = (shared_delta > 0)
            ? (unsigned long)(barge_total * 1000u / shared_delta) : 0;

        tm_printf("Total ops this window : %lu\n", (unsigned long)shared_delta);
        tm_printf("  Workers (total)     : %lu\n", (unsigned long)worker_total);
        tm_printf("  Interlopers (total) : %lu  (barge fraction: %lu.%lu%lu%lu)\n",
                  (unsigned long)barge_total,
                  barge_frac / 1000u,
                  (barge_frac % 1000u) / 100u,
                  (barge_frac % 100u) / 10u,
                  barge_frac % 10u);

        tm_printf("  -- Workers --\n");
        for (i = 0; i < TM_BARGE_NUM_WORKERS; i++)
            tm_printf("    Worker %d (pri 10): %lu\n", i,
                      (unsigned long)(cur_thread[i] - last_thread[i]));

        tm_printf("  -- Interlopers --\n");
        for (i = 0; i < TM_BARGE_NUM_INTERLOPERS; i++)
            tm_printf("    Interloper %d (pri 3): %lu\n", i,
                      (unsigned long)(cur_thread[TM_BARGE_NUM_WORKERS + i]
                                      - last_thread[TM_BARGE_NUM_WORKERS + i]));
        tm_printf("\n");

        last_shared = cur_shared;
        for (i = 0; i < TM_BARGE_TOTAL_CONTENDERS; i++)
            last_thread[i] = cur_thread[i];
    }
    TM_REPORT_FINISH;
}
