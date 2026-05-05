/* SPDX-License-Identifier: MIT
 *
 * KVB platform layer for Zephyr — board-agnostic with ARM DWT timing when
 * available.
 *
 * On Cortex-M3/M4/M7/M33/M55 targets, KVB uses DWT CYCCNT when it is
 * usable at runtime.  That keeps nRF52832 measurements on the 64 MHz
 * CPU-cycle counter instead of Zephyr's low-resolution uptime path.
 * If CYCCNT is unavailable or does not advance at runtime, the port falls
 * back to Zephyr k_cycle_get_64()/k_cycle_get_32() and scales that source
 * to CPU-cycle-equivalent units for comparable KVB output.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/version.h>
#include <zephyr/irq.h>
#if defined(CONFIG_IRQ_OFFLOAD)
#include <zephyr/irq_offload.h>
#endif

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
static uint8_t  g_dwt_usable;
static uint8_t  g_dwt_runtime_reject;

static void kvb_zephyr_dwt_unlock(void)
{
#if defined(DWT_BASE) && \
    (defined(DWT_LAR_KEY) || defined(DWT_LAR_KEY_Msk) || defined(DWT_LAR_PRESENT))
    *((volatile uint32_t *)(DWT_BASE + 0xFB0u)) = 0xC5ACCE55u;
#endif
}

static void kvb_zephyr_dwt_enable(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    kvb_zephyr_dwt_unlock();
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();
}

static uint8_t kvb_zephyr_dwt_advances(void)
{
    uint32_t start;
    uint32_t end;
    volatile uint32_t spin;

    kvb_zephyr_dwt_enable();
    start = DWT->CYCCNT;
    for (spin = 0u; spin < 2048u; ++spin) {
        __NOP();
    }
    __DSB();
    __ISB();
    end = DWT->CYCCNT;

    return (end != start) ? 1u : 0u;
}

static void kvb_zephyr_dwt_reject_runtime(void)
{
    g_dwt_usable = 0u;
    g_dwt_runtime_reject = 1u;
    printk("[KVB] CYCLE DWT runtime stall detected — switched to k_cycle_get fallback\n");
}

static void kvb_zephyr_dwt_init(void)
{
    if (g_dwt_ready != 0u) {
        return;
    }

    kvb_zephyr_dwt_enable();
    DWT->CYCCNT = 0u;
    __DSB();
    __ISB();

    g_dwt_last = DWT->CYCCNT;
    g_dwt_high = 0u;
    g_dwt_runtime_reject = 0u;
    g_dwt_usable = kvb_zephyr_dwt_advances();
    g_dwt_last = DWT->CYCCNT;
    g_dwt_ready = 1u;
}

static uint8_t kvb_zephyr_dwt_runtime_ok(uint32_t *now)
{
    if (now == NULL || g_dwt_usable == 0u || g_dwt_runtime_reject != 0u) {
        return 0u;
    }

    /* A same-value read after a full platform call should not happen on a
     * usable CPU-cycle counter.  nRF54L15 Zephyr can leave CYCCNT present but
     * stopped after idle/sleep transitions.  Detect that condition here and
     * fall back instead of reporting zero-cycle PASS metrics. */
    if (*now == g_dwt_last) {
        if (kvb_zephyr_dwt_advances() == 0u) {
            kvb_zephyr_dwt_reject_runtime();
            return 0u;
        }
        *now = DWT->CYCCNT;
    }

    return 1u;
}

static uint64_t kvb_zephyr_dwt_count64(void)
{
    uint32_t now;

    kvb_zephyr_dwt_init();
    if (g_dwt_usable == 0u || g_dwt_runtime_reject != 0u) {
        return 0u;
    }

    kvb_zephyr_dwt_enable();
    now = DWT->CYCCNT;

    if (kvb_zephyr_dwt_runtime_ok(&now) == 0u) {
        return 0u;
    }

    if (now < g_dwt_last) {
        g_dwt_high += (1ull << 32);
    }

    g_dwt_last = now;
    return g_dwt_high | (uint64_t)now;
}
#endif

