/**-------------------------------------------------------------------------
 * @file    kvb_config_stm32f0308.h
 * @brief   KVB build configuration for STM32F0308-DISCO.
 *
 * Forced into every TU via -include flag in the Eclipse project settings,
 * so its values land before include/kvb_config.h's #ifndef-guarded defaults.
 *
 * Memory budget (8 KB SRAM total):
 *
 *   STM32F030R8 is Cortex-M0 / ARMv6-M with no DWT cycle counter and no
 *   MPU-backed stack guard in this target.  TaktOS therefore uses the
 *   software guard word only:
 *
 *     TAKTOS_STACK_GUARD_ALIGN        = 8
 *     TAKTOS_THREAD_GUARD_REGION_SIZE = 0
 *
 *   The previous 256-byte ARMv6-M guard quantum is only valid for an
 *   explicitly enabled ARMv6-M MPU stack-guard configuration.  It is too
 *   expensive as the default on an 8 KB SRAM MCU.
 *
 *   Runner usable stack       512 B
 *   Worker usable stacks      3 * 256 B
 *   Mutex-owner usable stack  256 B
 *   IOsonata UART TxFIFO      128-byte FIFO storage
 *
 * Runner stack at 512 B usable is tight but sufficient for the current
 * Phase-1 KVB tests.  If longer kvb_logf format strings or deeper tests
 * are added, monitor the TaktOS software guard word.
 * -------------------------------------------------------------------------*/
#ifndef KVB_CONFIG_STM32F0308_H
#define KVB_CONFIG_STM32F0308_H

#define KVB_PORT_TAKTOS         1

#define KVB_CORE_CLOCK_HZ       48000000u
#define KVB_TICK_HZ             1000u
/* 10 s measurement window — matches kvb_config_stm32f0308_freertos.h and
 * kvb_config_stm32f0308_threadx.h.  The earlier 1 s setting was too short:
 * SCHED_COOP_001 reports raw yield counts over the window (not yields/sec),
 * so a TaktOS run at 1 s is not directly comparable to a FreeRTOS/ThreadX
 * run at 10 s.  The four throughput tests auto-normalise to per-second so
 * they were unaffected, but having the same window across all three
 * kernels removes the asymmetry entirely. */
#define KVB_MEASUREMENT_MS      10000u
#define KVB_WARMUP_MS           100u

/* Runner stack: 1024 B usable matches the FreeRTOS variant.  Earlier
 * 512 B sizing was based on a kernel-only argument ("TaktOS sem/queue
 * creation paths are shallow"), which holds for the kernel — but the
 * runner call chain through kvb_logf -> newlib-nano vsnprintf ->
 * kvb_platform_log_write is identical across kernels and pushes
 * ~200-250 B by itself (vsnprintf frame ~180 B + the kvb_log_metadata
 * format-arg frame).  At 512 B the runner overflowed into BSS during
 * kvb_log_metadata and corrupted kernel state, which surfaced later as
 * an unaligned LDM HardFault inside PendSV (next TCB's sp field got
 * stomped).  Same FreeRTOS rationale: 1024 B is the minimum that lets
 * the full kvb_logf chain run to completion without tripping the stack
 * guard.
 *
 * Note: the KVB_LOG_BUFFER_SIZE buffer itself does NOT live on this
 * stack.  It is a single static buffer in BSS (src/core/kvb_log.c,
 * single-caller invariant) — it costs flat BSS bytes, not runner stack
 * peak.  The 1024 B figure here covers vsnprintf and the call chain. */
#define KVB_RUNNER_STACK_SIZE   1024u
#define KVB_DEFAULT_STACK_SIZE  256u
#define KVB_TEST_THREAD_STACK_SIZE   192u
#define KVB_SCHED_WORKER_STACK_SIZE  128u
#define KVB_RT_THREAD_STACK_SIZE     128u
#define KVB_SYNC_THREAD_STACK_SIZE   128u

/* STM32F030R8 has only 8 KB SRAM. Keep the runner at 1024 B because it
 * executes kvb_logf/newlib-nano formatting, but size the helper threads
 * separately. These workers do not format log strings; they block, wake,
 * yield, and touch kernel objects only. The target-local value avoids
 * carrying the larger nRF5x/nRF54 helper-thread budget into this board. */

#define KVB_WORKER_THREAD_COUNT 3u

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u

/* 192 B fits the longest header line (PLATFORM/CONFIG, ~155-165 chars).
 * Lives in BSS as a single static buffer; the +64 B vs the previous
 * 128 B sizing comes out of flat BSS, not the runner stack peak.  On
 * 8 KB SRAM this is 0.8 % of total — fits inside the documented
 * memory budget for all three STM32F0308 kernel variants. */
#define KVB_LOG_BUFFER_SIZE     192u

/* UART TX FIFO size override.  The shared Targets/src/main.cpp uses
 * CFIFO_MEMSIZE(256) by default; we override to 128 to fit Debug builds
 * (rdimon semihosting carries ~70 B of extra .bss vs Release nosys).
 * 128 B holds ~10 ms of TX at 115200 baud, comfortably more than any
 * single kvb_logf line, so the drop has no functional impact on log
 * delivery — the UART class blocks the producer if the FIFO fills. */
#define UARTFIFOSIZE            CFIFO_MEMSIZE(128)

/* Trim the registry from the framework default (32) to 16 — saves
 * 16 * sizeof(void*) = 64 B on M0.  We register 6 tests today;
 * 16 is comfortable headroom.  If you grow to nearly 16, raise this
 * before adding more. */
#define KVB_MAX_REGISTERED_TESTS 16u

/* Smaller throughput batch keeps measurement windows short on M0 — the
 * primitives are slower per-op and batches of 1024 would push the test
 * wall-clock time uncomfortably long.  SysTick-derived timing has CPU-
 * cycle resolution so smaller batches stay accurate. */
#define KVB_THROUGHPUT_BATCH    256u

#define KVB_ENABLE_RT_TESTS          1u
#define KVB_RT_LATENCY_ITERATIONS    64u
#define KVB_RT_JITTER_ITERATIONS     32u

/* STM32F0308 is Cortex-M0, so there is no DWT cycle counter.
 * The STM32F0308 platform port provides a SysTick-derived microsecond
 * timebase, and the RT tests use that fallback with reduced iteration
 * counts and shared helper stacks. This keeps the canonical KVB test set
 * active on the 8 KB SRAM target. */

#endif /* KVB_CONFIG_STM32F0308_H */
