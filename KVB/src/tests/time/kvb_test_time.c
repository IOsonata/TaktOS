#include <stdint.h>

#include "kvb.h"
#include "../kvb_test_helpers.h"

#if defined(KVB_ENABLE_TAKT_WAKE_TRACE_TEST) && (KVB_ENABLE_TAKT_WAKE_TRACE_TEST != 0) && defined(KVB_PORT_TAKTOS) && (KVB_PORT_TAKTOS != 0)
typedef struct {
    volatile uint32_t Enabled;
    volatile uint32_t Sequence;
    volatile uint32_t TickCount;
    volatile uint32_t ExpiredCount;
    volatile uint32_t SwitchRequested;
    volatile uint32_t CurrentPriority;
    volatile uint32_t NextPriority;
    volatile uint32_t TickEntryCycles;
    volatile uint32_t AfterWakeCycles;
    volatile uint32_t BeforePendSVCycles;
    volatile uint32_t PendSVEntryCycles;
    volatile uint32_t PendSVReturnCycles;
} KvbTaktWakeTrace;

extern uint32_t TaktOSWakeTraceSupported(void) __attribute__((weak));
extern void TaktOSWakeTraceReset(void) __attribute__((weak));
extern void TaktOSWakeTraceEnable(uint32_t enable) __attribute__((weak));
extern const KvbTaktWakeTrace *TaktOSWakeTraceSnapshot(void) __attribute__((weak));
#endif