static uint64_t kvb_zephyr_scale_to_cpu_cycles(uint64_t hw_cycles)
{
    const uint64_t hw_hz = (uint64_t)sys_clock_hw_cycles_per_sec();
    const uint64_t cpu_hz = (uint64_t)KVB_CORE_CLOCK_HZ;

    if (hw_hz == 0u || cpu_hz == 0u) {
        return hw_cycles;
    }

    if (hw_hz == cpu_hz) {
        return hw_cycles;
    }

    /* Avoid overflow for long-running counters.  KVB measurements are short,
     * but split quotient/remainder scaling keeps the conversion well behaved
     * if the board has been running for a while before the suite starts. */
    return ((hw_cycles / hw_hz) * cpu_hz) +
           (((hw_cycles % hw_hz) * cpu_hz) / hw_hz);
}

uint32_t kvb_platform_cycle_frequency_hz(void)
{
#if KVB_ZEPHYR_USE_DWT
    kvb_zephyr_dwt_init();
    if (g_dwt_usable != 0u && SystemCoreClock != 0u) {
        return SystemCoreClock;
    }
#endif

    /* Zephyr's k_cycle_get_*() can expose a low-frequency platform timer
     * instead of the CPU clock.  Report CPU-cycle-equivalent units so KVB
     * cycle metrics remain comparable with the IOsonata-backed ports. */
    return (uint32_t)KVB_CORE_CLOCK_HZ;
}

uint64_t kvb_platform_cycle_count(void)
{
#if KVB_ZEPHYR_USE_DWT
    kvb_zephyr_dwt_init();
    if (g_dwt_usable != 0u && g_dwt_runtime_reject == 0u) {
        const uint64_t dwt_cycles = kvb_zephyr_dwt_count64();

        if (g_dwt_usable != 0u && g_dwt_runtime_reject == 0u) {
            return dwt_cycles;
        }
    }
#endif
    return kvb_zephyr_scale_to_cpu_cycles(k_cycle_get_64());
}

uint32_t kvb_platform_cycle_count32(void)
{
    /* Route the 32-bit API through the same source-selection logic as the
     * 64-bit API.  Do not read DWT->CYCCNT directly here: if Zephyr leaves
     * CYCCNT stopped, direct reads produce zero IRQ-latency metrics while the
     * 64-bit path has already rejected DWT and selected the fallback source. */
    return (uint32_t)kvb_platform_cycle_count();
}

uint64_t kvb_platform_time_us(void)
{
    /* Use Zephyr uptime for wall-clock measurements.
     *
     * The DWT CYCCNT path is useful for active-loop cycle counting, but it is
     * not a stable wall-clock source across k_sleep()/idle on all Cortex-M33
     * targets.  Zephyr uptime is driven by the kernel timer and is the correct
     * source for sleep-duration validation.
     */
#if KVB_ZEPHYR_USE_DWT
    if (g_dwt_usable != 0u && g_dwt_runtime_reject == 0u) {
        kvb_zephyr_dwt_enable();
    }
#endif
    return (uint64_t)k_ticks_to_us_floor64(k_uptime_ticks());
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
        const uint32_t hz = KVB_CORE_CLOCK_HZ;
        const unsigned mhz = (unsigned)((hz + 500000u) / 1000000u);

        (void)snprintf(s_cpu_name, sizeof s_cpu_name,
                       "%s @ %u MHz (%s)",
                       KVB_ARCH_NAME,
                       mhz,
                       CONFIG_SOC);
    }

    return s_cpu_name;
}

const char *kvb_platform_timing_source(void)
{
#if KVB_ZEPHYR_USE_DWT
    kvb_zephyr_dwt_init();
    if (g_dwt_usable != 0u && g_dwt_runtime_reject == 0u) {
        return "DWT cycles with runtime fallback + Zephyr uptime";
    }
#endif
    if ((uint32_t)sys_clock_hw_cycles_per_sec() != KVB_CORE_CLOCK_HZ) {
        return "Zephyr cycles scaled to CPU clock + uptime";
    }
    return "Zephyr cycles + uptime";
}

