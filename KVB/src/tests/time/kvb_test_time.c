#include <stdint.h>

#include "kvb.h"
#include "../kvb_test_helpers.h"

static void kvb_run_sleep_accuracy(KvbTestResult *result)
{
    uint64_t start;
    uint64_t end;
    uint64_t elapsed_us;
    uint64_t tick_us;
    uint64_t nominal_us;
    uint64_t min_expected_us;
    KvbStatus st;

    tick_us = 1000000ull / (uint64_t)KVB_TICK_HZ;
    nominal_us = ((uint64_t)KVB_SLEEP_TEST_TICKS * 1000000ull) / (uint64_t)KVB_TICK_HZ;

#if KVB_SLEEP_TEST_STRICT_MINIMUM
    min_expected_us = nominal_us;
#else
    /* Tick-based sleep APIs usually delay until N tick boundaries have elapsed.
       If the call happens just before the next tick interrupt, the measured
       wall-clock delay for N ticks can legitimately be close to (N - 1) tick
       periods.  Strict N*tick validation is only valid when the test first
       aligns itself to a known tick boundary. */
    if (KVB_SLEEP_TEST_TICKS > 0u) {
        min_expected_us = (((uint64_t)KVB_SLEEP_TEST_TICKS - 1ull) * 1000000ull) / (uint64_t)KVB_TICK_HZ;
    } else {
        min_expected_us = 0u;
    }
#endif

    start = kvb_test_time_now_us();
    st = kvb_thread_sleep_ticks(KVB_SLEEP_TEST_TICKS);
    end = kvb_test_time_now_us();

    KVB_ASSERT_TRUE(st == KVB_OK, "thread sleep failed");

    elapsed_us = kvb_test_elapsed_us(start, end);

    kvb_result_add_metric(result, "requested_ticks", KVB_SLEEP_TEST_TICKS, "ticks");
    kvb_result_add_metric(result, "tick_us", (int64_t)tick_us, "us");
    kvb_result_add_metric(result, "nominal_us", (int64_t)nominal_us, "us");
    kvb_result_add_metric(result, "min_expected_us", (int64_t)min_expected_us, "us");
    kvb_result_add_metric(result, "elapsed_us", (int64_t)elapsed_us, "us");

    if (start == 0u && end == 0u) {
        result->state = KVB_RESULT_INVALID;
        result->message = "platform microsecond source unavailable";
        return;
    }

    if (elapsed_us < min_expected_us) {
        result->state = KVB_RESULT_FAIL;
        result->message = "sleep returned earlier than minimum tick-phase bound";
        return;
    }

    result->state = KVB_RESULT_PASS;
    result->message = "sleep duration valid";
}

const KvbTestCase kvb_test_time_sleep_001 = {
    "TIME_SLEEP_001",
    "thread sleep duration accuracy",
    "TIME",
    kvb_run_sleep_accuracy
};
