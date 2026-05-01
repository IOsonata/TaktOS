/* SPDX-License-Identifier: MIT
 *
 * KVB build configuration for nRF54L15-DK + Zephyr.
 *
 * Force-included into every TU via the CMakeLists.txt -include flag so
 * the per-target overrides land before kvb_config.h's defaults.
 *
 * Memory budget on this target is very generous:
 *
 *   nRF54L15  =  256 KB SRAM, 1.5 MB Flash
 *
 * Unlike the STM32F0308 target (8 KB SRAM), there is no need to trim
 * worker stacks or pool sizes for memory.  Stack sizes are chosen for
 * comfortable margin under -O0 debug builds; release builds run with
 * the same settings to keep results comparable.
 *
 * Equivalent feature set to the other KVB ports:
 *
 *   - Worker stack 1024 B usable (matches the Beningo 2024 reference
 *     board configuration on faster M4/M33 targets, and the worker
 *     stack ThreadMetric uses on this same nRF54L15-DK in TaktOS's
 *     Thread-Metric ports).
 *   - 10-second measurement window (matches the v2.0 KVB report for
 *     STM32F0308 — keeps tick-edge slop ~0.04% of the window).
 *   - 5 worker threads (matches kvb_config.h default; STM32F0308 uses
 *     3 due to SRAM constraint, but on nRF54L15 there is no reason to
 *     deviate from the framework default).
 */
#ifndef KVB_CONFIG_NRF54L15DK_ZEPHYR_H
#define KVB_CONFIG_NRF54L15DK_ZEPHYR_H

/* ----- CPU / tick rate ---------------------------------------------- */

#define KVB_CORE_CLOCK_HZ       128000000u   /* nRF54L15 cpuapp at 128 MHz */
#define KVB_TICK_HZ             1000u

/* ----- Measurement windows ------------------------------------------ */

#define KVB_MEASUREMENT_MS      10000u       /* 10 s, parity with v2.0 KVB report */
#define KVB_WARMUP_MS           100u

/* ----- Thread / runner stacks --------------------------------------- *
 * These are USABLE stack sizes.  The Zephyr port maps them directly
 * onto K_THREAD_STACK_DEFINE(name, size), and Zephyr internally adds
 * its own per-thread overhead (TLS, MPU guard region) outside the
 * usable area.  No KVB_THREAD_BUF_SIZE round-up applies here — the
 * caller-provided block is purely usable stack. */

#define KVB_RUNNER_STACK_SIZE   2048u   /* matches CONFIG_MAIN_STACK_SIZE in prj.conf */
#define KVB_DEFAULT_STACK_SIZE  1024u   /* per-worker usable stack */

/* ----- Worker count ------------------------------------------------- */

#define KVB_WORKER_THREAD_COUNT 5u

/* ----- Queue / log sizing ------------------------------------------- */

#define KVB_QUEUE_DEPTH         16u
#define KVB_QUEUE_MESSAGE_SIZE  16u
#define KVB_LOG_BUFFER_SIZE     256u

/* ----- Throughput tuning -------------------------------------------- */

#define KVB_MAX_REGISTERED_TESTS 16u
#define KVB_THROUGHPUT_BATCH    1024u   /* parity with kvb_config.h default */

/* ----- Test selection ----------------------------------------------- */

/* TIME_SLEEP_001 strict-minimum mode:
 *
 *   0 = permissive (sleep may wake on the (N-1)th tick boundary if
 *       the request was issued mid-tick); matches Zephyr documented
 *       k_sleep behavior of "at least N ticks but may be less when
 *       the call lands very close to a tick boundary"
 *   1 = strict (require >= N tick periods of wall-clock time)
 *
 * Set to 0 here for cross-port consistency with the published v2.0
 * KVB report on STM32F0308 (which also uses the permissive setting). */
#define KVB_SLEEP_TEST_STRICT_MINIMUM  0

#endif /* KVB_CONFIG_NRF54L15DK_ZEPHYR_H */
