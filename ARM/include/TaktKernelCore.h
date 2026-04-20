/**---------------------------------------------------------------------------
@file   TaktKernelCore.h

@brief  TaktOS ARM Cortex-M core kernel interface

Provides the ARM-specific primitives that the portable kernel depends on:
  TaktOSCtxSwitch()    pend PendSV via SCB ICSR (inlined, ~2 cy)
  TaktOSStartFirst()   launch first task via SVC #0 (declared; implemented
                        in ARM/cm{0,4,7,33,55}/PendSV_M*.S)
  TAKTOS_SOFT_STACK_CHECK  defined on M0/M4/M7 (no PSPLIM) to enable the
                        software guard-word check in the tick handler

Included by TaktKernel.h so that all kernel sources see these primitives.
Does NOT contain the SysTick_Handler alias  that lives in TaktKernelTick.h
and is included only by taktos.cpp (the TU that defines TaktKernelTickHandler).

Safety boundary: IN  branch + line coverage required.

@author Nguyen Hoan Hoang
@date   Apr. 2026

@license

MIT License

Copyright (c) 2026 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/
#ifndef __TAKTKERNELCORE_H__
#define __TAKTKERNELCORE_H__

#include "TaktCompiler.h"

// Thread memory sizing is architecture-defined here so each port can account
// for:
//   - the initial context frame built by TaktOSStackInit()
//   - stack-top alignment slack (SP is aligned down before the frame is built)
//   - any reserved no-access stack-guard region used by the MPU-aware handler
//   - any slack required to align pStackBottom to the guard-region boundary
#if defined(__ARM_ARCH_6M__)
#  define TAKTOS_STACK_GUARD_ALIGN         256u
#  define TAKTOS_THREAD_INIT_FRAME_SIZE     64u
#  define TAKTOS_THREAD_STACK_TOP_ALIGN      8u
#  define TAKTOS_THREAD_GUARD_REGION_SIZE  256u
#elif defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#  define TAKTOS_STACK_GUARD_ALIGN          32u
#  define TAKTOS_THREAD_INIT_FRAME_SIZE     68u
#  define TAKTOS_THREAD_STACK_TOP_ALIGN      8u
#  define TAKTOS_THREAD_GUARD_REGION_SIZE   32u
#elif defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8M_MAIN__)
#  define TAKTOS_STACK_GUARD_ALIGN          32u
#  define TAKTOS_THREAD_INIT_FRAME_SIZE     68u
#  define TAKTOS_THREAD_STACK_TOP_ALIGN      8u
   // 32 bytes reserved at the stack floor. When the MPU-aware handlers
   // (PendSV_Handler_MPU / SVC_Handler_MPU) are bound via
   // TaktKernelHandlerAssign(), this region is the ARMv8-M MPU region 7
   // stack guard (AP = 10 RO-priv, XN = 1  writes fault). When they are
   // not bound, PSPLIM alone still protects the stack and these 32 bytes
   // are simply unused headroom between PSPLIM and the usable stack.
   // Either way, sizeof each thread is larger by 32 bytes versus the
   // previous 0-size guard region.
#  define TAKTOS_THREAD_GUARD_REGION_SIZE   32u
#else
#  error "Unsupported ARM core for TaktKernelCore.h"
#endif

#define TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD                             \
    (sizeof(TaktOSThread_t) + sizeof(uint32_t) +                          \
     (TAKTOS_STACK_GUARD_ALIGN - 1u) +                                    \
     TAKTOS_THREAD_GUARD_REGION_SIZE +                                    \
     TAKTOS_THREAD_INIT_FRAME_SIZE +                                      \
     (TAKTOS_THREAD_STACK_TOP_ALIGN - 1u))


// Software guard word check is needed on targets that have no hardware
// stack limit register (PSPLIM). M33/M55 have PSPLIM  the hardware fires
// before PSP can reach the guard word, so the software check is redundant.
// M0/M0+, M3/M4/M7 have no PSPLIM; software check is the only fallback
// when MPU is also not enabled.
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#  define TAKTOS_SOFT_STACK_CHECK
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Pend PendSV via SCB ICSR  request a deferred context switch.
 *
 * Inlined at every call site to eliminate the BL/BX overhead (~5 cy).
 * Sets bit 28 (PENDSVSET) of the Interrupt Control and State Register.
 * PendSV fires after all higher-priority exceptions have returned, making
 * this the lowest-latency safe context-switch mechanism on Cortex-M.
 *
 * Host/stub builds: no-op.
 */
#if defined(TAKT_ARCH_STUB) || defined(__x86_64__) || defined(__i386__)
static TAKT_ALWAYS_INLINE void TaktOSCtxSwitch(void) { /* host stub  no-op */ }
#else
static TAKT_ALWAYS_INLINE void TaktOSCtxSwitch(void)
{
    *((volatile unsigned int*)0xE000ED04u) = (1u << 28);
}
#endif

/**
 * @brief	Launch the first task  ARM Cortex-M specific, never returns.
 *
 * Issues SVC #0, which triggers SVC_Handler.  SVC_Handler loads the first
 * task's fake context frame, switches the active stack from MSP (used during
 * init) to PSP (used by all tasks), and performs an exception return into
 * Thread mode.  This is the only safe way to make the MSPPSP transition on
 * Cortex-M  a plain branch would leave the processor in Handler mode.
 *
 * Implemented in ARM/cm{0,4,7,33,55}/PendSV_M*.S (one per Cortex-M variant).
 * Called once from TaktOSStart() in taktos.cpp.
 *
 * Host/stub builds: no-op stub (TaktOSStart() is never called from tests).
 */
#if !defined(TAKT_ARCH_STUB) && !defined(__x86_64__) && !defined(__i386__)
void TaktOSStartFirst(void);
#else
static TAKT_ALWAYS_INLINE void TaktOSStartFirst(void) {} /* host stub */
#endif

#ifdef __cplusplus
}
#endif

#endif // __TAKTKERNELCORE_H__
