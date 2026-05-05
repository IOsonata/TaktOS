/**-------------------------------------------------------------------------
 * @file    kvb_config_nrf54l15.h
 * @brief   KVB build configuration for nRF54L15-DK + TaktOS.
 *
 * Forced into every TU via -include flag in the Eclipse project settings,
 * so its values land before include/kvb_config.h's #ifndef-guarded defaults.
 *
 * Memory budget (256 KB SRAM total) — nRF54L15 has roughly 8x the SRAM of
 * STM32F0308, so this is comfortable across all three kernels:
 *
 *   Runner thread       (TCB inline + 1024 B usable stack)   ~= 1.3 KB
 *   Worker stacks (3)   3 * (TCB inline + 512 B usable)      ~= 2.4 KB
 *   Mutex-owner stack   (TCB inline + 512 B usable)          ~= 0.8 KB
 *   IOsonata UART TxFIFO (UARTFIFOSIZE = 256) + state        ~= 0.5 KB
 *   KVB statics + queue/sem/mutex storage                    ~= 0.5 KB
 *   ISR/main MSP + bss headroom                              ~= 1.5 KB
 *   ----------------------------------------------------------------------
 *   Total                                                    ~= 7.0 / 256.0 KB
 *
 * Stacks here are larger than the STM32F0308 build because Cortex-M33 has a
 * 32-bit ARMv8-M context (8 callee-save + 8 hardware-stacked = 16 words
 * per exception entry) versus M0's narrower spill set, AND because newlib
 * vsnprintf paths used by kvb_logf retain ~180 B of frame on M4 -Os.  M0
 * managed in 256 B because its register set was even smaller; M4 needs 512.
 * -------------------------------------------------------------------------*/
#ifndef KVB_CONFIG_NRF54L15_H
#define KVB_CONFIG_NRF54L15_H

#define KVB_PORT_TAKTOS         1

#define KVB_CORE_CLOCK_HZ       128000000u
#define KVB_TICK_HZ             1000u
#define KVB_MEASUREMENT_MS      10000u    /* 10 s window: tick-edge slop drops to ~0.04% */
#define KVB_WARMUP_MS           100u

/* Runner stack at 1024 B usable matches the FreeRTOS and ThreadX variants
 * on the same target.  Plenty of headroom for kvb_logf -> newlib vsnprintf
 * (~400 B frame) plus call-chain overhead.  The KVB_LOG_BUFFER_SIZE buffer
 * itself lives in BSS (single static buffer in src/core/kvb_log.c, single-
 * caller / runner-only) and no longer contributes to runner stack peak. */
#define KVB_RUNNER_STACK_SIZE   1024u
#define KVB_DEFAULT_STACK_SIZE  512u
#define KVB_TEST_THREAD_STACK_SIZE   KVB_DEFAULT_STACK_SIZE
#define KVB_SCHED_WORKER_STACK_SIZE  KVB_TEST_THREAD_STACK_SIZE
#define KVB_RT_THREAD_STACK_SIZE     KVB_TEST_THREAD_STACK_SIZE
#define KVB_SYNC_THREAD_STACK_SIZE   KVB_TEST_THREAD_STACK_SIZE

#define KVB_WORKER_THREAD_COUNT 3u

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u

/* Sized to hold the longest header line emitted at boot.  PLATFORM and
 * CONFIG are ~155-165 chars on this target (board name + cpu name +
 * compiler version + counts + stack sizes), so 192 fits with a small
 * headroom margin.  Lives in BSS as a single static buffer (see comment
 * in src/core/kvb_log.c on single-thread / runner-only invariant). */
#define KVB_LOG_BUFFER_SIZE     192u

/* UART TX FIFO size override.  256 B (vs 128 on STM32F0308) is appropriate
 * on a 256 KB SRAM target — holds ~22 ms of TX at 115200 baud, more than
 * enough headroom for any single kvb_logf line.  Same value across all
 * three nRF54L15 kernel variants. */
#define UARTFIFOSIZE            CFIFO_MEMSIZE(256)

/* Trim from the framework default of 32 to 16.  We register 6 tests today;
 * 16 is comfortable headroom.  Saves 16 * sizeof(void*) = 64 B. */
#define KVB_MAX_REGISTERED_TESTS 16u

/* Same throughput batch as STM32F0308 — 256 keeps measurement windows
 * comparable across targets at different clock speeds.  At 64 MHz on M4,
 * each batch takes well under a millisecond, so the 10 s measurement
 * window covers thousands of batches per test. */
#define KVB_THROUGHPUT_BATCH    256u

#define KVB_ENABLE_RT_TESTS          1u

#endif /* KVB_CONFIG_NRF54L15_H */
