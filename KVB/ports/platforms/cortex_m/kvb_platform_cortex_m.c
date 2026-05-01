#include <stdint.h>
#include <stddef.h>

#include "kvb_config.h"
#include "kvb_platform_port.h"

/*
 * Cortex-M cycle/time source
 * --------------------------
 * DWT->CYCCNT is not present on every Cortex-M profile.
 *
 * Present on the usual ARMv7-M / ARMv7E-M / ARMv8-M Mainline cores:
 *   Cortex-M3, Cortex-M4, Cortex-M7, Cortex-M33, Cortex-M55, etc.
 *
 * Not present on ARMv6-M baseline cores:
 *   Cortex-M0, Cortex-M0+  (for example STM32F03xx)
 *
 * Therefore this file must never unconditionally read 0xE0001004.  If no
 * DWT cycle counter is available, kvb_platform_cycle_count() returns 0 and
 * kvb_platform_time_us() falls back to kvb_platform_cortex_m_fallback_time_us().
 * Boards without DWT should provide a strong implementation of that fallback
 * using a hardware timer or kernel-independent free-running microsecond timer.
 */

#ifndef KVB_CORTEX_M_HAS_DWT
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)
/* ARMv6-M (M0/M0+) and ARMv8-M Baseline (M23) — DWT/CYCCNT is either
   absent or implementation-defined optional. Treat as no-DWT. */
#define KVB_CORTEX_M_HAS_DWT 0
#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
      defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8_1M_MAIN__)
#define KVB_CORTEX_M_HAS_DWT 1
#elif defined(__CORTEX_M) && (__CORTEX_M != 23) && (__CORTEX_M >= 3)
/* Fallback for toolchains that don't define __ARM_ARCH_*M__. Exclude M23
   explicitly since CMSIS sets __CORTEX_M=23 but Baseline lacks DWT. */
#define KVB_CORTEX_M_HAS_DWT 1
#else
#define KVB_CORTEX_M_HAS_DWT 0
#endif
#endif

#ifndef KVB_CORTEX_M_ENABLE_DWT
#define KVB_CORTEX_M_ENABLE_DWT KVB_CORTEX_M_HAS_DWT
#endif

#if (KVB_CORTEX_M_ENABLE_DWT != 0) && (KVB_CORTEX_M_HAS_DWT == 0)
#error "KVB_CORTEX_M_ENABLE_DWT=1 but this target is configured as no-DWT. ARMv6-M/M0/M0+ targets such as STM32F03xx must use a timer fallback."
#endif

#if (KVB_CORTEX_M_ENABLE_DWT != 0)
#define KVB_DEMCR      (*(volatile uint32_t *)0xE000EDFCu)
#define KVB_DWT_CTRL   (*(volatile uint32_t *)0xE0001000u)
#define KVB_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004u)

#define KVB_DEMCR_TRCENA        (1u << 24)
#define KVB_DWT_CTRL_CYCCNTENA  (1u << 0)
#endif

#if (KVB_CORTEX_M_ENABLE_DWT != 0)
/* DWT cycle counter state.  These are read and written only by
   kvb_platform_cycle_count() and kvb_cortex_m_cycle_init(), both of which
   assume a single-threaded reader.  Do NOT call kvb_platform_cycle_count()
   from an ISR or from multiple threads concurrently — the 32->64 bit
   wraparound extension below is not atomic and would corrupt g_cycle_high
   under a race. */
static uint32_t g_last_cycle32;
static uint64_t g_cycle_high;
static int      g_cycle_initialized;

static void kvb_cortex_m_cycle_init(void)
{
    if (!g_cycle_initialized) {
        KVB_DEMCR |= KVB_DEMCR_TRCENA;
        KVB_DWT_CYCCNT = 0u;
        KVB_DWT_CTRL |= KVB_DWT_CTRL_CYCCNTENA;
        g_last_cycle32 = 0u;
        g_cycle_high = 0u;
        g_cycle_initialized = 1;
    }
}
#endif

#if defined(__GNUC__)
#define KVB_WEAK __attribute__((weak))
#else
#define KVB_WEAK
#endif

/* No-DWT fallback hook.  Boards without DWT/CYCCNT (ARMv6-M, ARMv8-M
   Baseline, or any target where DWT is disabled at build time) override
   this with a strong implementation that returns a monotonic microsecond
   timestamp from a hardware timer or other kernel-independent source.

   Returning 0 from the default causes timing-dependent KVB tests to report
   KVB_RESULT_INVALID rather than publishing meaningless throughput values. */
KVB_WEAK uint64_t kvb_platform_cortex_m_fallback_time_us(void)
{
    return 0u;
}

uint64_t kvb_platform_cycle_count(void)
{
#if (KVB_CORTEX_M_ENABLE_DWT != 0)
    uint32_t now32;

    kvb_cortex_m_cycle_init();

    now32 = KVB_DWT_CYCCNT;

    if (now32 < g_last_cycle32) {
        g_cycle_high += (1ull << 32);
    }

    g_last_cycle32 = now32;
    return g_cycle_high | (uint64_t)now32;
#else
    return 0u;
#endif
}

uint32_t kvb_platform_cycle_frequency_hz(void)
{
#if (KVB_CORTEX_M_ENABLE_DWT != 0)
    return KVB_CORE_CLOCK_HZ;
#else
    return 0u;
#endif
}

uint64_t kvb_platform_time_us(void)
{
#if (KVB_CORTEX_M_ENABLE_DWT != 0)
    uint64_t cycles = kvb_platform_cycle_count();
    uint32_t hz = kvb_platform_cycle_frequency_hz();

    if (hz == 0u) {
        return 0u;
    }

    return (cycles * 1000000ull) / (uint64_t)hz;
#else
    return kvb_platform_cortex_m_fallback_time_us();
#endif
}
