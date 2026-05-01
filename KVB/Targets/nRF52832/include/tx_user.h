/**-------------------------------------------------------------------------
 * @file    tx_user.h
 *
 * @brief   Eclipse ThreadX configuration for nRF52-DK + KVB benchmark.
 *
 * Feature-parity with TaktOS and FreeRTOS configurations on the same
 * target.  Same settings as the STM32F0308 build — only the comment
 * about SRAM constraint differs (this target has 64 KB SRAM, no tight
 * memory budget, so no settings need to flex on size grounds).
 *
 * TaktOS default behavior (always-on, not optional):
 *   - Per-thread stack overflow detection
 *   - Null parameter checks on public API entry points
 *   - Object validity checks on public API entry points
 *
 * FreeRTOS V11.3 matching settings (KVB FreeRTOSConfig.h):
 *   - configCHECK_FOR_STACK_OVERFLOW = 2 (canary pattern check)
 *   - configASSERT enabled (catches NULL handles, bad parameters)
 *
 * Eclipse ThreadX matching settings (THIS FILE):
 *   - TX_ENABLE_STACK_CHECKING       (sentinel-zone overflow check)
 *   - TX_DISABLE_ERROR_CHECKING NOT set (so _txe_* parameter and
 *     object-validity wrappers run on every public API call)
 *
 * The three kernels implement equivalent checks differently:
 *
 *   - TaktOS: inline check at function entry, single conditional branch,
 *     no extra stack frame.
 *   - FreeRTOS: configASSERT macro inlined at the call site, no extra
 *     stack frame.
 *   - ThreadX: separate _txe_* wrapper function calls _tx_* implementation,
 *     adds a real C call frame (smaller on M4 than on M0 — fewer spills
 *     because of the larger register file — but structurally still
 *     present) to every public API.
 *
 * The wrapper-function shape is a structural property of how ThreadX is
 * distributed.  KVB chooses to keep error checking ON for this comparison
 * so that all three kernels are running with equivalent runtime safety.
 * The cost ThreadX pays for that safety is visible in the benchmark
 * numbers - which is exactly what an apples-to-apples comparison should show.
 * -------------------------------------------------------------------------*/
#ifndef TX_USER_H
#define TX_USER_H

#define TX_TIMER_TICKS_PER_SECOND               1000

/* Maximum priority levels - 32 is ThreadX default.  KVB uses only 5
 * canonical levels mapped into the 1..30 range; cost is one ULONG bitmap. */
#define TX_MAX_PRIORITIES                       32u

/* Stack overflow checking - parity with TaktOS always-on stack guard
 * and FreeRTOS configCHECK_FOR_STACK_OVERFLOW = 2. */
#define TX_ENABLE_STACK_CHECKING                1

/* Error checking - explicitly NOT disabled. */
/* #define TX_DISABLE_ERROR_CHECKING            (NOT defined) */

/* Notify callbacks - KVB doesn't use them.  Disabling shrinks code
 * size and is consistent with the no-callback shape of the other two
 * kernels' KVB integrations. */
#define TX_DISABLE_NOTIFY_CALLBACKS             1

#define TX_DISABLE_REDUNDANT_CLEARING           1

#define TX_DISABLE_PREEMPTION_THRESHOLD         1

/* Timer processing in ISR - eliminates ThreadX's dedicated timer thread.
 * Tick processing runs directly in the SysTick ISR, matching FreeRTOS
 * (vTaskIncrementTick called from xPortSysTickHandler) and TaktOS
 * (sleep wake-up via SysTick handler). */
#define TX_TIMER_PROCESS_IN_ISR                 1

#endif /* TX_USER_H */
