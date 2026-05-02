/**-------------------------------------------------------------------------
 * @file    kvb_config_nrf52832_freertos.h
 * @brief   KVB build configuration for nRF52-DK (PCA10040) + FreeRTOS.
 *
 * Forced into every TU via -include in the Eclipse project settings, so
 * its values land before include/kvb_config.h's #ifndef-guarded defaults.
 *
 * Memory budget (64 KB SRAM total) — comfortable on this target.  FreeRTOS
 * V11.3 on Cortex-M4 has its TCB outside the caller-supplied stack (TCB
 * lives inside KvbThread when configSUPPORT_STATIC_ALLOCATION=1), so per-
 * task cost is just (StackBytes + ~88 B TCB).
 *
 *   FreeRTOS kernel infrastructure (idle TCB + stack, scheduler state)
 *                                                       ~= 0.6 KB
 *   Runner thread     (StaticTask_t + 2048 B stack)     ~= 2.1 KB
 *   Worker tasks (3)  3 * (StaticTask_t + 512 B stack)  ~= 1.8 KB
 *   Mutex-owner task  (StaticTask_t + 512 B stack)      ~= 0.6 KB
 *   IOsonata UART TxFIFO (UARTFIFOSIZE = 256) + state   ~= 0.5 KB
 *   Queue + sem + mutex storage (StaticQueue_t etc)     ~= 0.4 KB
 *   KVB statics (registry 16 * 4 B + result + log)      ~= 0.4 KB
 *   ISR/main MSP + bss headroom                         ~= 1.5 KB
 *   ----------------------------------------------------------------------
 *   Total                                               ~= 7.4 / 64.0 KB
 *
 * Same UART, same shared platform layer, same KVB framework as the TaktOS
 * counterpart — only kernel selection and a couple of per-kernel runner
 * tunings differ.
 *
 * Stack sizing rationale: runner stack at 2048 B (up from initial 1024 B)
 * because the metric-print path goes runner -> kvb_run_all_tests
 * (KvbTestResult ~150 B) -> kvb_log_test_result -> kvb_logf -> newlib
 * vsnprintf (~180 B frame).  Empirically, 1024 B was marginal at -O0
 * with newlib full vsnprintf; the firmware truncated mid-line during
 * SYNC_MUTEX_FAST_001 metric emission.  2048 B has generous headroom
 * and the SRAM budget is large enough to absorb it without trade-off.
 * Worker stacks at 512 B match the TaktOS variant on this target —
 * the FreeRTOS/TaktOS worker stack pair is documented in §3.5 of the
 * KVB Comparison report as the lower of the two paired sizes
 * (ThreadX/Zephyr use 1024 B for their per-thread overhead reasons).
 *
 * Note: the KVB_LOG_BUFFER_SIZE buffer itself does NOT live on the
 * runner stack.  It is a single static buffer in BSS (src/core/kvb_log.c,
 * single-caller invariant), so its size affects flat BSS only, not stack
 * peak.  The runner stack here is sized for the vsnprintf + call chain,
 * which is the dominant transient cost.
 *
 * Allocation model: pure static.  configSUPPORT_DYNAMIC_ALLOCATION=0,
 * configSUPPORT_STATIC_ALLOCATION=1, configTOTAL_HEAP_SIZE=0 — no heap_4
 * needed.  This matches the proven Benchmark/ThreadMetric/nRF52832
 * FreeRTOS reference build, which has been the canonical FreeRTOS
 * configuration on this target since the original Thread-Metric port.
 * The STM32F0308 FreeRTOS variant uses heap_4 because its specific
 * V11.3 ARM_CM0 split-port + IOsonata startup combination hardfaults
 * under pure static allocation; that issue is M0-specific and does not
 * apply on Cortex-M4F.
 * -------------------------------------------------------------------------*/
#ifndef KVB_CONFIG_NRF52832_FREERTOS_H
#define KVB_CONFIG_NRF52832_FREERTOS_H

#define KVB_PORT_FREERTOS       1

#define KVB_CORE_CLOCK_HZ       64000000u
#define KVB_TICK_HZ             1000u
#define KVB_MEASUREMENT_MS      10000u
#define KVB_WARMUP_MS           100u

#define KVB_RUNNER_STACK_SIZE   2048u
#define KVB_DEFAULT_STACK_SIZE  512u

#define KVB_WORKER_THREAD_COUNT 3u

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u

/* 192 B fits the longest header line (PLATFORM/CONFIG, ~155-165 chars)
 * with margin.  In BSS, not on the stack — see header docstring. */
#define KVB_LOG_BUFFER_SIZE     192u

#define UARTFIFOSIZE            CFIFO_MEMSIZE(256)

#define KVB_MAX_REGISTERED_TESTS 16u
#define KVB_THROUGHPUT_BATCH    256u

#endif /* KVB_CONFIG_NRF52832_FREERTOS_H */
