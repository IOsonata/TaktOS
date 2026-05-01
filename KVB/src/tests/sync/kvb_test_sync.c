#include <stdint.h>

#include "kvb.h"
#include "../kvb_test_helpers.h"

static void kvb_run_sem_fast(KvbTestResult *result)
{
    KvbSemaphore sem;
    uint64_t start;
    uint64_t now = 0u;
    uint64_t elapsed;
    uint64_t target_us = (uint64_t)KVB_MEASUREMENT_MS * 1000ull;
    uint32_t ops = 0u;

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&sem, 1u, 1u), "semaphore create failed");

    start = kvb_test_time_now_us();

    /* Hot loop: read the wall clock only once per KVB_THROUGHPUT_BATCH ops
       so the time-read overhead does not bias measurements on kernels with
       very fast post/wait fast paths. */
    for (;;) {
        uint32_t i;

        for (i = 0u; i < KVB_THROUGHPUT_BATCH; ++i) {
            if (kvb_sem_wait(&sem, KVB_NO_WAIT) != KVB_OK) {
                (void)kvb_sem_delete(&sem);
                result->state = KVB_RESULT_FAIL;
                result->message = "semaphore wait failed";
                return;
            }

            if (kvb_sem_post(&sem) != KVB_OK) {
                (void)kvb_sem_delete(&sem);
                result->state = KVB_RESULT_FAIL;
                result->message = "semaphore post failed";
                return;
            }
        }

        ops += KVB_THROUGHPUT_BATCH;
        now = kvb_test_time_now_us();
        if (kvb_test_elapsed_us(start, now) >= target_us) {
            break;
        }
    }

    elapsed = kvb_test_elapsed_us(start, now);
    if (elapsed == 0u) {
        (void)kvb_sem_delete(&sem);
        result->state = KVB_RESULT_INVALID;
        result->message = "platform microsecond source unavailable";
        return;
    }

    kvb_result_add_metric(result, "operations", (int64_t)ops, "pairs");
    kvb_result_add_metric(result, "elapsed_us", (int64_t)elapsed, "us");
    kvb_result_add_metric(result, "throughput",
                          (int64_t)((uint64_t)ops * 1000000ull / elapsed),
                          "pairs_per_sec");

    (void)kvb_sem_delete(&sem);

    result->state = KVB_RESULT_PASS;
    result->message = "semaphore wait/post fast path valid";
}

static void kvb_run_mutex_fast(KvbTestResult *result)
{
    KvbMutex mutex;
    uint64_t start;
    uint64_t now = 0u;
    uint64_t elapsed;
    uint64_t target_us = (uint64_t)KVB_MEASUREMENT_MS * 1000ull;
    uint32_t ops = 0u;

    KVB_ASSERT_STATUS_OK(kvb_mutex_create(&mutex, KVB_MUTEX_NORMAL), "mutex create failed");

    start = kvb_test_time_now_us();

    for (;;) {
        uint32_t i;

        for (i = 0u; i < KVB_THROUGHPUT_BATCH; ++i) {
            if (kvb_mutex_lock(&mutex, KVB_NO_WAIT) != KVB_OK) {
                (void)kvb_mutex_delete(&mutex);
                result->state = KVB_RESULT_FAIL;
                result->message = "mutex lock failed";
                return;
            }

            if (kvb_mutex_unlock(&mutex) != KVB_OK) {
                (void)kvb_mutex_delete(&mutex);
                result->state = KVB_RESULT_FAIL;
                result->message = "mutex unlock failed";
                return;
            }
        }

        ops += KVB_THROUGHPUT_BATCH;
        now = kvb_test_time_now_us();
        if (kvb_test_elapsed_us(start, now) >= target_us) {
            break;
        }
    }

    elapsed = kvb_test_elapsed_us(start, now);
    if (elapsed == 0u) {
        (void)kvb_mutex_delete(&mutex);
        result->state = KVB_RESULT_INVALID;
        result->message = "platform microsecond source unavailable";
        return;
    }

    kvb_result_add_metric(result, "operations", (int64_t)ops, "pairs");
    kvb_result_add_metric(result, "elapsed_us", (int64_t)elapsed, "us");
    kvb_result_add_metric(result, "throughput",
                          (int64_t)((uint64_t)ops * 1000000ull / elapsed),
                          "pairs_per_sec");

    (void)kvb_mutex_delete(&mutex);

    result->state = KVB_RESULT_PASS;
    result->message = "mutex lock/unlock fast path valid";
}

