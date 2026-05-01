/**-------------------------------------------------------------------------
 * @file    kvb_config_stm32f0308_threadx.h
 * @brief   KVB build configuration for STM32F0308-DISCO + Eclipse ThreadX.
 *
 * v17 — feature parity build.  All three kernels run with equivalent
 * runtime safety: stack overflow detection, null-parameter check, and
 * object-validity check.  See tx_user.h for the ThreadX-side rationale.
 *
 * Memory budget (8 KB SRAM total).
 *
 *   Runner thread     (1 pool slot + 1024 B stack)              ~= 1.2 KB
 *   Worker threads    (3 pool slots + 3 * 768 B stacks)         ~= 2.7 KB
 *   Mutex-owner task  (1 pool slot + 768 B stack)               ~= 0.9 KB
 *   Port-private pools (4 thread + 6 sem + 2 mutex + 2 queue)   ~= 1.2 KB
 *   IOsonata UART TxFIFO + state                                ~= 0.3 KB
 *   Caller-supplied queue/sem/mutex storage                     ~= 0.3 KB
 *   KVB statics + ThreadX kernel BSS                            ~= 0.9 KB
 *   ISR/main MSP + bss headroom                                 ~= 0.4 KB
 *   ----------------------------------------------------------------------
 *   Total                                                       ~= 7.9 / 8.0 KB
 *
 * Tight but within budget.  All numbers disclosed in the published
 * report's methodology section.
 *
 * Disclosed asymmetries vs TaktOS / FreeRTOS:
 *
 *   1. Worker stack: 768 B (vs 256 B on the other two kernels).  Reason:
 *      ThreadX's _txe_* parameter-validation wrappers add a real C call
 *      frame to every public API call (~30 B on M0 -Os), which
 *      compounds with the deeper internal call chain in
 *      tx_thread_relinquish.  TaktOS inlines its checks; FreeRTOS uses
 *      configASSERT macros at the call site.  256 B has no margin for
 *      the resulting depth.
 *
 *   2. Port-private pool storage: 4 TX_THREAD + 6 TX_SEMAPHORE +
 *      2 TX_MUTEX + 2 TX_QUEUE in BSS.  Reason: ThreadX hangs control
 *      blocks on kernel-global linked lists (priority list, created
 *      list) for the lifetime of the object - addresses must remain
 *      stable, so caller-stack allocation of TCBs (the pattern KVB uses
 *      for TaktOS and FreeRTOS) corrupts kernel state on first
 *      function-frame churn.  TaktOS and FreeRTOS use their static
 *      control blocks differently (TaktOS keeps minimal kernel state
 *      pointers; FreeRTOS xTaskCreateStatic uses the address only for
 *      the lifetime of the task and unlinks cleanly on
 *      vTaskDelete).  The KVB ThreadX port matches the pattern used by
 *      Microsoft's own Thread-Metric ThreadX reference (static slot
 *      pool indexed by handle).  Pool sizes here trim to test-suite
 *      peak (3 workers + 1 mutex-owner = 4 threads peak; 4 sems peak;
 *      2 mutexes peak; 2 queues peak) - no padding for 'spare' slots.
 *
 *   3. Static allocation strategy:
 *        - TaktOS: inline-in-handle, every kernel object is a struct
 *          member of KvbThread / KvbSemaphore / etc.
 *        - FreeRTOS: configSUPPORT_STATIC_ALLOCATION + heap_4 4 KB
 *          arena (heap_4 is a workaround for V11+ ARM_CM0 split-port
 *          static-only hardfault, see KVB v2.0 report §3.4).
 *        - ThreadX: port-private static slot pool, handle indexed.
 *      All three avoid runtime malloc inside benchmark loops.  The
 *      different shapes are kernel-implementation differences, not
 *      benchmark tuning.
 *
 *   None of these asymmetries change WHAT the benchmark measures
 *   (iterations of the same kernel API in the same workload).  They
 *   change the byte-level memory layout, which is documented here for
 *   reproducibility.
 * -------------------------------------------------------------------------*/
#ifndef KVB_CONFIG_STM32F0308_THREADX_H
#define KVB_CONFIG_STM32F0308_THREADX_H

#define KVB_PORT_THREADX        1

#define KVB_CORE_CLOCK_HZ       48000000u
#define KVB_TICK_HZ             1000u
#define KVB_MEASUREMENT_MS      10000u   /* 10 s window for parity with v2.0 report */
#define KVB_WARMUP_MS           100u

/* Runner stack - 1024 B matches FreeRTOS runner exactly.  With _txe_*
 * wrappers compiled in (error checking on), the runner needs slightly
 * more headroom than at error-checking-off, but vsnprintf for log line
 * formatting is the dominant frame; 1024 B remains comfortable. */
#define KVB_RUNNER_STACK_SIZE   1024u

/* Worker stack - 768 B (vs 256 B on TaktOS/FreeRTOS).  Disclosed
 * asymmetry, kernel-structural - see header docstring. */
#define KVB_DEFAULT_STACK_SIZE  768u

#define KVB_WORKER_THREAD_COUNT 3u

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u

/* 192 B fits the longest header line (PLATFORM/CONFIG, ~155-165 chars).
 * Lives in BSS as a single static buffer (src/core/kvb_log.c,
 * single-caller / runner-only invariant) — costs flat BSS, not runner
 * stack peak.  +64 B vs prior 128 B sizing fits inside the
 * "ISR/main MSP + bss headroom" line in the memory budget above. */
#define KVB_LOG_BUFFER_SIZE     192u

/* UART TX FIFO override - same value as the TaktOS and FreeRTOS variants
 * so all three kernels stream log output through identically-sized
 * buffers. */
#define UARTFIFOSIZE            CFIFO_MEMSIZE(128)

#define KVB_MAX_REGISTERED_TESTS 16u
#define KVB_THROUGHPUT_BATCH    256u

/* Port-private pool sizes - sized to cumulative peak across the suite
 * because no test calls kvb_sem_delete / kvb_mutex_delete (slots
 * accumulate for the lifetime of the run).
 *
 *   Threads:    3 workers (SCHED_COOP_001, leaked) + 1 mutex-owner
 *               (SYNC_MUTEX_OWNERSHIP_001, deleted at test end) = 4 peak
 *   Semaphores: SCHED_COOP_001 leaks 2 (start_sem + done_sem)
 *             + SYNC_SEM_FAST_001 leaks 1
 *             + SYNC_MUTEX_OWNERSHIP_001 needs 3 simultaneously
 *               (locked_sem + release_sem + done_sem)
 *               => 6 cumulative peak (was 4 - undersized; release_sem
 *                  create failed with KVB_ERR_KERNEL on the prior run)
 *   Mutexes:    SYNC_MUTEX_FAST_001 leaks 1 + SYNC_MUTEX_OWNERSHIP_001
 *               leaks 1 = 2 peak
 *   Queues:     IPC_QUEUE_FAST_001 leaks 1 = 1 peak (2 with margin)
 *
 * BSS impact of bumping sem pool 4 -> 6 on Cortex-M0: +2 * sizeof(TX_
 * SEMAPHORE + bool + padding) ~= +160 B, fits inside the 0.3 KB
 * headroom documented in the memory budget above. */
#define KVB_THREADX_THREAD_POOL_SIZE   4u
#define KVB_THREADX_SEM_POOL_SIZE      6u
#define KVB_THREADX_MUTEX_POOL_SIZE    2u
#define KVB_THREADX_QUEUE_POOL_SIZE    2u

#endif /* KVB_CONFIG_STM32F0308_THREADX_H */
