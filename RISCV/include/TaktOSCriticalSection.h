/**---------------------------------------------------------------------------
@file   TaktOSCriticalSection.h

@brief  Generic RISC-V critical section API for TaktOS

This shared header provides the common RISC-V machine-mode interrupt masking
primitives used by TaktOS.  The deferred context-switch trigger
(TaktOSCtxSwitch) is inlined here and writes 1 to the address held in the
global pointer g_TaktSoftIntReg, which is initialized once by TaktOSInit()
from its SoftIntAddr parameter.  No build macros, no weak symbol overrides,
no preprocessor — the user passes the chip's software-interrupt register
address explicitly at init.

No vendor HAL is referenced here.  The arch library compiles standalone
and the user's application supplies the address from its board / chip
configuration when calling TaktOSInit().

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

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/
#ifndef __TAKTOSCRRITICALSECTION_H__
#define __TAKTOSCRRITICALSECTION_H__

#include <stdint.h>
#include <stdbool.h>
#include "TaktCompiler.h"

#ifdef __cplusplus
extern "C" {
#endif

TAKT_ALWAYS_INLINE uint32_t TaktOSEnterCritical(void)
{
    uint32_t mstatus;
    __asm__ volatile (
        "csrr %0, mstatus   \n"
        "csrci mstatus, 0x8 \n"
        : "=r"(mstatus) :: "memory"
    );
    return mstatus;
}

TAKT_ALWAYS_INLINE void TaktOSExitCritical(uint32_t saved)
{
    __asm__ volatile (
        "csrw mstatus, %0"
        :: "r"(saved) : "memory"
    );
}

TAKT_ALWAYS_INLINE void TaktOSDisableInterrupts(void)
{
    __asm__ volatile ("csrci mstatus, 0x8" ::: "memory");
}

TAKT_ALWAYS_INLINE void TaktOSEnableInterrupts(void)
{
    __asm__ volatile ("csrsi mstatus, 0x8" ::: "memory");
}

/* Deferred context-switch request — inline write of 1 to the chip's
 * software-interrupt trigger register.
 *
 * g_TaktSoftIntReg is set once by TaktOSInit() from Cfg->SoftIntAddr.
 * Per-chip examples (in the application's TaktOSCfg_t):
 *     CLINT-based (FE310, BL616, R9A02G021, ESP32-C6/H2):
 *         .SoftIntAddr = 0x02000000u,    // standard CLINT MSIP
 *     GD32VF103 (CLINT at non-standard base):
 *         .SoftIntAddr = 0xD1000000u,
 *     ESP32-C3 (no CLINT, uses INTMTX-routed SYSTEM register):
 *         .SoftIntAddr = 0x600C00D8u,    // SYSTEM_CPU_INTR_FROM_CPU_0
 *
 * Hot-path cost is one load of the global pointer + one store to MMIO
 * (~3-4 cy), fully inlined at every call site.  Called from TaktReadyTask /
 * TaktBlockTask whenever a higher-priority task becomes runnable or the
 * running task blocks.
 *
 * Pre-condition: TaktOSInit() has been called.  TaktOS APIs are not valid
 * before init in any case, so this constraint is no stronger than the
 * existing one.
 */
extern volatile uint32_t *g_TaktSoftIntReg;

TAKT_ALWAYS_INLINE void TaktOSCtxSwitch(void)
{
    *g_TaktSoftIntReg = 1u;
}

/* RISC-V has no architectural equivalent to Cortex-M's IPSR.  A target
 * port that needs the blocking-from-ISR safety check must override this
 * inline by defining TAKT_RISCV_PORT_HAS_IN_ISR=1 and supplying its own
 * TaktOSInIsr() — e.g. via a software-maintained ISR-nesting counter
 * incremented in the trap entry stub and decremented on exit, or via a
 * platform-specific status register read.  Default returns false; the
 * blocking APIs treat the caller as a thread context.  This is acceptable
 * for bring-up but a target intended for SIL 2 / ASIL D certification
 * must override it. */
#ifndef TAKT_RISCV_PORT_HAS_IN_ISR
TAKT_ALWAYS_INLINE bool TaktOSInIsr(void) { return false; }
#endif

/* First-task launcher, defined in ctx_switch.S.  Loads the initial frame
 * built by TaktKernelStackInit() and issues mret to enter the highest
 * priority ready task.  Never returns. */
void TaktOSStartFirst(void);

#ifdef __cplusplus
}
#endif

#endif // __TAKTOSCRRITICALSECTION_H__
