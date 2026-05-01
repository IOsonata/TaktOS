/**-------------------------------------------------------------------------
 * @file    kvb_config_nrf52832_threadx.h
 * @brief   KVB build configuration for nRF52-DK (PCA10040) + Eclipse ThreadX.
 *
 * Feature parity build.  All three kernels run with equivalent runtime
 * safety: stack overflow detection, null-parameter check, and object-
 * validity check.  See tx_user.h for the ThreadX-side rationale.
 *
 * Memory budget (64 KB SRAM total).  Comfortable on this target.
 *
 *   Runner thread     (1 pool slot + 1024 B stack)              ~= 1.2 KB
 *   Worker threads    (3 pool slots + 3 * 1024 B stacks)        ~= 3.4 KB
 *   Mutex-owner task  (1 pool slot + 1024 B stack)              ~= 1.2 KB
 *   Port-private pools (4 thread + 6 sem + 2 mutex + 2 queue)   ~= 1.2 KB
 *   IOsonata UART TxFIFO (UARTFIFOSIZE = 256) + state           ~= 0.5 KB
 *   Caller-supplied queue/sem/mutex storage                     ~= 0.3 KB
 *   KVB statics + ThreadX kernel BSS                            ~= 0.9 KB
 *   ISR/main MSP + bss headroom                                 ~= 1.5 KB
 *   ----------------------------------------------------------------------
 *   Total                                                       ~= 10.0 / 64.0 KB
 *
 * Disclosed asymmetries vs TaktOS / FreeRTOS (same as STM32F0308 build):
 *
 *   1. Worker stack: 1024 B (vs 512 B on the other two kernels).  Reason:
 *      ThreadX's _txe_* parameter-validation wrappers add a real C call
 *      frame to every public API call.  On Cortex-M4 the cost is smaller
 *      per frame than on M0 (full register set, fewer spills) but the
 *      structural overhead is the same and adds up on tx_thread_relinquish.
 *      512 B has no margin once tx error checking is on.
 *
 *   2. Port-private pool storage: 4 TX_THREAD + 6 TX_SEMAPHORE +
 *      2 TX_MUTEX + 2 TX_QUEUE in BSS.  Reason: ThreadX hangs control
 *      blocks on kernel-global linked lists for the lifetime of the
 *      object; addresses must remain stable, so caller-stack allocation
 *      of TCBs (the pattern KVB uses for TaktOS and FreeRTOS) corrupts
 *      kernel state on first function-frame churn.  Same reason as on
 *      STM32F0308; the pool sizes here trim to test-suite peak.
 *
 *   3. Static allocation strategy:
 *        - TaktOS:   inline-in-handle, every kernel object is a struct
 *                    member of KvbThread / KvbSemaphore / etc.
 *        - FreeRTOS: pure static (configSUPPORT_STATIC_ALLOCATION=1,
 *                    configSUPPORT_DYNAMIC_ALLOCATION=0,
 *                    configTOTAL_HEAP_SIZE=0).  Mirrors the
 *                    Benchmark/ThreadMetric/nRF52832 reference build.
 *        - ThreadX:  port-private static slot pool, handle indexed.
 *      All three avoid runtime malloc inside benchmark loops.
 *
 *   None of these asymmetries change WHAT the benchmark measures.  They
 *   change byte-level memory layout, documented here for reproducibility.
 * -------------------------------------------------------------------------*/
#ifndef KVB_CONFIG_NRF52832_THREADX_H
#define KVB_CONFIG_NRF52832_THREADX_H

#define KVB_PORT_THREADX        1

#define KVB_CORE_CLOCK_HZ       64000000u
#define KVB_TICK_HZ             1000u
#define KVB_MEASUREMENT_MS      10000u
#define KVB_WARMUP_MS           100u

#define KVB_RUNNER_STACK_SIZE   1024u

/* Worker stack - 1024 B (vs 512 B on TaktOS/FreeRTOS).  Disclosed
 * asymmetry, kernel-structural - see header docstring. */
#define KVB_DEFAULT_STACK_SIZE  1024u

#define KVB_WORKER_THREAD_COUNT 3u

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u

/* 192 B fits the longest header line (PLATFORM/CONFIG, ~155-165 chars).
 * Lives in BSS as a single static buffer (src/core/kvb_log.c). */
#define KVB_LOG_BUFFER_SIZE     192u

#define UARTFIFOSIZE            CFIFO_MEMSIZE(256)

#define KVB_MAX_REGISTERED_TESTS 16u
#define KVB_THROUGHPUT_BATCH    256u

/* Port-private pool sizes — sized to cumulative peak across the suite
 * because no test calls kvb_sem_delete / kvb_mutex_delete (slots
 * accumulate for the lifetime of the run).
 *
 *   Threads:    3 workers (SCHED_COOP_001, leaked) + 1 mutex-owner
 *               (SYNC_MUTEX_OWNERSHIP_001, deleted at test end) = 4 peak
 *   Semaphores: SCHED_COOP_001 leaks 2 + SYNC_SEM_FAST_001 leaks 1
 *             + SYNC_MUTEX_OWNERSHIP_001 needs 3 simultaneously
 *               = 6 cumulative peak (was 4 — undersized; the STM32F0308
 *                 run failed SYNC_MUTEX_OWNERSHIP_001 with "release
 *                 semaphore create failed" until this was bumped)
 *   Mutexes:    SYNC_MUTEX_FAST + SYNC_MUTEX_OWNERSHIP = 2 peak
 *   Queues:     IPC_QUEUE_FAST = 1; 2 with margin */
#define KVB_THREADX_THREAD_POOL_SIZE   4u
#define KVB_THREADX_SEM_POOL_SIZE      6u
#define KVB_THREADX_MUTEX_POOL_SIZE    2u
#define KVB_THREADX_QUEUE_POOL_SIZE    2u

#endif /* KVB_CONFIG_NRF52832_THREADX_H */
