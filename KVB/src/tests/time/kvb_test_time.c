#include <stdint.h>

#include "kvb.h"
#include "../kvb_test_helpers.h"

static void kvb_run_sleep_tick_validity(KvbTestResult *result)
{
    uint64_t start_ticks;
    uint64_t end_ticks;
    uint64_t elapsed_ticks;
    uint64_t tick_us;
    uint64_t nominal_us;
    uint64_t elapsed_us;
    uint64_t min_expected_ticks;
    uint64_t max_expected_ticks;
    KvbStatus st;

    tick_us = 1000000ull / (uint64_t)KVB_TICK_HZ;
    nominal_us = ((uint64_t)KVB_SLEEP_TEST_TICKS * 1000000ull) /
                 (uint64_t)KVB_TICK_HZ;

#if KVB_SLEEP_TEST_STRICT_MINIMUM
    min_expected_ticks = (uint64_t)KVB_SLEEP_TEST_TICKS;
#else
    /* Some kernels define a tick sleep as waiting until N tick boundaries have
       occurred. If the call happens immediately before a tick interrupt, the
       wall-clock duration can be just under N full tick periods. The native
       kernel tick count should still advance by at least N ticks for a normal
       nonzero sleep request, so the default tick-based minimum remains N. */
    min_expected_ticks = (uint64_t)KVB_SLEEP_TEST_TICKS;
#endif

    max_expected_ticks = (uint64_t)KVB_SLEEP_TEST_TICKS +
                         (uint64_t)KVB_SLEEP_TEST_MAX_EXTRA_TICKS;

    start_ticks = kvb_kernel_tick_count();
    st = kvb_thread_sleep_ticks(KVB_SLEEP_TEST_TICKS);
    end_ticks = kvb_kernel_tick_count();

    KVB_ASSERT_TRUE(st == KVB_OK, "thread sleep failed");

    elapsed_ticks = end_ticks - start_ticks;
    elapsed_us = elapsed_ticks * tick_us;

    kvb_result_add_metric(result, "requested_ticks", KVB_SLEEP_TEST_TICKS, "ticks");
    kvb_result_add_metric(result, "elapsed_ticks", (int64_t)elapsed_ticks, "ticks");
    kvb_result_add_metric(result, "min_expected_ticks", (int64_t)min_expected_ticks, "ticks");
    kvb_result_add_metric(result, "max_expected_ticks", (int64_t)max_expected_ticks, "ticks");
    kvb_result_add_metric(result, "tick_us", (int64_t)tick_us, "us");
    kvb_result_add_metric(result, "nominal_us", (int64_t)nominal_us, "us");
    kvb_result_add_metric(result, "elapsed_us", (int64_t)elapsed_us, "us");

    if (elapsed_ticks < min_expected_ticks) {
        result->state = KVB_RESULT_FAIL;
        result->message = "sleep returned before requested tick count";
        return;
    }

    if (elapsed_ticks > max_expected_ticks) {
        result->state = KVB_RESULT_FAIL;
        result->message = "sleep exceeded maximum tick tolerance";
        return;
    }

    result->state = KVB_RESULT_PASS;
    result->message = "sleep tick validity passed";
}

const KvbTestCase kvb_test_time_sleep_001 = {
    "TIME_SLEEP_001",
    "thread sleep tick validity",
    "TIME",
    kvb_run_sleep_tick_validity
};
