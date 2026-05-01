#include "kvb_runner.h"

extern const KvbTestCase kvb_test_sched_coop_001;
extern const KvbTestCase kvb_test_sync_sem_fast_001;
extern const KvbTestCase kvb_test_sync_mutex_fast_001;
extern const KvbTestCase kvb_test_sync_mutex_pcp_fast_001;
extern const KvbTestCase kvb_test_sync_mutex_ownership_001;
extern const KvbTestCase kvb_test_ipc_queue_fast_001;
extern const KvbTestCase kvb_test_time_sleep_001;

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
}
