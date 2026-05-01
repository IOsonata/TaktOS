/* SPDX-License-Identifier: MIT
 *
 * KVB platform layer for Zephyr — board-agnostic with ARM DWT timing when
 * available.
 *
 * On Cortex-M3/M4/M7/M33/M55 targets, KVB uses DWT CYCCNT as the cycle
 * source.  That keeps nRF52832 measurements on the 64 MHz CPU-cycle counter
 * instead of Zephyr's 32.768 kHz RTC system timer.  Non-DWT targets fall back
 * to Zephyr k_cycle_get_32().
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>

#if defined(CONFIG_ARM) && defined(CONFIG_CPU_CORTEX_M)
#include <cmsis_core.h>
#endif

#include "kvb_platform_port.h"
#include "kvb_config.h"

#if defined(__ARM_ARCH_6M__)
#  define KVB_ARCH_NAME "ARMv6-M"
#elif defined(__ARM_ARCH_7M__)
#  define KVB_ARCH_NAME "ARMv7-M"
#elif defined(__ARM_ARCH_7EM__)
#  define KVB_ARCH_NAME "ARMv7E-M"
#elif defined(__ARM_ARCH_8M_BASE__)
#  define KVB_ARCH_NAME "ARMv8-M Baseline"
#elif defined(__ARM_ARCH_8M_MAIN__)
#  define KVB_ARCH_NAME "ARMv8-M Mainline"
#elif defined(__ARM_ARCH_8_1M_MAIN__)
#  define KVB_ARCH_NAME "ARMv8.1-M Mainline"
#elif defined(__riscv) && (__riscv_xlen == 32)
#  define KVB_ARCH_NAME "RISC-V RV32"
#elif defined(__riscv) && (__riscv_xlen == 64)
#  define KVB_ARCH_NAME "RISC-V RV64"
#elif defined(__x86_64__)
#  define KVB_ARCH_NAME "x86-64"
#elif defined(__i386__)
#  define KVB_ARCH_NAME "x86"
#elif defined(__aarch64__)
#  define KVB_ARCH_NAME "AArch64"
#else
#  define KVB_ARCH_NAME "unknown-arch"
#endif

#if defined(CONFIG_ARM) && defined(CONFIG_CPU_CORTEX_M) && \
    defined(DWT) && defined(CoreDebug) && defined(DWT_CTRL_CYCCNTENA_Msk) && \
    defined(CoreDebug_DEMCR_TRCENA_Msk) && (!defined(__CORTEX_M) || (__CORTEX_M >= 3))
#define KVB_ZEPHYR_USE_DWT 1
#else
#define KVB_ZEPHYR_USE_DWT 0
#endif

#if KVB_ZEPHYR_USE_DWT
extern uint32_t SystemCoreClock;
static uint32_t g_dwt_last;
static uint64_t g_dwt_high;
static uint8_t  g_dwt_ready;

static void kvb_zephyr_dwt_init(void)
{
    if (g_dwt_ready != 0u) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    g_dwt_last = DWT->CYCCNT;
    g_dwt_high = 0u;
    g_dwt_ready = 1u;
}

static uint64_t kvb_zephyr_dwt_count64(void)
{
    uint32_t now;

    kvb_zephyr_dwt_init();
    now = DWT->CYCCNT;

    if (now < g_dwt_last) {
        g_dwt_high += (1ull << 32);
    }

    g_dwt_last = now;
    return g_dwt_high | (uint64_t)now;
}
#endif

uint32_t kvb_platform_cycle_frequency_hz(void)
{
#if KVB_ZEPHYR_USE_DWT
    if (SystemCoreClock != 0u) {
        return SystemCoreClock;
    }
#endif
    return (uint32_t)sys_clock_hw_cycles_per_sec();
}

uint64_t kvb_platform_cycle_count(void)
{
#if KVB_ZEPHYR_USE_DWT
    return kvb_zephyr_dwt_count64();
#else
    return (uint64_t)k_cycle_get_32();
#endif
}

uint64_t kvb_platform_time_us(void)
{
    uint32_t hz = kvb_platform_cycle_frequency_hz();
    uint64_t cycles = kvb_platform_cycle_count();

    if (hz == 0u) {
        return 0u;
    }

    return (cycles * 1000000ull) / (uint64_t)hz;
}

void kvb_platform_log_write(const char *data, size_t len)
{
    if (data == NULL || len == 0u) {
        return;
    }

    printk("%.*s", (int)len, data);
}

void kvb_platform_log_flush(void)
{
}

const char *kvb_platform_board_name(void)
{
    return CONFIG_BOARD;
}

const char *kvb_platform_cpu_name(void)
{
    static char s_cpu_name[64];

    if (s_cpu_name[0] == '\0') {
        const uint32_t hz = kvb_platform_cycle_frequency_hz();
        const unsigned mhz = (unsigned)((hz + 500000u) / 1000000u);

        (void)snprintf(s_cpu_name, sizeof s_cpu_name,
                       "%s @ %u MHz %s (%s)",
                       KVB_ARCH_NAME,
                       mhz,
                       kvb_platform_timing_source(),
                       CONFIG_SOC);
    }

    return s_cpu_name;
}

const char *kvb_platform_timing_source(void)
{
#if KVB_ZEPHYR_USE_DWT
    return "DWT";
#else
    return "Zephyr k_cycle";
#endif
}

const char *kvb_platform_safety_profile(void)
{
#if defined(CONFIG_HW_STACK_PROTECTION) && defined(CONFIG_STACK_SENTINEL) && defined(CONFIG_ASSERT)
    return "stack_guard+sentinel+assert";
#elif defined(CONFIG_STACK_SENTINEL) && defined(CONFIG_ASSERT)
    return "sentinel+assert";
#elif defined(CONFIG_ASSERT)
    return "assert";
#else
    return "minimal";
#endif
}

const char *kvb_platform_heap_profile(void)
{
#if defined(CONFIG_HEAP_MEM_POOL_SIZE) && (CONFIG_HEAP_MEM_POOL_SIZE > 0)
    return "on";
#else
    return "off";
#endif
}
