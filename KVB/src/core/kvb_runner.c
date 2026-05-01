#include <stdint.h>
#include <string.h>

#include "kvb.h"

static uint32_t g_pass_count;
static uint32_t g_fail_count;
static uint32_t g_unsupported_count;
static uint32_t g_skipped_count;
static uint32_t g_invalid_count;

static void kvb_log_metadata(void)
{
    const KvbKernelFeatures *f = kvb_kernel_features();

    kvb_logf("[KVB] VERSION %s", KVB_VERSION_STRING);

    if (f != 0) {
        kvb_logf("[KVB] KERNEL name=%s version=%s", f->kernel_name, f->kernel_version);
        kvb_logf("[KVB] FEATURES dyn_threads=%u sem=%u mutex=%u mutex_pi=%u mutex_pp=%u queue=%u timer=%u stack_guard=%u null_check=%u invalid_obj=%u",
                 (unsigned)f->supports_dynamic_threads,
                 (unsigned)f->supports_semaphore,
                 (unsigned)f->supports_mutex,
                 (unsigned)f->supports_mutex_priority_inheritance,
                 (unsigned)f->supports_mutex_priority_protect,
                 (unsigned)f->supports_queue,
                 (unsigned)f->supports_timer,
                 (unsigned)f->supports_stack_overflow_detection,
                 (unsigned)f->validates_null_args,
                 (unsigned)f->validates_invalid_objects);
    }

    kvb_logf("[KVB] PLATFORM board=%s cpu=%s clock_hz=%u cycle_hz=%u tick_hz=%u compiler=%s version=\"%s\"",
             kvb_platform_board_name(),
             kvb_platform_cpu_name(),
             (unsigned)KVB_CORE_CLOCK_HZ,
             (unsigned)kvb_platform_cycle_frequency_hz(),
             (unsigned)KVB_TICK_HZ,
             kvb_platform_compiler_name(),
             kvb_platform_compiler_version());

    kvb_logf("[KVB] CONFIG measurement_ms=%u warmup_ms=%u throughput_batch=%u workers=%u queue_depth=%u queue_msg_size=%u stack_size=%u runner_stack_size=%u",
             (unsigned)KVB_MEASUREMENT_MS,
             (unsigned)KVB_WARMUP_MS,
             (unsigned)KVB_THROUGHPUT_BATCH,
             (unsigned)KVB_WORKER_THREAD_COUNT,
             (unsigned)KVB_QUEUE_DEPTH,
             (unsigned)KVB_QUEUE_MESSAGE_SIZE,
             (unsigned)KVB_DEFAULT_STACK_SIZE,
             (unsigned)KVB_RUNNER_STACK_SIZE);

    kvb_logf("[KVB] BUILD opt=%s fpu=%s float_abi=%s timing=%s safety=\"%s\" heap=%s",
             kvb_platform_build_optimization(),
             kvb_platform_build_fpu(),
             kvb_platform_build_float_abi(),
             kvb_platform_timing_source(),
             kvb_platform_safety_profile(),
             kvb_platform_heap_profile());
}

void kvb_result_init(KvbTestResult *result, const KvbTestCase *test)
{
    if (result == 0 || test == 0) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->test_id = test->id;
    result->test_name = test->name;
    result->test_group = test->group;
    result->state = KVB_RESULT_PASS;
    result->message = "ok";
}

void kvb_result_add_metric(KvbTestResult *result, const char *name, int64_t value, const char *unit)
{
    uint32_t i;

    if (result == 0 || name == 0) {
        return;
    }

    i = result->metric_count;
    if (i >= KVB_MAX_METRICS_PER_TEST) {
        return;
    }

    result->metrics[i].name = name;
    result->metrics[i].value = value;
    result->metrics[i].unit = unit;
    result->metric_count = i + 1u;
}

static void kvb_count_result(KvbResultState state)
{
    switch (state) {
    case KVB_RESULT_PASS:
        ++g_pass_count;
        break;
    case KVB_RESULT_FAIL:
        ++g_fail_count;
        break;
    case KVB_RESULT_UNSUPPORTED:
        ++g_unsupported_count;
        break;
    case KVB_RESULT_SKIPPED:
        ++g_skipped_count;
        break;
    case KVB_RESULT_INVALID:
    default:
        ++g_invalid_count;
        break;
    }
}

static void kvb_log_test_result(const KvbTestResult *result)
{
    uint32_t i;

    if (result == 0) {
        return;
    }

    kvb_logf("[KVB] RESULT id=%s group=%s state=%s message=\"%s\"",
             result->test_id,
             result->test_group,
             kvb_result_state_name(result->state),
             (result->message != 0) ? result->message : "");

    for (i = 0; i < result->metric_count; ++i) {
        kvb_logf("[KVB] METRIC id=%s name=%s value=%ld unit=%s",
                 result->test_id,
                 result->metrics[i].name,
                 (long)result->metrics[i].value,
                 (result->metrics[i].unit != 0) ? result->metrics[i].unit : "");
    }

    kvb_logf("[KVB] END id=%s state=%s", result->test_id, kvb_result_state_name(result->state));
}

int kvb_run_all_tests(void)
{
    uint32_t i;
    uint32_t count;

    g_pass_count = 0u;
    g_fail_count = 0u;
    g_unsupported_count = 0u;
    g_skipped_count = 0u;
    g_invalid_count = 0u;

    kvb_register_default_tests();
    kvb_log_line("[KVB] BEGIN RUN");
    kvb_log_metadata();

    count = kvb_registered_test_count();

    for (i = 0; i < count; ++i) {
        const KvbTestCase *test = kvb_registered_test(i);
        KvbTestResult result;

        if (test == 0 || test->run == 0) {
            continue;
        }

        kvb_result_init(&result, test);
        kvb_logf("[KVB] BEGIN id=%s group=%s name=\"%s\"", test->id, test->group, test->name);

        test->run(&result);

        kvb_count_result(result.state);
        kvb_log_test_result(&result);
    }

    kvb_logf("[KVB] SUMMARY pass=%u fail=%u unsupported=%u skipped=%u invalid=%u total=%u",
             (unsigned)g_pass_count,
             (unsigned)g_fail_count,
             (unsigned)g_unsupported_count,
             (unsigned)g_skipped_count,
             (unsigned)g_invalid_count,
             (unsigned)count);

    kvb_log_line("[KVB] END RUN");
    kvb_platform_log_flush();

    return (g_fail_count == 0u && g_invalid_count == 0u) ? 0 : -1;
}

void kvb_runner_main(void *arg)
{
    (void)arg;

    /* Boot-time settle.
     * Sleep ~50 ms before the first log emission so the UART hardware
     * and the IOsonata DMA path are fully past their post-reset
     * transient before the runner starts writing.  main()'s pre-kernel
     * busy-wait is the primary mitigation; this is belt-and-suspenders
     * for the case where main() was not modified, or where the kernel
     * tick driver itself takes a few ticks to stabilise after start.
     * Costs ~50 ms once at boot; never on the measurement path. */
    {
        const uint32_t settle_ticks = (KVB_TICK_HZ >= 20u) ? (KVB_TICK_HZ / 20u) : 1u;
        (void)kvb_thread_sleep_ticks(settle_ticks);
    }

    (void)kvb_run_all_tests();

    for (;;) {
        (void)kvb_thread_sleep_ticks(KVB_TICK_HZ);
    }
}
