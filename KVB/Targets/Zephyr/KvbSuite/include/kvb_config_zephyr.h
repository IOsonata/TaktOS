/* SPDX-License-Identifier: MIT
 *
 * KVB build configuration for the Zephyr target — board-agnostic.
 *
 * Force-included into every TU via the CMakeLists.txt -include flag
 * so the per-target overrides land before kvb_config.h's defaults.
 *
 * These defaults assume a "reasonable" Zephyr board: at least 32 KB
 * SRAM, at least 128 KB flash.  The vast majority of Zephyr-supported
 * boards meet that bar with ample margin.  If you target a tighter
 * device (say, an 8 KB SRAM Cortex-M0+ — possible on Zephyr but
 * uncommon), drop a board-specific override into a per-board file
 * (see boards/README.md) or pass -D values on the cmake command line.
 *
 * Identical safety profile across every kernel in the KVB comparison —
 * see prj.conf for the per-feature parity rationale.
 */
#ifndef KVB_CONFIG_ZEPHYR_H
#define KVB_CONFIG_ZEPHYR_H

/* ----- Tick rate / CPU clock metadata ------------------------------- *
 *
 * KVB_CORE_CLOCK_HZ is the CPU clock printed in the run metadata.  It is
 * not necessarily the same as kvb_platform_cycle_frequency_hz(): Zephyr can
 * expose a platform timer as its cycle source while the CPU runs faster.
 *
 * KVB_TICK_HZ matches CONFIG_SYS_CLOCK_TICKS_PER_SEC in prj.conf.
 * Both must agree — keep them at 1000 unless you change both. */
#if defined(CONFIG_SOC_NRF54L15_CPUAPP) || defined(CONFIG_SOC_NRF54L15) || \
    defined(CONFIG_SOC_SERIES_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF54L)
#define KVB_CORE_CLOCK_HZ       128000000u
#elif defined(CONFIG_SOC_NRF52832) || defined(CONFIG_SOC_NRF52833) || \
      defined(CONFIG_SOC_NRF52840) || defined(CONFIG_SOC_SERIES_NRF52X)
#define KVB_CORE_CLOCK_HZ       64000000u
#endif

#define KVB_TICK_HZ             1000u

/* ----- Measurement windows ------------------------------------------ */

/* 10-second window — same as the v2.0 KVB report on STM32F0308.  Drops
 * tick-edge slop to ~0.04 % of the window.  Each test takes 10 s, so
 * the full 6-test suite takes ~60 s. */
#define KVB_MEASUREMENT_MS      10000u

#define KVB_WARMUP_MS           100u

/* ----- Thread / runner stacks --------------------------------------- *
 *
 * USABLE stack sizes.  The Zephyr port maps these directly onto
 * K_THREAD_STACK_DEFINE(name, size); Zephyr internally adds its own
 * per-thread overhead (TLS, MPU guard region) outside the usable area.
 * No KVB_THREAD_BUF_SIZE round-up applies to Zephyr — the caller-
 * provided block is purely usable stack. */

#define KVB_RUNNER_STACK_SIZE   2048u   /* matches CONFIG_MAIN_STACK_SIZE */
#define KVB_DEFAULT_STACK_SIZE  1024u   /* per-worker usable stack */
#define KVB_TEST_THREAD_STACK_SIZE   KVB_DEFAULT_STACK_SIZE
#define KVB_SCHED_WORKER_STACK_SIZE  KVB_TEST_THREAD_STACK_SIZE
#define KVB_RT_THREAD_STACK_SIZE     KVB_TEST_THREAD_STACK_SIZE
#define KVB_SYNC_THREAD_STACK_SIZE   KVB_TEST_THREAD_STACK_SIZE

/* ----- Worker count ------------------------------------------------- */

#define KVB_WORKER_THREAD_COUNT 3u

/* ----- Queue / log sizing ------------------------------------------- */

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u
#define KVB_LOG_BUFFER_SIZE     256u

/* ----- Throughput tuning -------------------------------------------- */

#define KVB_MAX_REGISTERED_TESTS 16u
#define KVB_THROUGHPUT_BATCH    256u

#define KVB_ENABLE_RT_TESTS          1u

/* ----- Test selection ----------------------------------------------- */

/* TIME_SLEEP_001 strict-minimum mode:
 *
 *   0 = permissive (sleep may wake on the (N-1)th tick boundary if the
 *       request was issued mid-tick); matches Zephyr's documented
 *       k_sleep behavior of "at least N ticks but may be less when the
 *       call lands very close to a tick boundary"
 *   1 = strict (require >= N tick periods of wall-clock time)
 *
 * Set to 0 here for cross-port consistency with the published v2.0
 * KVB report on STM32F0308 (which also uses the permissive setting). */
#define KVB_SLEEP_TEST_STRICT_MINIMUM  0

#endif /* KVB_CONFIG_ZEPHYR_H */
