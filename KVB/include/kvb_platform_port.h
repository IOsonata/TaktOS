#ifndef KVB_PLATFORM_PORT_H
#define KVB_PLATFORM_PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t kvb_platform_cycle_count(void);
uint32_t kvb_platform_cycle_frequency_hz(void);
uint64_t kvb_platform_time_us(void);

void kvb_platform_log_write(const char *data, size_t len);
void kvb_platform_log_flush(void);

const char *kvb_platform_board_name(void);
const char *kvb_platform_cpu_name(void);
const char *kvb_platform_compiler_name(void);
const char *kvb_platform_compiler_version(void);
const char *kvb_platform_build_optimization(void);
const char *kvb_platform_build_fpu(void);
const char *kvb_platform_build_float_abi(void);
const char *kvb_platform_timing_source(void);
const char *kvb_platform_safety_profile(void);
const char *kvb_platform_heap_profile(void);

#ifdef __cplusplus
}
#endif

#endif /* KVB_PLATFORM_PORT_H */
