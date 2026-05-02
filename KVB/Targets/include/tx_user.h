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
 *   - Null pointer check at every public API entry (single inline
 *     test).  Range / object-validity checks run inside Init only,
 *     not on the Lock / Unlock / Take / Give hot path.
 *
 * FreeRTOS V11.3 matching settings (KVB FreeRTOSConfig.h):
 *   - configCHECK_FOR_STACK_OVERFLOW = 2 (canary pattern check)
 *   - configASSERT enabled — 3-4 inline asserts per public API
 *     (NULL handle plus structural invariants like item-size /
 *     scheduler-state) at every queue / sem / mutex call.
 *
 * Eclipse ThreadX matching settings (THIS FILE):
 *   - TX_ENABLE_STACK_CHECKING       (sentinel-zone overflow check)
 *   - TX_DISABLE_ERROR_CHECKING NOT set (so _txe_* parameter and
 *     object-validity wrappers run on every public API call;
 *     adds a real C call frame plus 4-5 checks per call)
 *
 * The three kernels draw the kernel/application-responsibility line
 * at different places.  This is a design choice, not a quality
 * gradient:
 *
 *   - TaktOS fences the kernel against caller errors that would
 *     corrupt kernel state and stops there.  Always-on hot-path
 *     checks at every public API entry are: NULL on the handle
 *     (and pData on Queue Send / Receive), non-owner unlock
 *     rejection on every MutexUnlock (returns ERR_INVALID, no
 *     trap), and 4-byte alignment of pData on Queue Send / Receive
 *     (returns ERR_ALIGN).  Init APIs additionally range-check
 *     MaxCount / Initial / Ceiling / ItemSize / Capacity and
 *     pStorage alignment, but Init is not on the hot loop.  Slow /
 *     blocking paths add an ISR-context guard plus a scheduler-
 *     ring corruption-detect.  No extra C call frame on any of
 *     the inline hot paths.  Validating arbitrary caller arguments
 *     beyond this list is the application's job.  This is the
 *     boundary IEC 61508 SIL 2 / ISO 26262 ASIL D draw  the kernel
 *     does the minimum required to stay safe; it does not babysit
 *     the application.
 *   - FreeRTOS adds a middle layer of caller-misuse defence:
 *     configASSERT inlined 3-4 times per public API (NULL plus
 *     structural invariants like item-size / scheduler-state).
 *   - ThreadX adds an outer _txe_* wrapper that re-validates every
 *     argument on every call (NULL + object-ID + wait-option +
 *     timer-thread + ISR posture), then BLs the corresponding _tx_*
 *     body.  Adds a real C call frame (smaller on M4 than on M0 —
 *     fewer spills because of the larger register file — but
 *     structurally still present).  Maximum-paranoia end of the
 *     spectrum, and the heaviest per-call cost in the suite.
 *
 * The wrapper-function shape is a structural property of how ThreadX
 * is distributed.  KVB keeps each kernel's native validation regime
 * in place rather than overriding any of them — measuring a kernel
 * as it ships, not as a tuned-down variant nobody would deploy.  The
 * cost ThreadX pays for its design choice is visible in the
 * benchmark numbers, which is what the report should show.
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

#if defined(__VFP_FP__) && !defined(__SOFTFP__)
#define TX_ENABLE_FPU_SUPPORT                   1
#endif

/* On Cortex-M33 / M55 (ARMv8-M Mainline) we run single-mode secure: no
 * TrustZone, no NS world. Without TX_SINGLE_MODE_SECURE the M33/M55 port
 * emits secure/NS context-switch glue that requires a TrustZone partition
 * we don't have. On Cortex-M0/M3/M4/M7 (ARMv6-M / ARMv7-M) the macro is
 * unknown to the port and harmless to omit. */
#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8_1M_MAIN__)
#define TX_SINGLE_MODE_SECURE                   1
#endif

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
