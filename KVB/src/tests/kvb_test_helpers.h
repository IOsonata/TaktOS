#ifndef KVB_TEST_HELPERS_H
#define KVB_TEST_HELPERS_H

#include <stdint.h>
#include "kvb.h"

/* Wall-clock microseconds from the platform layer.  KVB does NOT silently
   substitute cycle counts here: that produced mixed-unit results when the
   platform's microsecond source was uninitialised at t=0.  Tests that rely
   on timing must instead bail out with KVB_RESULT_INVALID when they observe
   a zero elapsed window — see the throughput tests for the pattern. */
static inline uint64_t kvb_test_time_now_us(void)
{
    return kvb_platform_time_us();
}

static inline uint64_t kvb_test_elapsed_us(uint64_t start, uint64_t end)
{
    if (end >= start) {
        return end - start;
    }

    return 0u;
}

static inline uint32_t kvb_test_ticks_from_ms(uint32_t ms)
{
    uint64_t ticks = ((uint64_t)ms * (uint64_t)KVB_TICK_HZ + 999ull) / 1000ull;

    if (ticks == 0u) {
        ticks = 1u;
    }

    if (ticks > 0xFFFFFFFFull) {
        ticks = 0xFFFFFFFFu;
    }

    return (uint32_t)ticks;
}

#endif /* KVB_TEST_HELPERS_H */
