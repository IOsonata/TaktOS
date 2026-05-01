/**-------------------------------------------------------------------------
 * @file    kvb_config_stm32f0308_freertos.h
 * @brief   KVB build configuration for STM32F0308-DISCO + FreeRTOS.
 *
 * Forced into every TU via -include in the Eclipse project settings, so
 * its values land before include/kvb_config.h's #ifndef-guarded defaults.
 *
 * Memory budget (8 KB SRAM total).  FreeRTOS keeps the TCB OUTSIDE the
 * caller-supplied stack buffer (StaticTask_t lives inside KvbThread),
 * so per-task cost is just (StackBytes + ~88 B TCB).  Compare to TaktOS
 * which pays the 256 B M0 alignment quantum on every thread block —
 * FreeRTOS uses ~1/3 the SRAM per task at the same usable stack.
 *
 *   FreeRTOS kernel infrastructure (idle TCB + stack, scheduler state)
 *                                                       ~= 0.6 KB
 *   Runner thread     (StaticTask_t + 512 B stack)      ~= 0.6 KB
 *   Worker tasks (3)  3 * (StaticTask_t + 256 B stack)  ~= 1.0 KB
 *   Mutex-owner task  (StaticTask_t + 256 B stack)      ~= 0.4 KB
 *   IOsonata UART TxFIFO (UARTFIFOSIZE = 128) + state   ~= 0.3 KB
 *   Queue + sem + mutex storage (StaticQueue_t etc)     ~= 0.4 KB
 *   KVB statics (registry 16 * 4 B + result + log)      ~= 0.4 KB
 *   ISR/main MSP + bss headroom                         ~= 0.7 KB
 *   ----------------------------------------------------------------------
 *   Total                                                ~= 4.4 / 8.0 KB
 *
 * Generous headroom — the FreeRTOS variant has ~3.6 KB free.  This is
 * NOT a bug in TaktOS; it's a deliberate trade-off.  TaktOS reserves a
 * 256 B stack guard region per thread on M0 to catch overflow without
 * hardware MPU; FreeRTOS does no such check at static-config level.
 * Both produce correct programs at the configured stack sizes.
 *
 * Same UART, same shared platform layer, same KVB framework as the
 * TaktOS counterpart — the only differences are kernel selection and
 * the per-kernel runner stack tuning below.
 * -------------------------------------------------------------------------*/
#ifndef KVB_CONFIG_STM32F0308_FREERTOS_H
#define KVB_CONFIG_STM32F0308_FREERTOS_H

#define KVB_PORT_FREERTOS       1

#define KVB_CORE_CLOCK_HZ       48000000u
#define KVB_TICK_HZ             1000u
#define KVB_MEASUREMENT_MS      10000u   /* 10 s window: tick-edge slop drops to ~0.04% */
#define KVB_WARMUP_MS           100u

/* Runner stack in usable bytes.
 *
 * The TaktOS counterpart runs comfortably at 512 B because TaktOS's queue
 * and semaphore creation paths are shallow (one critical section, direct
 * struct init, no list-of-lists walk).  FreeRTOS V11.3 on Cortex-M0 needs
 * roughly twice that on the same hot path because:
 *
 *   - xQueueCreateCountingSemaphore -> xQueueGenericCreate ->
 *     prvInitialiseNewQueue -> xQueueGenericReset -> vListInitialise
 *     is a five-frame chain, each frame ~24 B of callee-save + locals
 *     on M0 where there are no high registers (r0-r7 only) so spills
 *     happen earlier.
 *   - V11.3 unified port keeps an MPU-aware stack frame layout even when
 *     configENABLE_MPU=0, costing extra words at task entry.
 *   - newlib-nano vsnprintf used by kvb_logf retains ~180 B of frame.
 *   - The KVB_LOG_BUFFER_SIZE buffer itself does NOT contribute to
 *     runner stack peak — it is a single static buffer in BSS
 *     (src/core/kvb_log.c, single-caller / runner-only invariant).
 *
 * 1024 B was sized after on-target diagnosis: with stack-overflow check
 * level 2 enabled and 512 B runner, the kvb_runner task tripped the hook
 * during the very first kvb_sem_create call.  At 1024 B the chain runs
 * to completion.  The +512 B comes out of the 4 KB heap_4 backing array,
 * which retains comfortable margin (heap usage stays under 2.8 KB even
 * at peak with all worker tasks created).
 *
 * On targets where FreeRTOS allocation goes through static buffers
 * instead of the heap, the runner stack lives in BSS (g_kvb_runner_stack)
 * so the size shows up directly in the .bss section rather than in heap
 * pressure.  Either way the same 1024 B figure applies. */
#define KVB_RUNNER_STACK_SIZE   1024u
#define KVB_DEFAULT_STACK_SIZE  256u

#define KVB_WORKER_THREAD_COUNT 3u

#define KVB_QUEUE_DEPTH         8u
#define KVB_QUEUE_MESSAGE_SIZE  16u

/* 192 B fits the longest header line (PLATFORM/CONFIG, ~155-165 chars).
 * Lives in BSS — see the runner-stack rationale above for the full
 * accounting; this size affects flat BSS only, not stack peak. */
#define KVB_LOG_BUFFER_SIZE     192u

/* UART TX FIFO override — same value as the TaktOS project so both
 * variants stream log output through the same-sized FIFO.  Eliminates
 * any concern that one kernel runs faster only because more bytes fit
 * in flight. */
#define UARTFIFOSIZE            CFIFO_MEMSIZE(128)

/* Match the TaktOS project's registry trim — fits the same 6 currently-
 * registered tests with comfortable headroom. */
#define KVB_MAX_REGISTERED_TESTS 16u

/* Same throughput batch as the TaktOS counterpart so per-batch wall-clock
 * measurement windows are identical for direct comparison. */
#define KVB_THROUGHPUT_BATCH    256u

#endif /* KVB_CONFIG_STM32F0308_FREERTOS_H */