const char *kvb_platform_safety_profile(void)
{
    /* In current Zephyr, API-entry parameter validation is split
     * across two mechanisms: CHECKIF() (gated by the "Error checking
     * behavior for CHECK macro" Kconfig choice; KVB pins
     * RUNTIME_ERROR_CHECKS=y) and __ASSERT() (gated by CONFIG_ASSERT;
     * KVB pins CONFIG_ASSERT=y).  Both are required for parity with
     * FreeRTOS configASSERT, ThreadX _txe_*, and TaktOS API-entry
     * checks.  We report a combined tag that lists each layer that
     * is actually compiled in.
     */
#if defined(CONFIG_HW_STACK_PROTECTION) && defined(CONFIG_STACK_SENTINEL) && \
    defined(CONFIG_RUNTIME_ERROR_CHECKS) && defined(CONFIG_ASSERT)
    return "stack_guard+sentinel+runtime_checks+assert";
#elif defined(CONFIG_STACK_SENTINEL) && \
    defined(CONFIG_RUNTIME_ERROR_CHECKS) && defined(CONFIG_ASSERT)
    return "sentinel+runtime_checks+assert";
#elif defined(CONFIG_RUNTIME_ERROR_CHECKS) && defined(CONFIG_ASSERT)
    return "runtime_checks+assert";
#elif defined(CONFIG_RUNTIME_ERROR_CHECKS)
    return "runtime_checks";
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


/* --------------------------------------------------------------------------
 * IRQ responsiveness probe
 * --------------------------------------------------------------------------
 *
 * Do not depend on CONFIG_IRQ_OFFLOAD for the Nordic Zephyr runs.  In current
 * Zephyr/NCS, irq_offload() is a test helper whose Kconfig symbol may be
 * hidden behind test-only dependencies; setting CONFIG_IRQ_OFFLOAD=y in
 * prj.conf can therefore still leave the symbol disabled.  KVB needs a normal
 * target IRQ, so Nordic Cortex-M builds wire one spare NVIC software/event IRQ
 * directly through Zephyr's IRQ_CONNECT path.
 */

#define KVB_ZEPHYR_HW_IRQ_PROBE 0

#if defined(CONFIG_ARM) && defined(CONFIG_CPU_CORTEX_M)
#if defined(CONFIG_SOC_NRF52832) || defined(CONFIG_SOC_NRF52833) || \
    defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_SERIES_NRF52X)
#undef KVB_ZEPHYR_HW_IRQ_PROBE
#define KVB_ZEPHYR_HW_IRQ_PROBE 1
#define KVB_ZEPHYR_IRQ_PROBE_IRQ  SWI3_EGU3_IRQn
#define KVB_ZEPHYR_IRQ_PROBE_NAME "NVIC SWI3_EGU3"
#elif defined(CONFIG_SOC_NRF54L15_CPUAPP) || defined(CONFIG_SOC_NRF54L15) || \
      defined(CONFIG_SOC_SERIES_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF54L)
#undef KVB_ZEPHYR_HW_IRQ_PROBE
#define KVB_ZEPHYR_HW_IRQ_PROBE 1
#define KVB_ZEPHYR_IRQ_PROBE_IRQ  28
#define KVB_ZEPHYR_IRQ_PROBE_NAME "NVIC SWI00"
#endif
#endif

#if KVB_ZEPHYR_HW_IRQ_PROBE
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS 3u
#endif
#define KVB_ZEPHYR_IRQ_PROBE_PRIO \
    ((unsigned int)((KVB_IRQ_PROBE_PRIORITY) >> (8u - __NVIC_PRIO_BITS)))

static KvbPlatformIrqHandler g_zephyr_irq_probe_handler;
static void *g_zephyr_irq_probe_arg;
static uint8_t g_zephyr_irq_probe_ready;

static void kvb_zephyr_irq_probe_isr(const void *parameter)
{
    (void)parameter;

    if (g_zephyr_irq_probe_handler != NULL) {
        g_zephyr_irq_probe_handler(g_zephyr_irq_probe_arg);
    }
}

