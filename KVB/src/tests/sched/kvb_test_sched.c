#include <stdint.h>

#include "kvb.h"
#include "../kvb_test_helpers.h"

typedef struct {
    KvbSemaphore *start_sem;
    KvbSemaphore *done_sem;
    volatile uint32_t *stop_flag;
    volatile uint32_t counter;
} KvbSchedWorkerCtx;

static void kvb_sched_worker(void *arg)
{
    KvbSchedWorkerCtx *ctx = (KvbSchedWorkerCtx *)arg;

    if (kvb_sem_wait(ctx->start_sem, KVB_WAIT_FOREVER) != KVB_OK) {
        (void)kvb_sem_post(ctx->done_sem);
        for (;;) {
            (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
        }
    }

    while (*(ctx->stop_flag) == 0u) {
        ctx->counter++;
        (void)kvb_thread_yield();
    }

    (void)kvb_sem_post(ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_sched_coop(KvbTestResult *result)
{
    /* Each worker stack is sized via KVB_THREAD_BUF_SIZE so that every
       kernel — including kernels that store the TCB inside the caller-
       supplied block — receives KVB_DEFAULT_STACK_SIZE bytes of usable
       stack. */
    static KVB_THREAD_STACK_ARRAY_DEFINE(stacks, KVB_WORKER_THREAD_COUNT, KVB_DEFAULT_STACK_SIZE);
    KvbThread threads[KVB_WORKER_THREAD_COUNT];
    KvbSchedWorkerCtx ctx[KVB_WORKER_THREAD_COUNT];
    KvbSemaphore start_sem;
    KvbSemaphore done_sem;
    volatile uint32_t stop_flag = 0u;
    uint64_t total = 0u;
    uint32_t min_count = 0xFFFFFFFFu;
    uint32_t max_count = 0u;
    uint32_t i;

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&start_sem, 0u, KVB_WORKER_THREAD_COUNT),
                         "start semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&done_sem, 0u, KVB_WORKER_THREAD_COUNT),
                         "done semaphore create failed");

    for (i = 0u; i < KVB_WORKER_THREAD_COUNT; ++i) {
        ctx[i].start_sem = &start_sem;
        ctx[i].done_sem = &done_sem;
        ctx[i].stop_flag = &stop_flag;
        ctx[i].counter = 0u;

        KVB_ASSERT_STATUS_OK(kvb_thread_create(&threads[i],
                                               "kvb_sched_worker",
                                               kvb_sched_worker,
                                               &ctx[i],
                                               KVB_THREAD_STACK_ARRAY_MEM(stacks, i),
                                               KVB_THREAD_STACK_ARRAY_SIZE(stacks, i),
                                               KVB_PRIORITY_NORMAL),
                             "worker thread create failed");
    }

    for (i = 0u; i < KVB_WORKER_THREAD_COUNT; ++i) {
        KVB_ASSERT_STATUS_OK(kvb_sem_post(&start_sem), "start semaphore post failed");
    }

    KVB_ASSERT_STATUS_OK(kvb_thread_sleep_ticks(kvb_test_ticks_from_ms(KVB_MEASUREMENT_MS)),
                         "measurement sleep failed");

    stop_flag = 1u;

    for (i = 0u; i < KVB_WORKER_THREAD_COUNT; ++i) {
        KVB_ASSERT_STATUS_OK(kvb_sem_wait(&done_sem, KVB_TEST_TIMEOUT_TICKS),
                             "worker did not finish");
    }

    for (i = 0u; i < KVB_WORKER_THREAD_COUNT; ++i) {
        uint32_t c = ctx[i].counter;

        if (c == 0u) {
            uint32_t j;
            for (j = 0u; j < KVB_WORKER_THREAD_COUNT; ++j) {
                (void)kvb_thread_delete(&threads[j]);
            }
            (void)kvb_sem_delete(&done_sem);
            (void)kvb_sem_delete(&start_sem);
            result->state = KVB_RESULT_FAIL;
            result->message = "worker thread did not run";
            return;
        }

        if (c < min_count) {
            min_count = c;
        }

        if (c > max_count) {
            max_count = c;
        }

        total += c;
    }

    for (i = 0u; i < KVB_WORKER_THREAD_COUNT; ++i) {
        (void)kvb_thread_delete(&threads[i]);
    }
    (void)kvb_sem_delete(&done_sem);
    (void)kvb_sem_delete(&start_sem);

    kvb_result_add_metric(result, "workers", KVB_WORKER_THREAD_COUNT, "threads");
    kvb_result_add_metric(result, "total_yields", (int64_t)total, "yields");
    kvb_result_add_metric(result, "min_thread_count", min_count, "iterations");
    kvb_result_add_metric(result, "max_thread_count", max_count, "iterations");
    kvb_result_add_metric(result, "measurement_ms", KVB_MEASUREMENT_MS, "ms");

    result->state = KVB_RESULT_PASS;
    result->message = "cooperative yield benchmark valid";
}

const KvbTestCase kvb_test_sched_coop_001 = {
    "SCHED_COOP_001",
    "cooperative yield throughput",
    "SCHED",
    kvb_run_sched_coop
};
