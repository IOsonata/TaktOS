#ifndef KVB_RESULT_H
#define KVB_RESULT_H

#include <stdint.h>
#include "kvb_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KVB_RESULT_PASS = 0,
    KVB_RESULT_FAIL,
    KVB_RESULT_UNSUPPORTED,
    KVB_RESULT_SKIPPED,
    KVB_RESULT_INVALID
} KvbResultState;

typedef struct {
    const char *name;
    int64_t value;
    const char *unit;
} KvbMetric;

typedef struct {
    const char *test_id;
    const char *test_name;
    const char *test_group;
    KvbResultState state;
    const char *message;
    KvbMetric metrics[KVB_MAX_METRICS_PER_TEST];
    uint32_t metric_count;
} KvbTestResult;

const char *kvb_result_state_name(KvbResultState state);

#ifdef __cplusplus
}
#endif

#endif /* KVB_RESULT_H */
