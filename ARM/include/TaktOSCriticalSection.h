/**---------------------------------------------------------------------------
@file   TaktOSCriticalSection.h

@brief  TaktOS ARM Cortex-M critical section inline API

Provides TaktOSEnterCritical() / TaktOSExitCritical() using PRIMASK-based
interrupt masking.  Valid for all Cortex-M variants.

Application code may include this header directly when explicit critical-
section control is needed.  Kernel object headers pull it in transitively
through TaktKernel.h.

Safety boundary: IN  MC/DC + branch coverage required.

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
#ifndef __TAKTOSCRITICALSECTION_H__
#define __TAKTOSCRITICALSECTION_H__

#include <stdint.h>
#include "TaktCompiler.h"

#ifdef __cplusplus
extern "C" {
#endif

//  Host / unit-test stub path 
// When building on x86/x64 (host-native unit tests, TAKT_ARCH_STUB=1), ARM
// inline assembly is unavailable.  Provide no-op stubs so the host build
// compiles and links without modification.  These stubs model a single-
// threaded host where "interrupts" are always conceptually enabled; the
// scheduler state is protected by the test harness not running concurrently.
#if defined(TAKT_ARCH_STUB) || defined(__x86_64__) || defined(__i386__)

static TAKT_ALWAYS_INLINE uint32_t TaktOSEnterCritical(void) { return 0u; }
static TAKT_ALWAYS_INLINE void     TaktOSExitCritical(uint32_t) {}
static TAKT_ALWAYS_INLINE void     TaktOSDisableInterrupts(void) {}
static TAKT_ALWAYS_INLINE void     TaktOSEnableInterrupts(void) {}

#else //  ARM Cortex-M target path 


static TAKT_ALWAYS_INLINE uint32_t TaktOSEnterCritical(void) {
    uint32_t primask;
    __asm__ volatile (
        "MRS %0, PRIMASK \n"
        "CPSID I"
        : "=r"(primask) :: "memory"
    );
    return primask;
}

static TAKT_ALWAYS_INLINE void TaktOSExitCritical(uint32_t saved) {
    __asm__ volatile (
        "MSR PRIMASK, %0"
        :: "r"(saved) : "memory"
    );
}

/* Unconditional interrupt disable/enable  no state saved or restored.
 *
 * Use ONLY when the caller guarantees interrupts are already enabled on
 * entry and wants to bracket a short atomic update without the overhead
 * of MRS/MSR PRIMASK.  Calling TaktOSDisableInterrupts() from inside an
 * existing critical section (PRIMASK=1) would re-enable interrupts on the
 * paired TaktOSEnableInterrupts() call, which is incorrect.
 *
 * Correct use case: TaktOSThreadYield()  must be called from Thread mode
 * with interrupts enabled; saves ~3 cycles vs the save-restore pattern by
 * allowing GCC to use only caller-save registers (r0r3), eliminating the
 * PUSH {r4,lr} / POP {r4,pc} function frame.                             */
static TAKT_ALWAYS_INLINE void TaktOSDisableInterrupts(void) {
    __asm__ volatile ("CPSID I" ::: "memory");
}

static TAKT_ALWAYS_INLINE void TaktOSEnableInterrupts(void) {
    __asm__ volatile ("CPSIE I" ::: "memory");
}

#endif // TAKT_ARCH_STUB / x86 vs ARM

#ifdef __cplusplus
}
#endif

#endif // __TAKTOSCRITICALSECTION_H__
