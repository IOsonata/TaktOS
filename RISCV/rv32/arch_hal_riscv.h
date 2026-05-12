/**---------------------------------------------------------------------------
@file   arch_hal_riscv.h

@brief  TaktOS Architecture Abstraction Layer — RISC-V RV32

Implements the six AAL functions for RV32 machine mode:
  EnterCritical / ExitCritical  — csrci / csrsi on mstatus.MIE
  MSBPos                        — Zbb clz (1 cy) or SW fallback (~12 cy)
  TriggerContextSwitch          — CLINT MSIP write (machine soft interrupt)
  WaitForInterrupt               — wfi
  ApplyProtDesc                 — PMP CSR writes (TAKT_PMP_ENABLE)

Required compiler defines (set per target in Makefile / IDE build settings):
  TAKT_CLINT_BASE — base address of the CLINT-compatible peripheral
  TAKT_CLINT_HZ   — reference clock driving mtime (= SystemCoreClock)

Safety boundary: IN — MC/DC + branch coverage required.

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
#pragma once

#include <cstdint>
#include "TaktCompiler.h"
#include <taktos/TaktOS.h>   // TaktCfg_t forward

#ifndef TAKT_CLINT_BASE
#  error "TAKT_CLINT_BASE not defined — set -DTAKT_CLINT_BASE=<addr> in project build settings (e.g. 0x02000000u for GD32VF103)"
#endif

namespace takt::arch {

// ─── Critical sections ───────────────────────────────────────────────────────

/**
 * @brief	Disable machine interrupts and save previous state.
 *
 * Saves the full mstatus CSR (contains MIE + MPP) and clears MIE atomically
 * via csrci.  Nested EnterCritical / ExitCritical pairs are safe because the
 * saved value is restored verbatim by ExitCritical.
 *
 * @return	Saved mstatus value to pass to ExitCritical.
 */
TAKT_ALWAYS_INLINE uint32_t EnterCritical() noexcept
{
    uint32_t s;
    asm volatile (
        "csrr  %0, mstatus   \n"   // save mstatus (MIE + MPP + ...)
        "csrci mstatus, 0x8  \n"   // clear MIE (bit 3)
        : "=r"(s)
        :
        : "memory"
    );
    return s;
}

/**
 * @brief	Restore machine interrupt state from a saved mstatus value.
 *
 * Writes @p saved back to mstatus verbatim via csrw, re-enabling MIE if and
 * only if it was set when the matching EnterCritical was called.
 *
 * @param	saved : Value returned by the matching EnterCritical call.
 */
TAKT_ALWAYS_INLINE void ExitCritical(uint32_t saved) noexcept
{
    asm volatile (
        "csrw  mstatus, %0"        // restore full mstatus (re-enables MIE if it was set)
        :
        : "r"(saved)
        : "memory"
    );
}

/**
 * @brief	RAII critical section — acquires on construction, releases on destruction.
 *
 * Zero overhead: EnterCritical / ExitCritical inline to two CSR instructions each.
 * Non-copyable to prevent accidental double-release.
 */
struct CriticalSection {
    uint32_t saved;
    CriticalSection()  noexcept : saved(EnterCritical()) {}
   ~CriticalSection()  noexcept { ExitCritical(saved); }
    CriticalSection(const CriticalSection&)            = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;
};

// ─── Bit-scan (MSB position) ─────────────────────────────────────────────────

#ifdef __riscv_zbb
/**
 * @brief	Return the position of the most-significant set bit (Zbb hardware path).
 *
 * Uses the Zbb @c clz instruction (1 cycle).  Undefined if @p v == 0.
 *
 * @param	v : Non-zero 32-bit value.
 * @return	Bit position of the MSB (31 = bit 31, 0 = bit 0).
 */
TAKT_ALWAYS_INLINE uint32_t MSBPos(uint32_t v) noexcept
{
    uint32_t r;
    asm volatile ("clz %0, %1" : "=r"(r) : "r"(v));
    return 31u - r;
}
#else
/**
 * @brief	Return the position of the most-significant set bit (software fallback).
 *
 * Binary search through 5 bit-masks, ~12 cycles on the GD32VF103 N205.
 * Active when the Zbb extension is not available.  Certified as a separate
 * branch path — CI must build and test both Zbb and no-Zbb configurations.
 * Undefined if @p v == 0.
 *
 * @param	v : Non-zero 32-bit value.
 * @return	Bit position of the MSB (31 = bit 31, 0 = bit 0).
 */
TAKT_ALWAYS_INLINE uint32_t MSBPos(uint32_t v) noexcept
{
    uint32_t r = 0u;
    if (v & 0xFFFF0000u)
    {
        r += 16u;
        v >>= 16u;
    }
    if (v & 0xFF00u)
    {
        r +=  8u;
        v >>=  8u;
    }
    if (v & 0xF0u)
    {
        r +=  4u;
        v >>=  4u;
    }
    if (v & 0xCu)
    {
        r +=  2u;
        v >>=  2u;
    }
    if (v & 0x2u)
    {
        r +=  1u;
    }
    return r;
}
#endif  // __riscv_zbb

// ─── Context switch trigger ───────────────────────────────────────────────────

/**
 * @brief	Request a deferred context switch via CLINT MSIP.
 *
 * Writes 1 to the CLINT MSIP register, which fires a machine-mode software
 * interrupt (mcause = 3).  The machine trap vector routes this to
 * _takt_ctx_switch_rv in ctx_switch.S.  Equivalent to ARM PendSV pending.
 */
TAKT_ALWAYS_INLINE void TriggerContextSwitch() noexcept
{
    *reinterpret_cast<volatile uint32_t*>(TAKT_CLINT_BASE) = 1u;
}

// ─── Idle / low-power wait ───────────────────────────────────────────────────

/**
 * @brief	Suspend the CPU until the next interrupt (wfi).
 *
 * Emits the RISC-V @c wfi instruction.  Power saving on most implementations.
 * Called by the idle task when no threads are ready.
 */
TAKT_ALWAYS_INLINE void WaitForInterrupt() noexcept
{
    asm volatile ("wfi" ::: "memory");
}

// ─── Memory protection (PMP) ─────────────────────────────────────────────────

/**
 * @brief	Per-task PMP protection descriptor (up to 4 entry pairs).
 *
 * Packs pmpcfgN and pmpaddrN pairs into 8 words.  Written by
 * takt::arch::riscv::PmpEntry() and applied by ApplyProtDesc() on every
 * context switch when TAKT_PMP_ENABLE = 1.
 */
struct ProtDesc { uint32_t words[8]; };

#if TAKT_PMP_ENABLE
/**
 * @brief	Write a task's PMP descriptor to the PMP CSRs (TAKT_PMP_ENABLE path).
 *
 * Called from ctx_switch.S on every context switch.  Each entry pair writes
 * pmpcfgN and pmpaddrN for one PMP region.  Implemented in pmp.cpp.
 *
 * @param	d : Pointer to the task's ProtDesc (null → skip).
 */
void ApplyProtDesc(const ProtDesc* d) noexcept;   // implemented in pmp.cpp
#else
/**
 * @brief	No-op ApplyProtDesc when PMP is disabled.
 */
TAKT_ALWAYS_INLINE void ApplyProtDesc(const ProtDesc*) noexcept {}
#endif

} // namespace takt::arch