static void kvb_zephyr_irq_probe_barrier(void)
{
    __DSB();
    __ISB();
}
#elif defined(CONFIG_IRQ_OFFLOAD)
static KvbPlatformIrqHandler g_zephyr_irq_probe_handler;
static void *g_zephyr_irq_probe_arg;

static void kvb_zephyr_irq_probe_offload(const void *parameter)
{
    (void)parameter;

    if (g_zephyr_irq_probe_handler != NULL) {
        g_zephyr_irq_probe_handler(g_zephyr_irq_probe_arg);
    }
}
#endif

KvbStatus kvb_platform_irq_probe_init(KvbPlatformIrqHandler handler, void *arg)
{
#if KVB_ZEPHYR_HW_IRQ_PROBE
    IRQ_CONNECT(KVB_ZEPHYR_IRQ_PROBE_IRQ,
                KVB_ZEPHYR_IRQ_PROBE_PRIO,
                kvb_zephyr_irq_probe_isr,
                NULL,
                0);

    if (handler == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    g_zephyr_irq_probe_handler = handler;
    g_zephyr_irq_probe_arg = arg;
    g_zephyr_irq_probe_ready = 1u;

    NVIC_ClearPendingIRQ((IRQn_Type)KVB_ZEPHYR_IRQ_PROBE_IRQ);
    irq_enable((unsigned int)KVB_ZEPHYR_IRQ_PROBE_IRQ);
    kvb_zephyr_irq_probe_barrier();
    return KVB_OK;
#elif defined(CONFIG_IRQ_OFFLOAD)
    if (handler == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    g_zephyr_irq_probe_handler = handler;
    g_zephyr_irq_probe_arg = arg;
    return KVB_OK;
#else
    (void)handler;
    (void)arg;
    return KVB_ERR_UNSUPPORTED;
#endif
}

KvbStatus kvb_platform_irq_probe_trigger(void)
{
#if KVB_ZEPHYR_HW_IRQ_PROBE
    if (g_zephyr_irq_probe_ready == 0u) {
        return KVB_ERR_UNSUPPORTED;
    }

    NVIC_SetPendingIRQ((IRQn_Type)KVB_ZEPHYR_IRQ_PROBE_IRQ);
    kvb_zephyr_irq_probe_barrier();
    return KVB_OK;
#elif defined(CONFIG_IRQ_OFFLOAD)
    if (g_zephyr_irq_probe_handler == NULL) {
        return KVB_ERR_UNSUPPORTED;
    }

    irq_offload(kvb_zephyr_irq_probe_offload, NULL);
    return KVB_OK;
#else
    return KVB_ERR_UNSUPPORTED;
#endif
}

KvbStatus kvb_platform_irq_probe_disable(void)
{
#if KVB_ZEPHYR_HW_IRQ_PROBE
    if (g_zephyr_irq_probe_ready == 0u) {
        return KVB_ERR_UNSUPPORTED;
    }

    irq_disable((unsigned int)KVB_ZEPHYR_IRQ_PROBE_IRQ);
    NVIC_ClearPendingIRQ((IRQn_Type)KVB_ZEPHYR_IRQ_PROBE_IRQ);
    g_zephyr_irq_probe_handler = NULL;
    g_zephyr_irq_probe_arg = NULL;
    g_zephyr_irq_probe_ready = 0u;
    kvb_zephyr_irq_probe_barrier();
    return KVB_OK;
#elif defined(CONFIG_IRQ_OFFLOAD)
    g_zephyr_irq_probe_handler = NULL;
    g_zephyr_irq_probe_arg = NULL;
    return KVB_OK;
#else
    return KVB_ERR_UNSUPPORTED;
#endif
}

const char *kvb_platform_irq_probe_name(void)
{
#if KVB_ZEPHYR_HW_IRQ_PROBE
    return KVB_ZEPHYR_IRQ_PROBE_NAME;
#elif defined(CONFIG_IRQ_OFFLOAD)
    return "Zephyr irq_offload";
#else
    return "none";
#endif
}