/* ------------------------------------------------------------------------
 * SYNC_MUTEX_PCP_FAST_001  apples-to-apples PI/PCP mutex fast path.
 *
 * The default SYNC_MUTEX_FAST_001 test creates each kernel's canonical mutex.
 * That is asymmetric:
 *   - TaktOS plain mutex performs no priority adjustment.
 *   - FreeRTOS, ThreadX (TX_INHERIT), and Zephyr always do priority-inheritance
 *     bookkeeping on every Lock/Unlock, even on the uncontended fast path.
 *
 * This test selects the priority-inversion-bounding variant on every kernel
 * that supports one  PCP on TaktOS, PI on the others  so the comparison is
 * like-for-like at the level of "mutex that bounds priority inversion".
 *
 * Kernels that report neither supports_mutex_priority_inheritance nor
 * supports_mutex_priority_protect are reported as KVB_RESULT_UNSUPPORTED.
 *
 * Single-threaded uncontended hot loop, identical structure to
 * kvb_run_mutex_fast.
 * --------------------------------------------------------------------- */
static void kvb_run_mutex_pcp_fast(KvbTestResult *result)
{
    const KvbKernelFeatures *features = kvb_kernel_features();
    KvbMutex mutex;
    KvbStatus create_status;
    uint64_t start;
    uint64_t now = 0u;
    uint64_t elapsed;
    uint64_t target_us = (uint64_t)KVB_MEASUREMENT_MS * 1000ull;
    uint32_t ops = 0u;
    const char *variant;

    if (features == 0) {
        result->state = KVB_RESULT_INVALID;
        result->message = "kernel features unavailable";
        return;
    }

    /* Selection rule: prefer PCP when available (stricter bound on inversion);
       fall back to PI when only PI is available.  Skip the test on kernels
       that offer neither.

       The KVB runner runs at KVB_PRIORITY_HIGHEST.  IPCP requires
       BasePriority <= Ceiling, so the ceiling must be HIGHEST.  At that
       ceiling the boost on Lock is a no-op (max(HIGHEST, HIGHEST) = HIGHEST),
       which is exactly what an uncontended-fast-path measurement wants:
       we exercise the lock+unlock kernel work without triggering a priority
       migration. */
    if (features->supports_mutex_priority_protect) {
        create_status = kvb_mutex_create_protect(&mutex, KVB_PRIORITY_HIGHEST);
        variant = "PCP";
    } else if (features->supports_mutex_priority_inheritance) {
        /* The kernel's canonical mutex is PI by construction (FreeRTOS,
           ThreadX with TX_INHERIT, Zephyr).  KVB_MUTEX_NORMAL maps to that. */
        create_status = kvb_mutex_create(&mutex, KVB_MUTEX_NORMAL);
        variant = "PI";
    } else {
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "kernel supports neither PI nor PCP mutexes";
        return;
    }

    if (create_status != KVB_OK) {
        result->state = KVB_RESULT_FAIL;
        result->message = "PI/PCP mutex create failed";
        return;
    }

    start = kvb_test_time_now_us();

    for (;;) {
        uint32_t i;

        for (i = 0u; i < KVB_THROUGHPUT_BATCH; ++i) {
            if (kvb_mutex_lock(&mutex, KVB_NO_WAIT) != KVB_OK) {
                (void)kvb_mutex_delete(&mutex);
                result->state = KVB_RESULT_FAIL;
                result->message = "PI/PCP mutex lock failed";
                return;
            }

            if (kvb_mutex_unlock(&mutex) != KVB_OK) {
                (void)kvb_mutex_delete(&mutex);
                result->state = KVB_RESULT_FAIL;
                result->message = "PI/PCP mutex unlock failed";
                return;
            }
        }

        ops += KVB_THROUGHPUT_BATCH;
        now = kvb_test_time_now_us();
        if (kvb_test_elapsed_us(start, now) >= target_us) {
            break;
        }
    }

    elapsed = kvb_test_elapsed_us(start, now);
    if (elapsed == 0u) {
        (void)kvb_mutex_delete(&mutex);
        result->state = KVB_RESULT_INVALID;
        result->message = "platform microsecond source unavailable";
        return;
    }

    kvb_result_add_metric(result, "operations", (int64_t)ops, "pairs");
    kvb_result_add_metric(result, "elapsed_us", (int64_t)elapsed, "us");
    kvb_result_add_metric(result, "throughput",
                          (int64_t)((uint64_t)ops * 1000000ull / elapsed),
                          "pairs_per_sec");
    /* Tag the result so post-processing can distinguish PI from PCP runs.
       Zero unit string keeps it out of the throughput aggregation. */
    kvb_result_add_metric(result, (variant[0] == 'P' && variant[1] == 'C')
                                   ? "pcp_variant"
                                   : "pi_variant",
                          1, "");

    (void)kvb_mutex_delete(&mutex);

    result->state = KVB_RESULT_PASS;
    result->message = "PI/PCP mutex lock/unlock fast path valid";
}

typedef struct {
    KvbMutex *mutex;
    KvbSemaphore *locked_sem;
    KvbSemaphore *release_sem;
    KvbSemaphore *done_sem;
    KvbStatus lock_status;
    KvbStatus unlock_status;
} KvbMutexOwnerCtx;