static uint64_t kvb_abs_diff_u64(uint64_t a, uint64_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

static void kvb_run_sleep_tick_validity(KvbTestResult *result)
{
    static const uint32_t sleep_ticks[] = {
        1u,
        2u,
        3u,
        5u,
        7u,
        11u,
        19u,
        37u,
        KVB_SLEEP_TEST_TICKS,
        KVB_SLEEP_TEST_LONG_TICKS
    };
    uint64_t tick_us;
    uint64_t total_requested_us = 0u;
    uint64_t total_elapsed_us = 0u;
    uint64_t total_abs_error_us = 0u;
    uint64_t max_abs_error_us = 0u;
    uint32_t total_samples = 0u;
    uint32_t active_case_count = 0u;
    uint32_t odd_case_count = 0u;
    uint32_t even_case_count = 0u;
    uint32_t early_count = 0u;
    uint32_t late_count = 0u;
    uint64_t max_early_ticks = 0u;
    uint64_t max_late_ticks = 0u;
    uint32_t worst_requested_ticks = 0u;
    uint32_t case_count;
    uint32_t case_index;

    tick_us = 1000000ull / (uint64_t)KVB_TICK_HZ;
    case_count = (uint32_t)(sizeof(sleep_ticks) / sizeof(sleep_ticks[0]));

    for (case_index = 0u; case_index < case_count; ++case_index) {
        uint32_t requested_ticks = sleep_ticks[case_index];
        uint32_t repeat;

        if (requested_ticks == 0u) {
            continue;
        }

        active_case_count++;
        if ((requested_ticks & 1u) != 0u) {
            odd_case_count++;
        } else {
            even_case_count++;
        }

        for (repeat = 0u; repeat < KVB_SLEEP_TEST_REPEAT_COUNT; ++repeat) {
            uint64_t start_ticks;
            uint64_t end_ticks;
            uint64_t elapsed_ticks;
            uint64_t requested_us;
            uint64_t elapsed_us;
            uint64_t abs_error_us;
            uint64_t min_expected_ticks;
            uint64_t max_expected_ticks;
            KvbStatus st;

            start_ticks = kvb_kernel_tick_count();
            st = kvb_thread_sleep_ticks(requested_ticks);
            end_ticks = kvb_kernel_tick_count();

            KVB_ASSERT_TRUE(st == KVB_OK, "thread sleep failed");

            elapsed_ticks = end_ticks - start_ticks;
            requested_us = ((uint64_t)requested_ticks * 1000000ull) /
                           (uint64_t)KVB_TICK_HZ;
            elapsed_us = elapsed_ticks * tick_us;
            abs_error_us = kvb_abs_diff_u64(elapsed_us, requested_us);

#if KVB_SLEEP_TEST_STRICT_MINIMUM
            min_expected_ticks = (uint64_t)requested_ticks;
#else
            min_expected_ticks = (uint64_t)requested_ticks;
#endif
            max_expected_ticks = (uint64_t)requested_ticks +
                                 (uint64_t)KVB_SLEEP_TEST_MAX_EXTRA_TICKS;

            total_requested_us += requested_us;
            total_elapsed_us += elapsed_us;
            total_abs_error_us += abs_error_us;
            total_samples++;

            if (abs_error_us > max_abs_error_us) {
                max_abs_error_us = abs_error_us;
                worst_requested_ticks = requested_ticks;
            }

            if (elapsed_ticks < (uint64_t)requested_ticks) {
                uint64_t early_ticks = (uint64_t)requested_ticks - elapsed_ticks;
                early_count++;
                if (early_ticks > max_early_ticks) {
                    max_early_ticks = early_ticks;
                }
            } else if (elapsed_ticks > (uint64_t)requested_ticks) {
                uint64_t late_ticks = elapsed_ticks - (uint64_t)requested_ticks;
                late_count++;
                if (late_ticks > max_late_ticks) {
                    max_late_ticks = late_ticks;
                }
            }

            if (elapsed_ticks < min_expected_ticks) {
                result->state = KVB_RESULT_FAIL;
                result->message = "sleep returned before requested tick count";
                kvb_result_add_metric(result, "requested_ticks", (int64_t)requested_ticks, "ticks");
                kvb_result_add_metric(result, "elapsed_ticks", (int64_t)elapsed_ticks, "ticks");
                kvb_result_add_metric(result, "repeat", (int64_t)repeat, "sample");
                return;
            }

            if (elapsed_ticks > max_expected_ticks) {
                result->state = KVB_RESULT_FAIL;
                result->message = "sleep exceeded maximum tick tolerance";
                kvb_result_add_metric(result, "requested_ticks", (int64_t)requested_ticks, "ticks");
                kvb_result_add_metric(result, "elapsed_ticks", (int64_t)elapsed_ticks, "ticks");
                kvb_result_add_metric(result, "max_expected_ticks", (int64_t)max_expected_ticks, "ticks");
                kvb_result_add_metric(result, "repeat", (int64_t)repeat, "sample");
                return;
            }
        }
    }

    kvb_result_add_metric(result, "sleep_cases", (int64_t)active_case_count, "cases");
    kvb_result_add_metric(result, "odd_sleep_cases", (int64_t)odd_case_count, "cases");
    kvb_result_add_metric(result, "even_sleep_cases", (int64_t)even_case_count, "cases");
    kvb_result_add_metric(result, "repeat_count", (int64_t)KVB_SLEEP_TEST_REPEAT_COUNT, "samples_per_case");
    kvb_result_add_metric(result, "total_samples", (int64_t)total_samples, "samples");
    kvb_result_add_metric(result, "tick_us", (int64_t)tick_us, "us");
    kvb_result_add_metric(result, "avg_requested_us",
                          (int64_t)(total_requested_us / (uint64_t)total_samples), "us");
    kvb_result_add_metric(result, "avg_elapsed_us",
                          (int64_t)(total_elapsed_us / (uint64_t)total_samples), "us");
    kvb_result_add_metric(result, "avg_abs_error_us",
                          (int64_t)(total_abs_error_us / (uint64_t)total_samples), "us");
    kvb_result_add_metric(result, "max_abs_error_us", (int64_t)max_abs_error_us, "us");
    kvb_result_add_metric(result, "worst_requested_ticks", (int64_t)worst_requested_ticks, "ticks");
    kvb_result_add_metric(result, "early_count", (int64_t)early_count, "samples");
    kvb_result_add_metric(result, "late_count", (int64_t)late_count, "samples");
    kvb_result_add_metric(result, "max_early_ticks", (int64_t)max_early_ticks, "ticks");
    kvb_result_add_metric(result, "max_late_ticks", (int64_t)max_late_ticks, "ticks");

    result->state = KVB_RESULT_PASS;
    result->message = "sleep accuracy measured across multiple durations";
}

const KvbTestCase kvb_test_time_sleep_001 = {
    "TIME_SLEEP_001",
    "thread sleep accuracy across durations",
    "TIME",
    kvb_run_sleep_tick_validity
};

/* ------------------------------------------------------------------------
 * Real-time responsiveness tests
 * --------------------------------------------------------------------- */

#if defined(KVB_ENABLE_RT_TESTS) && (KVB_ENABLE_RT_TESTS != 0)

typedef struct {
    uint32_t samples;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t total_cycles;
} KvbRtStats;

static void kvb_rt_stats_init(KvbRtStats *stats)
{
    stats->samples = 0u;
    stats->min_cycles = 0xFFFFFFFFFFFFFFFFull;
    stats->max_cycles = 0u;
    stats->total_cycles = 0u;
}

static void kvb_rt_stats_add(KvbRtStats *stats, uint64_t cycles)
{
    if (cycles < stats->min_cycles) {
        stats->min_cycles = cycles;
    }

    if (cycles > stats->max_cycles) {
        stats->max_cycles = cycles;
    }

    stats->total_cycles += cycles;
    stats->samples++;
}

static uint64_t kvb_rt_stats_avg(const KvbRtStats *stats)
{
    if (stats->samples == 0u) {
        return 0u;
    }

    return stats->total_cycles / (uint64_t)stats->samples;
}

static uint64_t kvb_rt_cycles_to_ns(uint64_t cycles, uint32_t hz)
{
    if (hz == 0u) {
        return 0u;
    }

    return (cycles * 1000000000ull) / (uint64_t)hz;
}

static uint64_t kvb_rt_us_to_cycles(uint64_t us, uint32_t hz)
{
    const uint64_t hz64 = (uint64_t)hz;

    if (hz == 0u) {
        return 0u;
    }

    return ((us / 1000000ull) * hz64) +
           (((us % 1000000ull) * hz64 + 500000ull) / 1000000ull);
}

static uint64_t kvb_rt_diff32(uint32_t end, uint32_t start)
{
    return (uint64_t)((uint32_t)(end - start));
}

/* RT tests use the best target timebase available.  Mainline Cortex-M
 * targets normally use DWT/CYCCNT.  ARMv6-M targets such as STM32F0308
 * have no DWT, but they can still run the same RT tests using the
 * platform microsecond clock.  The lower-resolution path reports latency
 * in microseconds; it is still a valid functional responsiveness test and
 * keeps the canonical KVB manifest intact on small MCUs. */
static int kvb_rt_has_cycle_counter(void)
{
    return kvb_platform_cycle_frequency_hz() != 0u;
}

static uint32_t kvb_rt_timebase_hz(void)
{
    uint32_t hz = kvb_platform_cycle_frequency_hz();

    if (hz != 0u) {
        return hz;
    }

    return 1000000u;
}

static const char *kvb_rt_timebase_unit(void)
{
    return kvb_rt_has_cycle_counter() ? "cycles" : "us";
}

static uint64_t kvb_rt_now(void)
{
    if (kvb_rt_has_cycle_counter()) {
        return kvb_platform_cycle_count();
    }

    return kvb_platform_time_us();
}

static uint32_t kvb_rt_now32(void)
{
    return (uint32_t)kvb_rt_now();
}

/* Reuse the same two helper-thread stack buffers across all RT tests.
 * KVB runs tests sequentially and deletes the helper threads before the
 * next test starts, so per-test static stacks only waste BSS on small
 * targets.  Two stacks are enough for all current RT tests; the tick
 * jitter test uses only stack 0. */
static KVB_THREAD_STACK_DEFINE(g_kvb_rt_stack0, KVB_RT_THREAD_STACK_SIZE);
static KVB_THREAD_STACK_DEFINE(g_kvb_rt_stack1, KVB_RT_THREAD_STACK_SIZE);

#if defined(KVB_PORT_TAKTOS) && (KVB_PORT_TAKTOS != 0)
static void kvb_rt_add_avg_max_metrics(KvbTestResult *result,
                                       const char *avg_name,
                                       const char *max_name,
                                       const KvbRtStats *stats)
{
    kvb_result_add_metric(result, avg_name, (int64_t)kvb_rt_stats_avg(stats), "cycles");
    kvb_result_add_metric(result, max_name, (int64_t)stats->max_cycles, "cycles");
}
#endif

static int kvb_rt_cycle_source_valid(KvbTestResult *result, uint32_t *hz_out)
{
    if (kvb_rt_has_cycle_counter()) {
        (void)kvb_platform_cycle_count();
        *hz_out = kvb_platform_cycle_frequency_hz();
        return 1;
    }

    /* No DWT/CYCCNT: validate the platform microsecond clock instead.
     * A one-tick sleep is enough to detect the weak default implementation
     * that returns zero forever. */
    {
        uint64_t before_us = kvb_platform_time_us();
        (void)kvb_thread_sleep_ticks(1u);
        uint64_t after_us = kvb_platform_time_us();

        if (after_us <= before_us) {
            result->state = KVB_RESULT_UNSUPPORTED;
            result->message = "high-resolution platform time unavailable";
            kvb_result_add_metric(result, "cycle_counter", 0, "bool");
            kvb_result_add_metric(result, "time_us_valid", 0, "bool");
            return 0;
        }
    }

    *hz_out = 1000000u;
    return 1;
}

static void kvb_rt_add_latency_metrics(KvbTestResult *result,
                                       const KvbRtStats *stats,
                                       uint32_t hz)
{
    uint64_t avg = kvb_rt_stats_avg(stats);
    const char *unit = kvb_rt_timebase_unit();

    kvb_result_add_metric(result, "iterations", (int64_t)stats->samples, "samples");
    kvb_result_add_metric(result, "timebase_hz", (int64_t)hz, "Hz");
    kvb_result_add_metric(result, "cycle_counter", kvb_rt_has_cycle_counter() ? 1 : 0, "bool");
    kvb_result_add_metric(result, "min_latency", (int64_t)stats->min_cycles, unit);
    kvb_result_add_metric(result, "avg_latency", (int64_t)avg, unit);
    kvb_result_add_metric(result, "max_latency", (int64_t)stats->max_cycles, unit);
    kvb_result_add_metric(result, "min_ns", (int64_t)kvb_rt_cycles_to_ns(stats->min_cycles, hz), "ns");
    kvb_result_add_metric(result, "avg_ns", (int64_t)kvb_rt_cycles_to_ns(avg, hz), "ns");
    kvb_result_add_metric(result, "max_ns", (int64_t)kvb_rt_cycles_to_ns(stats->max_cycles, hz), "ns");
}

typedef struct {
    KvbSemaphore ready_sem;
    KvbSemaphore wake_sem;
    KvbSemaphore ack_sem;
    KvbSemaphore done_sem;
    volatile uint64_t t0_cycles;
    KvbRtStats stats;
    uint32_t iterations;
    volatile uint32_t error_count;
} KvbRtSemWakeCtx;

static void kvb_rt_sem_waiter_thread(void *arg)
{
    KvbRtSemWakeCtx *ctx = (KvbRtSemWakeCtx *)arg;
    uint32_t i;

    (void)kvb_sem_post(&ctx->ready_sem);

    for (i = 0u; i < ctx->iterations; ++i) {
        if (kvb_sem_wait(&ctx->wake_sem, KVB_WAIT_FOREVER) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        kvb_rt_stats_add(&ctx->stats, kvb_rt_now() - ctx->t0_cycles);

        if (kvb_sem_post(&ctx->ack_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_rt_sem_controller_thread(void *arg)
{
    KvbRtSemWakeCtx *ctx = (KvbRtSemWakeCtx *)arg;
    uint32_t i;

    if (kvb_sem_wait(&ctx->ready_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
        ctx->error_count++;
        (void)kvb_sem_post(&ctx->done_sem);
        for (;;) {
            (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
        }
    }

    for (i = 0u; i < ctx->iterations; ++i) {
        ctx->t0_cycles = kvb_rt_now();

        if (kvb_sem_post(&ctx->wake_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_wait(&ctx->ack_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    (void)kvb_sem_post(&ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_rt_sem_wake(KvbTestResult *result)
{
    KvbRtSemWakeCtx ctx;
    KvbThread waiter;
    KvbThread controller;
    uint32_t hz;

    if (!kvb_kernel_features()->supports_semaphore) {
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "semaphore not supported";
        return;
    }

    if (!kvb_rt_cycle_source_valid(result, &hz)) {
        return;
    }

    ctx.t0_cycles = 0u;
    ctx.iterations = KVB_RT_LATENCY_ITERATIONS;
    ctx.error_count = 0u;
    kvb_rt_stats_init(&ctx.stats);

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ready_sem, 0u, 1u), "ready semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.wake_sem, 0u, 1u), "wake semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ack_sem, 0u, 1u), "ack semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.done_sem, 0u, 1u), "done semaphore create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&waiter,
                                           "kvb_rt_sem_waiter",
                                           kvb_rt_sem_waiter_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack0),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack0),
                                           KVB_PRIORITY_HIGH),
                         "semaphore waiter thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&controller,
                                           "kvb_rt_sem_controller",
                                           kvb_rt_sem_controller_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack1),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack1),
                                           KVB_PRIORITY_NORMAL),
                         "semaphore controller thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_sem_wait(&ctx.done_sem, KVB_TEST_TIMEOUT_TICKS),
                         "semaphore wake test did not finish");

    (void)kvb_thread_delete(&controller);
    (void)kvb_thread_delete(&waiter);
    (void)kvb_sem_delete(&ctx.done_sem);
    (void)kvb_sem_delete(&ctx.ack_sem);
    (void)kvb_sem_delete(&ctx.wake_sem);
    (void)kvb_sem_delete(&ctx.ready_sem);

    if (ctx.error_count != 0u || ctx.stats.samples != ctx.iterations) {
        result->state = KVB_RESULT_FAIL;
        result->message = "semaphore wake latency sequence failed";
        kvb_result_add_metric(result, "errors", (int64_t)ctx.error_count, "count");
        kvb_result_add_metric(result, "samples", (int64_t)ctx.stats.samples, "samples");
        return;
    }

    kvb_rt_add_latency_metrics(result, &ctx.stats, hz);
    result->state = KVB_RESULT_PASS;
    result->message = "blocked semaphore waiter wake latency measured";
}

typedef struct {
    uint32_t seq;
    uint64_t t0_cycles;
} KvbRtQueueMsg;

typedef struct {
    KvbSemaphore ready_sem;
    KvbSemaphore ack_sem;
    KvbSemaphore done_sem;
    KvbQueue queue;
    KvbRtStats stats;
    uint32_t iterations;
    volatile uint32_t error_count;
} KvbRtQueueWakeCtx;

static void kvb_rt_queue_receiver_thread(void *arg)
{
    KvbRtQueueWakeCtx *ctx = (KvbRtQueueWakeCtx *)arg;
    uint32_t i;

    (void)kvb_sem_post(&ctx->ready_sem);

    for (i = 0u; i < ctx->iterations; ++i) {
        KvbRtQueueMsg msg;

        if (kvb_queue_recv(&ctx->queue, &msg, KVB_WAIT_FOREVER) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (msg.seq != i) {
            ctx->error_count++;
            break;
        }

        kvb_rt_stats_add(&ctx->stats, kvb_rt_now() - msg.t0_cycles);

        if (kvb_sem_post(&ctx->ack_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_rt_queue_sender_thread(void *arg)
{
    KvbRtQueueWakeCtx *ctx = (KvbRtQueueWakeCtx *)arg;
    uint32_t i;

    if (kvb_sem_wait(&ctx->ready_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
        ctx->error_count++;
        (void)kvb_sem_post(&ctx->done_sem);
        for (;;) {
            (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
        }
    }

    for (i = 0u; i < ctx->iterations; ++i) {
        KvbRtQueueMsg msg;

        msg.seq = i;
        msg.t0_cycles = kvb_rt_now();

        if (kvb_queue_send(&ctx->queue, &msg, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_wait(&ctx->ack_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    (void)kvb_sem_post(&ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_rt_queue_wake(KvbTestResult *result)
{
    static KVB_QUEUE_STORAGE_DEFINE(queue_storage, sizeof(KvbRtQueueMsg), 1u);
    KvbRtQueueWakeCtx ctx;
    KvbThread receiver;
    KvbThread sender;
    uint32_t hz;

    if (!kvb_kernel_features()->supports_queue || !kvb_kernel_features()->supports_semaphore) {
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "queue or semaphore not supported";
        return;
    }

    if (!kvb_rt_cycle_source_valid(result, &hz)) {
        return;
    }

    ctx.iterations = KVB_RT_LATENCY_ITERATIONS;
    ctx.error_count = 0u;
    kvb_rt_stats_init(&ctx.stats);

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ready_sem, 0u, 1u), "ready semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ack_sem, 0u, 1u), "ack semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.done_sem, 0u, 1u), "done semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_queue_create(&ctx.queue,
                                          KVB_QUEUE_STORAGE_MEM(queue_storage),
                                          sizeof(KvbRtQueueMsg),
                                          1u),
                         "queue create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&receiver,
                                           "kvb_rt_queue_receiver",
                                           kvb_rt_queue_receiver_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack0),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack0),
                                           KVB_PRIORITY_HIGH),
                         "queue receiver thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&sender,
                                           "kvb_rt_queue_sender",
                                           kvb_rt_queue_sender_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack1),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack1),
                                           KVB_PRIORITY_NORMAL),
                         "queue sender thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_sem_wait(&ctx.done_sem, KVB_TEST_TIMEOUT_TICKS),
                         "queue wake test did not finish");

    (void)kvb_thread_delete(&sender);
    (void)kvb_thread_delete(&receiver);
    (void)kvb_queue_delete(&ctx.queue);
    (void)kvb_sem_delete(&ctx.done_sem);
    (void)kvb_sem_delete(&ctx.ack_sem);
    (void)kvb_sem_delete(&ctx.ready_sem);

    if (ctx.error_count != 0u || ctx.stats.samples != ctx.iterations) {
        result->state = KVB_RESULT_FAIL;
        result->message = "queue wake latency sequence failed";
        kvb_result_add_metric(result, "errors", (int64_t)ctx.error_count, "count");
        kvb_result_add_metric(result, "samples", (int64_t)ctx.stats.samples, "samples");
        return;
    }

    kvb_rt_add_latency_metrics(result, &ctx.stats, hz);
    result->state = KVB_RESULT_PASS;
    result->message = "blocked queue receiver wake latency measured";
}

typedef struct {
    KvbSemaphore start_sem;
    KvbSemaphore blocked_sem;
    KvbSemaphore ack_sem;
    KvbSemaphore done_sem;
    KvbMutex mutex;
    volatile uint64_t t0_cycles;
    KvbRtStats stats;
    uint32_t iterations;
    volatile uint32_t error_count;
} KvbRtMutexWakeCtx;

static void kvb_rt_mutex_waiter_thread(void *arg)
{
    KvbRtMutexWakeCtx *ctx = (KvbRtMutexWakeCtx *)arg;
    uint32_t i;

    for (i = 0u; i < ctx->iterations; ++i) {
        KvbStatus st;

        if (kvb_sem_wait(&ctx->start_sem, KVB_WAIT_FOREVER) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        st = kvb_mutex_lock(&ctx->mutex, KVB_NO_WAIT);
        if (st == KVB_OK) {
            (void)kvb_mutex_unlock(&ctx->mutex);
            ctx->error_count++;
            break;
        }

        if (kvb_sem_post(&ctx->blocked_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_mutex_lock(&ctx->mutex, KVB_WAIT_FOREVER) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        kvb_rt_stats_add(&ctx->stats, kvb_rt_now() - ctx->t0_cycles);

        if (kvb_mutex_unlock(&ctx->mutex) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_post(&ctx->ack_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_rt_mutex_owner_thread(void *arg)
{
    KvbRtMutexWakeCtx *ctx = (KvbRtMutexWakeCtx *)arg;
    uint32_t i;

    for (i = 0u; i < ctx->iterations; ++i) {
        if (kvb_mutex_lock(&ctx->mutex, KVB_WAIT_FOREVER) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_post(&ctx->start_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_wait(&ctx->blocked_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
            ctx->error_count++;
            (void)kvb_mutex_unlock(&ctx->mutex);
            break;
        }

        ctx->t0_cycles = kvb_rt_now();

        if (kvb_mutex_unlock(&ctx->mutex) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_wait(&ctx->ack_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    (void)kvb_sem_post(&ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_rt_mutex_wake(KvbTestResult *result)
{
    KvbRtMutexWakeCtx ctx;
    KvbThread waiter;
    KvbThread owner;
    uint32_t hz;

    if (!kvb_kernel_features()->supports_mutex || !kvb_kernel_features()->supports_semaphore) {
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "mutex or semaphore not supported";
        return;
    }

    if (!kvb_rt_cycle_source_valid(result, &hz)) {
        return;
    }

    ctx.t0_cycles = 0u;
    ctx.iterations = KVB_RT_LATENCY_ITERATIONS;
    ctx.error_count = 0u;
    kvb_rt_stats_init(&ctx.stats);

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.start_sem, 0u, 1u), "start semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.blocked_sem, 0u, 1u), "blocked semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ack_sem, 0u, 1u), "ack semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.done_sem, 0u, 1u), "done semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_mutex_create(&ctx.mutex, KVB_MUTEX_NORMAL), "mutex create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&waiter,
                                           "kvb_rt_mutex_waiter",
                                           kvb_rt_mutex_waiter_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack0),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack0),
                                           KVB_PRIORITY_HIGH),
                         "mutex waiter thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&owner,
                                           "kvb_rt_mutex_owner",
                                           kvb_rt_mutex_owner_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack1),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack1),
                                           KVB_PRIORITY_NORMAL),
                         "mutex owner thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_sem_wait(&ctx.done_sem, KVB_TEST_TIMEOUT_TICKS),
                         "mutex wake test did not finish");

    (void)kvb_thread_delete(&owner);
    (void)kvb_thread_delete(&waiter);
    (void)kvb_mutex_delete(&ctx.mutex);
    (void)kvb_sem_delete(&ctx.done_sem);
    (void)kvb_sem_delete(&ctx.ack_sem);
    (void)kvb_sem_delete(&ctx.blocked_sem);
    (void)kvb_sem_delete(&ctx.start_sem);

    if (ctx.error_count != 0u || ctx.stats.samples != ctx.iterations) {
        result->state = KVB_RESULT_FAIL;
        result->message = "mutex waiter wake latency sequence failed";
        kvb_result_add_metric(result, "errors", (int64_t)ctx.error_count, "count");
        kvb_result_add_metric(result, "samples", (int64_t)ctx.stats.samples, "samples");
        return;
    }

    kvb_rt_add_latency_metrics(result, &ctx.stats, hz);
    result->state = KVB_RESULT_PASS;
    result->message = "blocked mutex waiter wake latency measured";
}

typedef struct {
    KvbSemaphore done_sem;
    uint32_t sleep_case_count;
    uint32_t samples_per_case;
    uint32_t total_samples;
    uint32_t worst_requested_ticks;
    uint32_t early_samples;
    uint32_t late_samples;
    uint64_t total_target_cycles;
    uint64_t total_observed_cycles;
    int64_t total_error_cycles;
    int64_t min_error_cycles;
    int64_t max_error_cycles;
    uint64_t max_abs_jitter_cycles;
    uint64_t max_early_cycles;
    uint64_t max_late_cycles;
    uint32_t wall_time_samples;
    uint32_t tick_fallback_samples;
    uint32_t invalid_time_samples;
    volatile uint32_t error_count;
} KvbRtTickJitterCtx;

static const uint32_t g_kvb_rt_jitter_sleep_ticks[] = {
    1u,
    5u,
    10u,
    17u,
    23u,
    31u,
    47u
};

static uint32_t kvb_rt_jitter_case_count(void)
{
    return (uint32_t)(sizeof(g_kvb_rt_jitter_sleep_ticks) /
                      sizeof(g_kvb_rt_jitter_sleep_ticks[0]));
}

static int64_t kvb_rt_target_error_cycles(uint64_t observed_cycles,
                                          uint64_t target_cycles)
{
    if (observed_cycles >= target_cycles) {
        return (int64_t)(observed_cycles - target_cycles);
    }

    return -(int64_t)(target_cycles - observed_cycles);
}

static void kvb_rt_tick_jitter_thread(void *arg)
{
    KvbRtTickJitterCtx *ctx = (KvbRtTickJitterCtx *)arg;
    uint64_t tick_cycles;
    uint64_t tick_us;
    uint32_t case_index;
    uint32_t hz;

    hz = kvb_rt_timebase_hz();
    if (hz == 0u || KVB_TICK_HZ == 0u) {
        ctx->error_count++;
        (void)kvb_sem_post(&ctx->done_sem);
        for (;;) {
            (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
        }
    }

    tick_cycles = (uint64_t)hz / (uint64_t)KVB_TICK_HZ;
    tick_us = 1000000ull / (uint64_t)KVB_TICK_HZ;

    (void)kvb_thread_sleep_ticks(2u);

    for (case_index = 0u; case_index < ctx->sleep_case_count; ++case_index) {
        const uint32_t requested_ticks = g_kvb_rt_jitter_sleep_ticks[case_index];
        const uint64_t target_cycles = (uint64_t)requested_ticks * tick_cycles;
        uint32_t sample;

        if (requested_ticks == 0u) {
            continue;
        }

        for (sample = 0u; sample < ctx->samples_per_case; ++sample) {
            uint64_t start_us;
            uint64_t now_us;
            uint64_t start_ticks;
            uint64_t end_ticks;
            uint64_t elapsed_ticks;
            uint64_t period_us;
            uint64_t period;
            uint64_t abs_jitter;
            uint64_t requested_us;
            uint64_t max_sane_us;
            uint64_t max_sane_ticks;
            uint8_t have_period;
            int64_t error_cycles;

            /* Sleep target jitter is a wall-time property.  Prefer the
             * platform microsecond clock, but validate it before use.  If
             * the wall clock sample is not monotonic or is outside the
             * bounded sleep window, fall back to the kernel tick delta.  If
             * both sources are invalid, fail the test instead of allowing an
             * unsigned underflow to become a huge period and later a negative
             * metric after int64 conversion. */
            start_ticks = kvb_kernel_tick_count();
            start_us = kvb_platform_time_us();

            if (kvb_thread_sleep_ticks(requested_ticks) != KVB_OK) {
                ctx->error_count++;
                break;
            }

            now_us = kvb_platform_time_us();
            end_ticks = kvb_kernel_tick_count();
            requested_us = ((uint64_t)requested_ticks * 1000000ull) /
                           (uint64_t)KVB_TICK_HZ;
            max_sane_ticks = (uint64_t)requested_ticks +
                             (uint64_t)KVB_SLEEP_TEST_MAX_EXTRA_TICKS + 4ull;
            max_sane_us = requested_us +
                          (((uint64_t)KVB_SLEEP_TEST_MAX_EXTRA_TICKS + 4ull) * tick_us);

            period_us = 0u;
            have_period = 0u;
            if (now_us >= start_us) {
                const uint64_t wall_delta_us = now_us - start_us;

                if (wall_delta_us <= max_sane_us) {
                    period_us = wall_delta_us;
                    have_period = 1u;
                    ctx->wall_time_samples++;
                }
            }

            if (have_period == 0u && end_ticks >= start_ticks) {
                elapsed_ticks = end_ticks - start_ticks;
                if (elapsed_ticks <= max_sane_ticks) {
                    period_us = elapsed_ticks * tick_us;
                    have_period = 1u;
                    ctx->tick_fallback_samples++;
                }
            }

            if (have_period == 0u) {
                ctx->invalid_time_samples++;
                ctx->error_count++;
                break;
            }

            period = kvb_rt_us_to_cycles(period_us, hz);
            if (period > (uint64_t)INT64_MAX || target_cycles > (uint64_t)INT64_MAX) {
                ctx->invalid_time_samples++;
                ctx->error_count++;
                break;
            }
            error_cycles = kvb_rt_target_error_cycles(period, target_cycles);
            abs_jitter = kvb_abs_diff_u64(period, target_cycles);

            ctx->total_target_cycles += target_cycles;
            ctx->total_observed_cycles += period;
            ctx->total_error_cycles += error_cycles;
            ctx->total_samples++;

            if (error_cycles < ctx->min_error_cycles) {
                ctx->min_error_cycles = error_cycles;
            }
            if (error_cycles > ctx->max_error_cycles) {
                ctx->max_error_cycles = error_cycles;
            }

            if (error_cycles < 0) {
                const uint64_t early_cycles = (uint64_t)(-error_cycles);

                ctx->early_samples++;
                if (early_cycles > ctx->max_early_cycles) {
                    ctx->max_early_cycles = early_cycles;
                }
            } else if (error_cycles > 0) {
                const uint64_t late_cycles = (uint64_t)error_cycles;

                ctx->late_samples++;
                if (late_cycles > ctx->max_late_cycles) {
                    ctx->max_late_cycles = late_cycles;
                }
            }

            if (abs_jitter > ctx->max_abs_jitter_cycles) {
                ctx->max_abs_jitter_cycles = abs_jitter;
                ctx->worst_requested_ticks = requested_ticks;
            }
        }

        if (ctx->error_count != 0u) {
            break;
        }
    }

    (void)kvb_sem_post(&ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_rt_tick_jitter(KvbTestResult *result)
{
    KvbRtTickJitterCtx ctx;
    KvbThread jitter_thread;
    uint32_t hz;
    uint64_t tick_cycles;
    uint32_t expected_samples;

    if (!kvb_rt_cycle_source_valid(result, &hz)) {
        return;
    }

    ctx.sleep_case_count = kvb_rt_jitter_case_count();
    ctx.samples_per_case = KVB_RT_JITTER_ITERATIONS / ctx.sleep_case_count;
    if (ctx.samples_per_case == 0u) {
        ctx.samples_per_case = 1u;
    }

    ctx.total_samples = 0u;
    ctx.worst_requested_ticks = 0u;
    ctx.early_samples = 0u;
    ctx.late_samples = 0u;
    ctx.total_target_cycles = 0u;
    ctx.total_observed_cycles = 0u;
    ctx.total_error_cycles = 0;
    ctx.min_error_cycles = INT64_MAX;
    ctx.max_error_cycles = INT64_MIN;
    ctx.max_abs_jitter_cycles = 0u;
    ctx.max_early_cycles = 0u;
    ctx.max_late_cycles = 0u;
    ctx.wall_time_samples = 0u;
    ctx.tick_fallback_samples = 0u;
    ctx.invalid_time_samples = 0u;
    ctx.error_count = 0u;

    tick_cycles = (uint64_t)hz / (uint64_t)KVB_TICK_HZ;
    expected_samples = ctx.samples_per_case * ctx.sleep_case_count;

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.done_sem, 0u, 1u), "done semaphore create failed");

    KVB_ASSERT_STATUS_OK(kvb_thread_create(&jitter_thread,
                                           "kvb_rt_tick_jitter",
                                           kvb_rt_tick_jitter_thread,
                                           &ctx,
                                           KVB_THREAD_STACK_MEM(g_kvb_rt_stack0),
                                           KVB_THREAD_STACK_SIZE(g_kvb_rt_stack0),
                                           KVB_PRIORITY_HIGH),
                         "tick jitter thread create failed");

    KVB_ASSERT_STATUS_OK(kvb_sem_wait(&ctx.done_sem, KVB_TEST_TIMEOUT_TICKS),
                         "tick jitter test did not finish");

    (void)kvb_thread_delete(&jitter_thread);
    (void)kvb_sem_delete(&ctx.done_sem);

    if (ctx.error_count != 0u || ctx.total_samples != expected_samples) {
        result->state = KVB_RESULT_FAIL;
        result->message = "multi-duration tick jitter sequence failed";
        kvb_result_add_metric(result, "errors", (int64_t)ctx.error_count, "count");
        kvb_result_add_metric(result, "samples", (int64_t)ctx.total_samples, "samples");
        kvb_result_add_metric(result, "expected_samples", (int64_t)expected_samples, "samples");
        kvb_result_add_metric(result, "wall_time_samples", (int64_t)ctx.wall_time_samples, "samples");
        kvb_result_add_metric(result, "tick_fallback_samples", (int64_t)ctx.tick_fallback_samples, "samples");
        kvb_result_add_metric(result, "invalid_time_samples", (int64_t)ctx.invalid_time_samples, "samples");
        return;
    }

    kvb_result_add_metric(result, "sleep_cases", (int64_t)ctx.sleep_case_count, "cases");
    kvb_result_add_metric(result, "samples_per_case", (int64_t)ctx.samples_per_case, "samples");
    kvb_result_add_metric(result, "total_samples", (int64_t)ctx.total_samples, "samples");
    kvb_result_add_metric(result, "tick_cycles", (int64_t)tick_cycles, "cycles");
    kvb_result_add_metric(result, "avg_target_cycles",
                          (int64_t)(ctx.total_target_cycles / (uint64_t)ctx.total_samples), "cycles");
    kvb_result_add_metric(result, "avg_observed_cycles",
                          (int64_t)(ctx.total_observed_cycles / (uint64_t)ctx.total_samples), "cycles");
    kvb_result_add_metric(result, "avg_error_cycles",
                          (int64_t)(ctx.total_error_cycles / (int64_t)ctx.total_samples), "cycles");
    kvb_result_add_metric(result, "min_error_cycles", ctx.min_error_cycles, "cycles");
    kvb_result_add_metric(result, "max_error_cycles", ctx.max_error_cycles, "cycles");
    kvb_result_add_metric(result, "max_abs_jitter_cycles", (int64_t)ctx.max_abs_jitter_cycles, "cycles");
    kvb_result_add_metric(result, "max_abs_jitter_ns",
                          (int64_t)kvb_rt_cycles_to_ns(ctx.max_abs_jitter_cycles, hz), "ns");
    kvb_result_add_metric(result, "worst_requested_sleep_ticks",
                          (int64_t)ctx.worst_requested_ticks, "ticks");
    kvb_result_add_metric(result, "early_samples", (int64_t)ctx.early_samples, "samples");
    kvb_result_add_metric(result, "late_samples", (int64_t)ctx.late_samples, "samples");
    kvb_result_add_metric(result, "max_early_cycles", (int64_t)ctx.max_early_cycles, "cycles");
    kvb_result_add_metric(result, "max_late_cycles", (int64_t)ctx.max_late_cycles, "cycles");
    kvb_result_add_metric(result, "wall_time_samples", (int64_t)ctx.wall_time_samples, "samples");
    kvb_result_add_metric(result, "tick_fallback_samples", (int64_t)ctx.tick_fallback_samples, "samples");
    kvb_result_add_metric(result, "invalid_time_samples", (int64_t)ctx.invalid_time_samples, "samples");

    result->state = KVB_RESULT_PASS;
    result->message = "multi-duration sleep jitter measured against target wake time";
}


#if defined(KVB_ENABLE_TAKT_WAKE_TRACE_TEST) && (KVB_ENABLE_TAKT_WAKE_TRACE_TEST != 0)
static void kvb_run_rt_takt_wake_trace(KvbTestResult *result)
{
#if defined(KVB_PORT_TAKTOS) && (KVB_PORT_TAKTOS != 0)
    KvbRtStats tick_to_wake;
    KvbRtStats wake_to_pendsv;
    KvbRtStats pendsv_wait;
    KvbRtStats pendsv_body;
    KvbRtStats pendsv_to_thread;
    KvbRtStats total_tick_to_thread;
    uint32_t samples = 0u;
    uint32_t max_expired = 0u;
    uint32_t hz;
    uint32_t i;

    if (TaktOSWakeTraceSupported == 0 ||
        TaktOSWakeTraceReset == 0 ||
        TaktOSWakeTraceEnable == 0 ||
        TaktOSWakeTraceSnapshot == 0 ||
        TaktOSWakeTraceSupported() == 0u) {
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "TaktOS wake trace not enabled";
        kvb_result_add_metric(result, "trace_supported", 0, "bool");
        return;
    }

    if (!kvb_rt_cycle_source_valid(result, &hz)) {
        return;
    }

    kvb_rt_stats_init(&tick_to_wake);
    kvb_rt_stats_init(&wake_to_pendsv);
    kvb_rt_stats_init(&pendsv_wait);
    kvb_rt_stats_init(&pendsv_body);
    kvb_rt_stats_init(&pendsv_to_thread);
    kvb_rt_stats_init(&total_tick_to_thread);

    TaktOSWakeTraceEnable(1u);
    (void)kvb_thread_sleep_ticks(2u);

    for (i = 0u; i < KVB_RT_JITTER_ITERATIONS; ++i) {
        const KvbTaktWakeTrace *trace;
        uint32_t thread_cycles;

        if (kvb_thread_sleep_ticks(1u) != KVB_OK) {
            TaktOSWakeTraceEnable(0u);
            result->state = KVB_RESULT_FAIL;
            result->message = "sleep failed during wake trace diagnostic";
            return;
        }

        thread_cycles = kvb_rt_now32();
        trace = TaktOSWakeTraceSnapshot();

        if (trace == 0 ||
            trace->TickEntryCycles == 0u ||
            trace->AfterWakeCycles == 0u ||
            trace->BeforePendSVCycles == 0u ||
            trace->PendSVEntryCycles == 0u ||
            trace->PendSVReturnCycles == 0u) {
            TaktOSWakeTraceEnable(0u);
            result->state = KVB_RESULT_FAIL;
            result->message = "wake trace timestamps incomplete";
            kvb_result_add_metric(result, "sample", (int64_t)i, "index");
            return;
        }

        kvb_rt_stats_add(&tick_to_wake,
                         kvb_rt_diff32(trace->AfterWakeCycles, trace->TickEntryCycles));
        kvb_rt_stats_add(&wake_to_pendsv,
                         kvb_rt_diff32(trace->BeforePendSVCycles, trace->AfterWakeCycles));
        kvb_rt_stats_add(&pendsv_wait,
                         kvb_rt_diff32(trace->PendSVEntryCycles, trace->BeforePendSVCycles));
        kvb_rt_stats_add(&pendsv_body,
                         kvb_rt_diff32(trace->PendSVReturnCycles, trace->PendSVEntryCycles));
        kvb_rt_stats_add(&pendsv_to_thread,
                         kvb_rt_diff32(thread_cycles, trace->PendSVReturnCycles));
        kvb_rt_stats_add(&total_tick_to_thread,
                         kvb_rt_diff32(thread_cycles, trace->TickEntryCycles));

        if (trace->ExpiredCount > max_expired) {
            max_expired = trace->ExpiredCount;
        }
        samples++;
    }

    TaktOSWakeTraceEnable(0u);

    /* Keep this diagnostic within KVB_MAX_METRICS_PER_TEST so the runner
     * stack usage stays identical to the normal KVB build.  The omitted
     * switch_count/avg_expired values are useful but not needed for the
     * first jitter split; max_expired still catches unexpected multi-wake
     * samples. */
    kvb_result_add_metric(result, "trace_supported", 1, "bool");
    kvb_result_add_metric(result, "samples", (int64_t)samples, "samples");
    kvb_result_add_metric(result, "max_expired_threads", (int64_t)max_expired, "threads");
    kvb_rt_add_avg_max_metrics(result, "avg_tick_to_wake_cycles", "max_tick_to_wake_cycles", &tick_to_wake);
    kvb_rt_add_avg_max_metrics(result, "avg_wake_to_pendsv_cycles", "max_wake_to_pendsv_cycles", &wake_to_pendsv);
    kvb_rt_add_avg_max_metrics(result, "avg_pendsv_wait_cycles", "max_pendsv_wait_cycles", &pendsv_wait);
    kvb_rt_add_avg_max_metrics(result, "avg_pendsv_body_cycles", "max_pendsv_body_cycles", &pendsv_body);
    kvb_rt_add_avg_max_metrics(result, "avg_pendsv_to_thread_cycles", "max_pendsv_to_thread_cycles", &pendsv_to_thread);
    kvb_result_add_metric(result, "avg_tick_to_thread_cycles",
                          (int64_t)kvb_rt_stats_avg(&total_tick_to_thread), "cycles");
    kvb_result_add_metric(result, "max_tick_to_thread_cycles",
                          (int64_t)total_tick_to_thread.max_cycles, "cycles");
    kvb_result_add_metric(result, "max_tick_to_thread_ns",
                          (int64_t)kvb_rt_cycles_to_ns(total_tick_to_thread.max_cycles, hz), "ns");

    result->state = KVB_RESULT_PASS;
    result->message = "TaktOS tick-to-thread wake trace measured";
#else
    result->state = KVB_RESULT_UNSUPPORTED;
    result->message = "TaktOS wake trace diagnostic is TaktOS-specific";
    kvb_result_add_metric(result, "trace_supported", 0, "bool");
#endif
}

const KvbTestCase kvb_test_rt_takt_wake_trace_001 = {
    "RT_TAKT_WAKE_TRACE_001",
    "TaktOS tick-to-thread wake trace diagnostic",
    "RT",
    kvb_run_rt_takt_wake_trace
};
#endif

typedef struct {
    KvbSemaphore ready_sem;
    KvbSemaphore wake_sem;
    KvbSemaphore ack_sem;
    KvbSemaphore done_sem;
    KvbRtStats trigger_to_irq_stats;
    KvbRtStats irq_to_thread_stats;
    KvbRtStats trigger_to_thread_stats;
    uint32_t iterations;
    volatile uint32_t trigger_cycles;
    volatile uint32_t irq_entry_cycles;
    volatile uint32_t irq_seen;
    volatile uint32_t error_count;
    volatile uint32_t isr_error_count;
} KvbRtIrqCtx;

static void kvb_rt_add_named_latency_metrics(KvbTestResult *result,
                                             const char *avg_name,
                                             const char *max_name,
                                             const char *max_ns_name,
                                             const KvbRtStats *stats,
                                             uint32_t hz)
{
    const char *unit = kvb_rt_timebase_unit();

    kvb_result_add_metric(result, avg_name, (int64_t)kvb_rt_stats_avg(stats), unit);
    kvb_result_add_metric(result, max_name, (int64_t)stats->max_cycles, unit);
    kvb_result_add_metric(result, max_ns_name,
                          (int64_t)kvb_rt_cycles_to_ns(stats->max_cycles, hz), "ns");
}

static void kvb_rt_irq_probe_handler(void *arg)
{
    KvbRtIrqCtx *ctx = (KvbRtIrqCtx *)arg;

    ctx->irq_entry_cycles = kvb_rt_now32();
    ctx->irq_seen++;

    if (kvb_sem_post_from_isr(&ctx->wake_sem) != KVB_OK) {
        ctx->isr_error_count++;
    }
}

static void kvb_rt_irq_waiter_thread(void *arg)
{
    KvbRtIrqCtx *ctx = (KvbRtIrqCtx *)arg;
    uint32_t i;

    (void)kvb_sem_post(&ctx->ready_sem);

    for (i = 0u; i < ctx->iterations; ++i) {
        uint32_t thread_cycles;
        uint32_t trigger_cycles;
        uint32_t irq_entry_cycles;

        if (kvb_sem_wait(&ctx->wake_sem, KVB_WAIT_FOREVER) != KVB_OK) {
            ctx->error_count++;
            break;
        }

        thread_cycles = kvb_rt_now32();
        trigger_cycles = ctx->trigger_cycles;
        irq_entry_cycles = ctx->irq_entry_cycles;

        if (ctx->irq_seen != 1u) {
            ctx->error_count++;
            break;
        }

        kvb_rt_stats_add(&ctx->trigger_to_irq_stats,
                         kvb_rt_diff32(irq_entry_cycles, trigger_cycles));
        kvb_rt_stats_add(&ctx->irq_to_thread_stats,
                         kvb_rt_diff32(thread_cycles, irq_entry_cycles));
        kvb_rt_stats_add(&ctx->trigger_to_thread_stats,
                         kvb_rt_diff32(thread_cycles, trigger_cycles));

        if (kvb_sem_post(&ctx->ack_sem) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_rt_irq_controller_thread(void *arg)
{
    KvbRtIrqCtx *ctx = (KvbRtIrqCtx *)arg;
    uint32_t i;

    if (kvb_sem_wait(&ctx->ready_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
        ctx->error_count++;
        (void)kvb_sem_post(&ctx->done_sem);
        for (;;) {
            (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
        }
    }

    for (i = 0u; i < ctx->iterations; ++i) {
        ctx->irq_entry_cycles = 0u;
        ctx->irq_seen = 0u;
        ctx->trigger_cycles = kvb_rt_now32();

        if (kvb_platform_irq_probe_trigger() != KVB_OK) {
            ctx->error_count++;
            break;
        }

        if (kvb_sem_wait(&ctx->ack_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
            ctx->error_count++;
            break;
        }
    }

    (void)kvb_sem_post(&ctx->done_sem);

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}

static void kvb_run_rt_irq_mask(KvbTestResult *result)
{
    KvbRtIrqCtx ctx;
    KvbThread waiter;
    KvbThread controller;
    KvbStatus irq_st;
    uint32_t hz;

    if (!kvb_kernel_features()->supports_semaphore ||
        !kvb_kernel_features()->supports_isr_kernel_api) {
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "semaphore ISR API not supported";
        kvb_result_add_metric(result, "irq_probe_supported", 0, "bool");
        return;
    }

    if (!kvb_rt_cycle_source_valid(result, &hz)) {
        return;
    }

    ctx.iterations = KVB_RT_LATENCY_ITERATIONS;
    ctx.trigger_cycles = 0u;
    ctx.irq_entry_cycles = 0u;
    ctx.irq_seen = 0u;
    ctx.error_count = 0u;
    ctx.isr_error_count = 0u;
    kvb_rt_stats_init(&ctx.trigger_to_irq_stats);
    kvb_rt_stats_init(&ctx.irq_to_thread_stats);
    kvb_rt_stats_init(&ctx.trigger_to_thread_stats);

    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ready_sem, 0u, 1u), "IRQ ready semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.wake_sem, 0u, 1u), "IRQ wake semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.ack_sem, 0u, 1u), "IRQ ack semaphore create failed");
    KVB_ASSERT_STATUS_OK(kvb_sem_create(&ctx.done_sem, 0u, 1u), "IRQ done semaphore create failed");

    irq_st = kvb_platform_irq_probe_init(kvb_rt_irq_probe_handler, &ctx);
    if (irq_st == KVB_ERR_UNSUPPORTED) {
        (void)kvb_sem_delete(&ctx.done_sem);
        (void)kvb_sem_delete(&ctx.ack_sem);
        (void)kvb_sem_delete(&ctx.wake_sem);
        (void)kvb_sem_delete(&ctx.ready_sem);
        result->state = KVB_RESULT_UNSUPPORTED;
        result->message = "target IRQ probe hook not implemented";
        kvb_result_add_metric(result, "irq_probe_supported", 0, "bool");
        return;
    }

    if (irq_st != KVB_OK) {
        (void)kvb_sem_delete(&ctx.done_sem);
        (void)kvb_sem_delete(&ctx.ack_sem);
        (void)kvb_sem_delete(&ctx.wake_sem);
        (void)kvb_sem_delete(&ctx.ready_sem);
        result->state = KVB_RESULT_FAIL;
        result->message = "target IRQ probe hook init failed";
        kvb_result_add_metric(result, "irq_probe_supported", 0, "bool");
        return;
    }

    if (kvb_thread_create(&waiter,
                          "kvb_rt_irq_waiter",
                          kvb_rt_irq_waiter_thread,
                          &ctx,
                          KVB_THREAD_STACK_MEM(g_kvb_rt_stack0),
                          KVB_THREAD_STACK_SIZE(g_kvb_rt_stack0),
                          KVB_PRIORITY_HIGH) != KVB_OK) {
        (void)kvb_platform_irq_probe_disable();
        (void)kvb_sem_delete(&ctx.done_sem);
        (void)kvb_sem_delete(&ctx.ack_sem);
        (void)kvb_sem_delete(&ctx.wake_sem);
        (void)kvb_sem_delete(&ctx.ready_sem);
        result->state = KVB_RESULT_FAIL;
        result->message = "IRQ waiter thread create failed";
        return;
    }

    if (kvb_thread_create(&controller,
                          "kvb_rt_irq_controller",
                          kvb_rt_irq_controller_thread,
                          &ctx,
                          KVB_THREAD_STACK_MEM(g_kvb_rt_stack1),
                          KVB_THREAD_STACK_SIZE(g_kvb_rt_stack1),
                          KVB_PRIORITY_NORMAL) != KVB_OK) {
        (void)kvb_thread_delete(&waiter);
        (void)kvb_platform_irq_probe_disable();
        (void)kvb_sem_delete(&ctx.done_sem);
        (void)kvb_sem_delete(&ctx.ack_sem);
        (void)kvb_sem_delete(&ctx.wake_sem);
        (void)kvb_sem_delete(&ctx.ready_sem);
        result->state = KVB_RESULT_FAIL;
        result->message = "IRQ controller thread create failed";
        return;
    }

    if (kvb_sem_wait(&ctx.done_sem, KVB_TEST_TIMEOUT_TICKS) != KVB_OK) {
        (void)kvb_thread_delete(&controller);
        (void)kvb_thread_delete(&waiter);
        (void)kvb_platform_irq_probe_disable();
        (void)kvb_sem_delete(&ctx.done_sem);
        (void)kvb_sem_delete(&ctx.ack_sem);
        (void)kvb_sem_delete(&ctx.wake_sem);
        (void)kvb_sem_delete(&ctx.ready_sem);
        result->state = KVB_RESULT_FAIL;
        result->message = "IRQ responsiveness test did not finish";
        return;
    }

    (void)kvb_thread_delete(&controller);
    (void)kvb_thread_delete(&waiter);
    (void)kvb_platform_irq_probe_disable();
    (void)kvb_sem_delete(&ctx.done_sem);
    (void)kvb_sem_delete(&ctx.ack_sem);
    (void)kvb_sem_delete(&ctx.wake_sem);
    (void)kvb_sem_delete(&ctx.ready_sem);

    if (ctx.error_count != 0u ||
        ctx.isr_error_count != 0u ||
        ctx.trigger_to_thread_stats.samples != ctx.iterations) {
        result->state = KVB_RESULT_FAIL;
        result->message = "IRQ responsiveness sequence failed";
        kvb_result_add_metric(result, "irq_probe_supported", 1, "bool");
        kvb_result_add_metric(result, "errors", (int64_t)ctx.error_count, "count");
        kvb_result_add_metric(result, "isr_errors", (int64_t)ctx.isr_error_count, "count");
        kvb_result_add_metric(result, "samples", (int64_t)ctx.trigger_to_thread_stats.samples, "samples");
        return;
    }

    kvb_result_add_metric(result, "irq_probe_supported", 1, "bool");
    kvb_result_add_metric(result, "iterations", (int64_t)ctx.trigger_to_thread_stats.samples, "samples");
    kvb_result_add_metric(result, "errors", (int64_t)ctx.error_count, "count");
    kvb_result_add_metric(result, "isr_errors", (int64_t)ctx.isr_error_count, "count");
    kvb_rt_add_named_latency_metrics(result,
                                     "avg_trigger_to_irq_cycles",
                                     "max_trigger_to_irq_cycles",
                                     "max_trigger_to_irq_ns",
                                     &ctx.trigger_to_irq_stats,
                                     hz);
    kvb_rt_add_named_latency_metrics(result,
                                     "avg_irq_to_thread_cycles",
                                     "max_irq_to_thread_cycles",
                                     "max_irq_to_thread_ns",
                                     &ctx.irq_to_thread_stats,
                                     hz);
    kvb_rt_add_named_latency_metrics(result,
                                     "avg_trigger_to_thread_cycles",
                                     "max_trigger_to_thread_cycles",
                                     "max_trigger_to_thread_ns",
                                     &ctx.trigger_to_thread_stats,
                                     hz);

    result->state = KVB_RESULT_PASS;
    result->message = "IRQ trigger-to-thread responsiveness measured";
}

const KvbTestCase kvb_test_rt_sem_wake_001 = {
    "RT_SEM_WAKE_001",
    "semaphore wake-to-run latency",
    "RT",
    kvb_run_rt_sem_wake
};

const KvbTestCase kvb_test_rt_queue_wake_001 = {
    "RT_QUEUE_WAKE_001",
    "queue receiver wake-to-run latency",
    "RT",
    kvb_run_rt_queue_wake
};

const KvbTestCase kvb_test_rt_mutex_wake_001 = {
    "RT_MUTEX_WAKE_001",
    "mutex waiter wake-to-run latency",
    "RT",
    kvb_run_rt_mutex_wake
};

const KvbTestCase kvb_test_rt_tick_jitter_001 = {
    "RT_TICK_JITTER_001",
    "sleep wake target jitter",
    "RT",
    kvb_run_rt_tick_jitter
};

const KvbTestCase kvb_test_rt_irq_mask_001 = {
    "RT_IRQ_MASK_001",
    "IRQ response and ISR-to-thread latency",
    "RT",
    kvb_run_rt_irq_mask
};

#else

static void kvb_run_rt_disabled_by_target(KvbTestResult *result)
{
    result->state = KVB_RESULT_UNSUPPORTED;
    result->message = "RT tests disabled by target config";
    kvb_result_add_metric(result, "rt_tests_enabled", 0, "bool");
    kvb_result_add_metric(result, "cycle_hz", (int64_t)kvb_platform_cycle_frequency_hz(), "Hz");
}

const KvbTestCase kvb_test_rt_sem_wake_001 = {
    "RT_SEM_WAKE_001",
    "semaphore wake-to-run latency",
    "RT",
    kvb_run_rt_disabled_by_target
};

const KvbTestCase kvb_test_rt_queue_wake_001 = {
    "RT_QUEUE_WAKE_001",
    "queue receiver wake-to-run latency",
    "RT",
    kvb_run_rt_disabled_by_target
};

const KvbTestCase kvb_test_rt_mutex_wake_001 = {
    "RT_MUTEX_WAKE_001",
    "mutex waiter wake-to-run latency",
    "RT",
    kvb_run_rt_disabled_by_target
};

const KvbTestCase kvb_test_rt_tick_jitter_001 = {
    "RT_TICK_JITTER_001",
    "sleep wake target jitter",
    "RT",
    kvb_run_rt_disabled_by_target
};

const KvbTestCase kvb_test_rt_irq_mask_001 = {
    "RT_IRQ_MASK_001",
    "IRQ response and ISR-to-thread latency",
    "RT",
    kvb_run_rt_disabled_by_target
};

#endif

