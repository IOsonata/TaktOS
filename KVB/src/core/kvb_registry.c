#include <stdint.h>

#include "kvb_runner.h"

#ifndef KVB_MAX_REGISTERED_TESTS
#define KVB_MAX_REGISTERED_TESTS 32u
#endif

static const KvbTestCase *g_tests[KVB_MAX_REGISTERED_TESTS];
static uint32_t g_test_count;

void kvb_register_test(const KvbTestCase *test)
{
    if (test == 0 || test->run == 0) {
        return;
    }

    if (g_test_count < KVB_MAX_REGISTERED_TESTS) {
        g_tests[g_test_count++] = test;
    }
}

uint32_t kvb_registered_test_count(void)
{
    return g_test_count;
}

const KvbTestCase *kvb_registered_test(uint32_t index)
{
    if (index >= g_test_count) {
        return 0;
    }

    return g_tests[index];
}
