#include <stdint.h>
#include <string.h>

#include "kvb.h"
#include "../kvb_test_helpers.h"

typedef struct {
    uint32_t seq;
    uint32_t a;
    uint32_t b;
    uint32_t c;
} KvbQueueMsg;

static void kvb_run_queue_fast(KvbTestResult *result)
{
    KvbQueue queue;
    static KVB_QUEUE_STORAGE_DEFINE(storage, sizeof(KvbQueueMsg), KVB_QUEUE_DEPTH);
    KvbQueueMsg tx;
    KvbQueueMsg rx;
    uint64_t start;
    uint64_t now = 0u;
    uint64_t elapsed;
    uint64_t target_us = (uint64_t)KVB_MEASUREMENT_MS * 1000ull;
    uint32_t ops = 0u;

    KVB_ASSERT_STATUS_OK(kvb_queue_create(&queue, KVB_QUEUE_STORAGE_MEM(storage), sizeof(KvbQueueMsg), KVB_QUEUE_DEPTH),
                         "queue create failed");

    memset(&tx, 0, sizeof(tx));
    memset(&rx, 0, sizeof(rx));

    start = kvb_test_time_now_us();

    for (;;) {
        uint32_t i;

        for (i = 0u; i < KVB_THROUGHPUT_BATCH; ++i) {
            tx.seq = ops + i;
            tx.a = 0x12345678u ^ (ops + i);
            tx.b = 0xABCDEF00u + (ops + i);
            tx.c = 0x55AA55AAu;

            if (kvb_queue_send(&queue, &tx, KVB_NO_WAIT) != KVB_OK) {
                (void)kvb_queue_delete(&queue);
                result->state = KVB_RESULT_FAIL;
                result->message = "queue send failed";
                return;
            }

            if (kvb_queue_recv(&queue, &rx, KVB_NO_WAIT) != KVB_OK) {
                (void)kvb_queue_delete(&queue);
                result->state = KVB_RESULT_FAIL;
                result->message = "queue receive failed";
                return;
            }

            if (memcmp(&tx, &rx, sizeof(tx)) != 0) {
                (void)kvb_queue_delete(&queue);
                result->state = KVB_RESULT_FAIL;
                result->message = "queue payload mismatch";
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
        (void)kvb_queue_delete(&queue);
        result->state = KVB_RESULT_INVALID;
        result->message = "platform microsecond source unavailable";
        return;
    }

    kvb_result_add_metric(result, "operations", (int64_t)ops, "send_recv_pairs");
    kvb_result_add_metric(result, "elapsed_us", (int64_t)elapsed, "us");
    kvb_result_add_metric(result, "throughput",
                          (int64_t)((uint64_t)ops * 1000000ull / elapsed),
                          "pairs_per_sec");

    (void)kvb_queue_delete(&queue);

    result->state = KVB_RESULT_PASS;
    result->message = "queue fast path preserves payload";
}

const KvbTestCase kvb_test_ipc_queue_fast_001 = {
    "IPC_QUEUE_FAST_001",
    "queue send/receive same thread",
    "IPC",
    kvb_run_queue_fast
};
