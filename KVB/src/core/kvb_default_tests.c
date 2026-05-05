#include "kvb_runner.h"

extern const KvbTestCase kvb_test_sched_coop_001;
extern const KvbTestCase kvb_test_sync_sem_fast_001;
extern const KvbTestCase kvb_test_sync_mutex_fast_001;
extern const KvbTestCase kvb_test_sync_mutex_pcp_fast_001;
extern const KvbTestCase kvb_test_sync_mutex_ownership_001;
extern const KvbTestCase kvb_test_ipc_queue_fast_001;
extern const KvbTestCase kvb_test_time_sleep_001;
extern const KvbTestCase kvb_test_rt_sem_wake_001;
extern const KvbTestCase kvb_test_rt_queue_wake_001;
extern const KvbTestCase kvb_test_rt_mutex_wake_001;
extern const KvbTestCase kvb_test_rt_tick_jitter_001;
extern const KvbTestCase kvb_test_rt_irq_mask_001;
#if defined(KVB_ENABLE_TAKT_WAKE_TRACE_TEST) && (KVB_ENABLE_TAKT_WAKE_TRACE_TEST != 0)
extern const KvbTestCase kvb_test_rt_takt_wake_trace_001;
#endif

void kvb_register_default_tests(void)
{
    static int registered;

    if (registered) {
        return;
    }

    registered = 1;

    kvb_register_test(&kvb_test_sched_coop_001);
    kvb_register_test(&kvb_test_sync_sem_fast_001);
    kvb_register_test(&kvb_test_sync_mutex_fast_001);
    kvb_register_test(&kvb_test_sync_mutex_pcp_fast_001);
    kvb_register_test(&kvb_test_sync_mutex_ownership_001);
    kvb_register_test(&kvb_test_ipc_queue_fast_001);
    kvb_register_test(&kvb_test_time_sleep_001);
    kvb_register_test(&kvb_test_rt_sem_wake_001);
    kvb_register_test(&kvb_test_rt_queue_wake_001);
    kvb_register_test(&kvb_test_rt_mutex_wake_001);
    kvb_register_test(&kvb_test_rt_tick_jitter_001);
    kvb_register_test(&kvb_test_rt_irq_mask_001);
#if defined(KVB_ENABLE_TAKT_WAKE_TRACE_TEST) && (KVB_ENABLE_TAKT_WAKE_TRACE_TEST != 0)
    kvb_register_test(&kvb_test_rt_takt_wake_trace_001);
#endif
}
