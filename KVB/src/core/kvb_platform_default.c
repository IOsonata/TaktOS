#include <stddef.h>
#include <stdint.h>

#include "kvb_config.h"
#include "kvb_platform_port.h"

#if defined(KVB_PLATFORM_USE_STDIO) && (KVB_PLATFORM_USE_STDIO != 0)
#include <stdio.h>
#endif

#if defined(__GNUC__)
#define KVB_WEAK __attribute__((weak))
#else
#define KVB_WEAK
#endif

KVB_WEAK uint64_t kvb_platform_cycle_count(void)
{
    return 0u;
}

KVB_WEAK uint32_t kvb_platform_cycle_count32(void)
{
    return (uint32_t)kvb_platform_cycle_count();
}

KVB_WEAK uint32_t kvb_platform_cycle_frequency_hz(void)
{
    return KVB_CORE_CLOCK_HZ;
}

/* Default microsecond source: derived from the cycle counter.  Returns 0
   when no cycle source is available; tests detect that and report
   KVB_RESULT_INVALID rather than producing meaningless throughput numbers. */
KVB_WEAK uint64_t kvb_platform_time_us(void)
{
    uint32_t hz = kvb_platform_cycle_frequency_hz();
    uint64_t cycles = kvb_platform_cycle_count();

    if (hz == 0u) {
        return 0u;
    }

    return (cycles * 1000000ull) / (uint64_t)hz;
}

KVB_WEAK void kvb_platform_log_write(const char *data, size_t len)
{
#if defined(KVB_PLATFORM_USE_STDIO) && (KVB_PLATFORM_USE_STDIO != 0)
    if (data != 0 && len != 0u) {
        (void)fwrite(data, 1u, len, stdout);
    }
#else
    (void)data;
    (void)len;
#endif
}

KVB_WEAK void kvb_platform_log_flush(void)
{
#if defined(KVB_PLATFORM_USE_STDIO) && (KVB_PLATFORM_USE_STDIO != 0)
    fflush(stdout);
#endif
}

KVB_WEAK const char *kvb_platform_board_name(void)
{
    return "unknown";
}

KVB_WEAK const char *kvb_platform_cpu_name(void)
{
    return "unknown";
}

KVB_WEAK const char *kvb_platform_compiler_name(void)
{
#if defined(__clang__)
    return "clang";
#elif defined(__GNUC__)
    return "gcc";
#else
    return "unknown";
#endif
}

KVB_WEAK const char *kvb_platform_compiler_version(void)
{
#if defined(__VERSION__)
    return __VERSION__;
#else
    return "unknown";
#endif
}

KVB_WEAK const char *kvb_platform_build_optimization(void)
{
#if defined(__OPTIMIZE_SIZE__)
    return "size";
#elif defined(__OPTIMIZE__)
    return "speed";
#else
    return "none";
#endif
}

KVB_WEAK const char *kvb_platform_build_fpu(void)
{
#if (defined(__ARM_FP) && (__ARM_FP != 0)) || defined(__riscv_flen)
    return "on";
#else
    return "off";
#endif
}

KVB_WEAK const char *kvb_platform_build_float_abi(void)
{
#if defined(__ARM_FP) && (__ARM_FP != 0)
#  if defined(__SOFTFP__)
    return "softfp";
#  else
    return "hard";
#  endif
#else
    return "soft";
#endif
}

KVB_WEAK const char *kvb_platform_timing_source(void)
{
    return "port-default";
}

KVB_WEAK const char *kvb_platform_safety_profile(void)
{
    return "port-defined";
}

KVB_WEAK const char *kvb_platform_heap_profile(void)
{
    return "port-defined";
}


KVB_WEAK KvbStatus kvb_platform_irq_probe_init(KvbPlatformIrqHandler handler, void *arg)
{
    (void)handler;
    (void)arg;
    return KVB_ERR_UNSUPPORTED;
}

KVB_WEAK KvbStatus kvb_platform_irq_probe_trigger(void)
{
    return KVB_ERR_UNSUPPORTED;
}

KVB_WEAK KvbStatus kvb_platform_irq_probe_disable(void)
{
    return KVB_ERR_UNSUPPORTED;
}

KVB_WEAK const char *kvb_platform_irq_probe_name(void)
{
    return "none";
}
