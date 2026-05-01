#ifndef KVB_RUNNER_H
#define KVB_RUNNER_H

#include "kvb_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*KvbTestFunction)(KvbTestResult *result);

typedef struct {
    const char *id;
    const char *name;
    const char *group;
    KvbTestFunction run;
} KvbTestCase;

void kvb_runner_main(void *arg);
int kvb_run_all_tests(void);
void kvb_register_default_tests(void);

void kvb_result_init(KvbTestResult *result, const KvbTestCase *test);
void kvb_result_add_metric(KvbTestResult *result, const char *name, int64_t value, const char *unit);

void kvb_log_line(const char *line);
void kvb_logf(const char *fmt, ...);

void kvb_register_test(const KvbTestCase *test);
uint32_t kvb_registered_test_count(void);
const KvbTestCase *kvb_registered_test(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* KVB_RUNNER_H */