static void kvb_mutex_owner_thread(void *arg)
{
    KvbMutexOwnerCtx *ctx = (KvbMutexOwnerCtx *)arg;

    ctx->lock_status = kvb_mutex_lock(ctx->mutex, KVB_WAIT_FOREVER);
    (void)kvb_sem_post(ctx->locked_sem);

    (void)kvb_sem_wait(ctx->release_sem, KVB_WAIT_FOREVER);

    ctx->unlock_status = kvb_mutex_unlock(ctx->mutex);
    (void)kvb_sem_post(ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_mutex_ownership(KvbTestResult *result)
{
    /* Owner stack sized via KVB_THREAD_BUF_SIZE  same usable stack on
       every kernel port. */
    static KVB_THREAD_STACK_DEFINE(owner_stack, KVB_DEFAULT_STACK_SIZE);
    KvbMutex mutex;
    KvbSemaphore locked_sem;
    KvbSemaphore release_sem;
    KvbSemaphore done_sem;
    KvbThread owner_thread;
    KvbMutexOwnerCtx ctx;
    KvbStatus non_owner_unlock;

    KVB_ASSERT_STATUS_OK(kvb_mutex_create(&mutex, KVB_MUTEX_NORMAL), "mutex create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&locked_sem, 0u, 1u), "locked semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&release_sem, 0u, 1u), "release semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&done_sem, 0u, 1u), "done semaphore create failed");

    ctx.mutex = &mutex;
    ctx.locked_sem = &locked_sem;
    ctx.release_sem = &release_sem;
    ctx.done_sem = &done_sem;
    ctx.lock_status = KVB_ERR_KERNEL;
    ctx.unlock_status = KVB_ERR_KERNEL;

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&owner_thread,
                                           "kvb_mutex_owner",
                                           kvb_mutex_owner_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(owner_stack),
                                           KVB_THREAD_STACK_SIZE(owner_stack),
                                           KVB_PRIORITY_NORMAL),
                         "owner thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_sem_wait(&locked_sem, KVB_TEST_TIMEOUT_TICKS),
                         "owner did not lock mutex");
    KVB_ASSERT_TRUE(ctx.lock_status == KVB_OK, "owner failed to lock mutex");

    non_owner_unlock = kvb_mutex_unlock(&mutex);

    if (non_owner_unlock == KVB_OK) {
        (void)kvb_sem_post(&release_sem);
        /* Drain owner thread before reclaiming owner_thread storage. */
        (void)kvb_sem_wait(&done_sem, KVB_TEST_TIMEOUT_TICKS);
        (void)kvb_thread_delete(&owner_thread);
        (void)kvb_sem_delete(&done_sem);
        (void)kvb_sem_delete(&release_sem);
        (void)kvb_sem_delete(&locked_sem);
        (void)kvb_mutex_delete(&mutex);
        result->state = KVB_RESULT_FAIL;
        result->message = "non-owner mutex unlock succeeded";
        return;
    }

    (void)kvb_sem_post(&release_sem);
    KVB_ASSERT_STATUS_OK(kvb_sem_wait(&done_sem, KVB_TEST_TIMEOUT_TICKS),
                         "owner did not finish");
    KVB_ASSERT_TRUE(ctx.unlock_status == KVB_OK,
                    "owner failed to unlock mutex after ownership test");

    (void)kvb_thread_delete(&owner_thread);
    (void)kvb_sem_delete(&done_sem);
    (void)kvb_sem_delete(&release_sem);
    (void)kvb_sem_delete(&locked_sem);
    (void)kvb_mutex_delete(&mutex);

    kvb_result_add_metric(result, "non_owner_status", (int64_t)non_owner_unlock, "status");
    result->state = KVB_RESULT_PASS;
    result->message = "non-owner mutex unlock rejected";
}

const KvbTestCase kvb_test_sync_sem_fast_001 = {
    "SYNC_SEM_FAST_001",
    "uncontended semaphore wait/post",
    "SYNC",
    kvb_run_sem_fast
};

const KvbTestCase kvb_test_sync_mutex_fast_001 = {
    "SYNC_MUTEX_FAST_001",
    "uncontended mutex lock/unlock",
    "SYNC",
    kvb_run_mutex_fast
};

const KvbTestCase kvb_test_sync_mutex_pcp_fast_001 = {
    "SYNC_MUTEX_PCP_FAST_001",
    "uncontended PI/PCP mutex lock/unlock",
    "SYNC",
    kvb_run_mutex_pcp_fast
};

const KvbTestCase kvb_test_sync_mutex_ownership_001 = {
    "SYNC_MUTEX_OWNERSHIP_001",
    "mutex unlock by non-owner",
    "SYNC",
    kvb_run_mutex_ownership
};
