/**---------------------------------------------------------------------------
@file   clint.cpp

@brief  TaktOS standard RV32 port — CLINT tick, trap dispatch, first-task launch

This file is the standard RV32 counterpart to ARM/src/TaktKernelCM.cpp.
It provides the generic CLINT-based tick path used by standard RV32 targets
such as GD32VF103 / FE310 style machine-mode implementations.

Public kernel entry points implemented here:
  TaktOSTickInit()        — configure CLINT mtime / mtimecmp for TickHz
  TaktKernelHandlerAssign() — bind mtvec to the trap entry used by TaktOS
  TaktOSStartFirst()      — launch the first task by restoring its fake frame

The machine timer trap re-arms mtimecmp, then calls the portable
TaktKernelTickHandler() supplied by src/taktos.cpp.

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
#include <stdint.h>

#include "TaktCompiler.h"
#include "TaktKernel.h"
#include "port_rv32.h"

#ifndef TAKT_CLINT_BASE
#  error "TAKT_CLINT_BASE not defined - set -DTAKT_CLINT_BASE=<addr> in project build settings"
#endif

// CLINT register layout (GD32VF103 / FE310 compatible)
static constexpr uintptr_t CLINT_MSIP     = TAKT_CLINT_BASE + 0x0000u;
static constexpr uintptr_t CLINT_MTIME_LO = TAKT_CLINT_BASE + 0xBFF8u;
static constexpr uintptr_t CLINT_MTIME_HI = TAKT_CLINT_BASE + 0xBFFCu;
static constexpr uintptr_t CLINT_MTIMECMP = TAKT_CLINT_BASE + 0x4000u;

static inline volatile uint32_t& MMIO32(uintptr_t addr)
{
    return *reinterpret_cast<volatile uint32_t*>(addr);
}

static uint32_t s_TickPeriod = 0u;

extern "C" void _takt_trap_entry() noexcept;
extern "C" void _takt_ctx_switch_rv() noexcept;
extern "C" void TaktKernelTickHandler(void);
extern "C" TaktKernelCtx_t g_TaktosCtx;

static uint64_t ReadMtime() noexcept
{
    uint32_t lo;
    uint32_t hi;
    uint32_t hi2;

    do {
        hi  = MMIO32(CLINT_MTIME_HI);
        lo  = MMIO32(CLINT_MTIME_LO);
        hi2 = MMIO32(CLINT_MTIME_HI);
    } while (hi != hi2);

    return (static_cast<uint64_t>(hi) << 32u) | lo;
}

static void WriteMtimecmp(uint64_t v) noexcept
{
    MMIO32(CLINT_MTIMECMP + 4u) = 0xFFFFFFFFu;
    MMIO32(CLINT_MTIMECMP)      = static_cast<uint32_t>(v);
    MMIO32(CLINT_MTIMECMP + 4u) = static_cast<uint32_t>(v >> 32u);
}

static void TaktRV32TickIrq() noexcept
{
    const uint64_t cmp = (static_cast<uint64_t>(MMIO32(CLINT_MTIMECMP + 4u)) << 32u)
                       | MMIO32(CLINT_MTIMECMP);
    WriteMtimecmp(cmp + s_TickPeriod);

    TaktKernelTickHandler();
}

extern "C" bool TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr)
{
    const uintptr_t tvec = (HandlerBaseAddr != 0u ? HandlerBaseAddr :
                            (uintptr_t)_takt_trap_entry) & ~(uintptr_t)3u;
    asm volatile ("csrw mtvec, %0" :: "r"(tvec) : "memory");

    return false;
}

void TaktOSTickInit(uint32_t KernClockHz, uint32_t TickHz, TaktOSTickClockSrc_t TickClockSrc)
{
    (void)TickClockSrc;

    s_TickPeriod = KernClockHz / TickHz;
    const uint64_t now = ReadMtime();
    WriteMtimecmp(now + s_TickPeriod);

    asm volatile (
        "li   t0, 0x80     \n"
        "csrs mie, t0"
        ::: "t0", "memory"
    );
}

extern "C" TAKT_WEAK void _takt_default_trap_handler() noexcept
{
    while (true)
    {
        asm volatile ("wfi");
    }
}

extern "C" void _takt_trap_entry() noexcept
{
    uint32_t cause;
    asm volatile ("csrr %0, mcause" : "=r"(cause));

    const bool isInterrupt = (cause & 0x80000000u) != 0u;
    const uint32_t code    = cause & 0x7FFFFFFFu;

    if (isInterrupt)
    {
        if (code == 3u)
        {
            _takt_ctx_switch_rv();
            return;
        }

        if (code == 7u)
        {
            TaktRV32TickIrq();
            return;
        }
    }

    _takt_default_trap_handler();
}

extern "C" void TaktOSStartFirst()
{
    asm volatile (
        "la   t0, g_TaktosCtx       \n"
        "lw   t1, 0(t0)             \n"
        "lw   sp, 0(t1)             \n"

        "lw   t0, %[mepc](sp)       \n"
        "csrw mepc, t0              \n"
        "lw   t0, %[mstatus](sp)    \n"
        "csrw mstatus, t0           \n"

        "lw   x1,  %[x1](sp)        \n"
        "lw   x6,  %[x6](sp)        \n"
        "lw   x7,  %[x7](sp)        \n"
        "lw   x8,  %[x8](sp)        \n"
        "lw   x9,  %[x9](sp)        \n"
        "lw   x10, %[x10](sp)       \n"
        "lw   x11, %[x11](sp)       \n"
        "lw   x12, %[x12](sp)       \n"
        "lw   x13, %[x13](sp)       \n"
        "lw   x14, %[x14](sp)       \n"
        "lw   x15, %[x15](sp)       \n"
        "lw   x16, %[x16](sp)       \n"
        "lw   x17, %[x17](sp)       \n"
        "lw   x18, %[x18](sp)       \n"
        "lw   x19, %[x19](sp)       \n"
        "lw   x20, %[x20](sp)       \n"
        "lw   x21, %[x21](sp)       \n"
        "lw   x22, %[x22](sp)       \n"
        "lw   x23, %[x23](sp)       \n"
        "lw   x24, %[x24](sp)       \n"
        "lw   x25, %[x25](sp)       \n"
        "lw   x26, %[x26](sp)       \n"
        "lw   x27, %[x27](sp)       \n"
        "lw   x28, %[x28](sp)       \n"
        "lw   x29, %[x29](sp)       \n"
        "lw   x30, %[x30](sp)       \n"
        "lw   x31, %[x31](sp)       \n"
        "lw   x5,  %[x5](sp)        \n"

        "addi sp, sp, %[frame]      \n"
        "mret                       \n"
        :
        : [mepc]    "i"(CTX_MEPC),
          [mstatus] "i"(CTX_MSTATUS),
          [x1]      "i"(CTX_X1),
          [x5]      "i"(CTX_X5),
          [x6]      "i"(CTX_X6),
          [x7]      "i"(CTX_X7),
          [x8]      "i"(CTX_X8),
          [x9]      "i"(CTX_X9),
          [x10]     "i"(CTX_X10),
          [x11]     "i"(CTX_X11),
          [x12]     "i"(CTX_X12),
          [x13]     "i"(CTX_X13),
          [x14]     "i"(CTX_X14),
          [x15]     "i"(CTX_X15),
          [x16]     "i"(CTX_X16),
          [x17]     "i"(CTX_X17),
          [x18]     "i"(CTX_X18),
          [x19]     "i"(CTX_X19),
          [x20]     "i"(CTX_X20),
          [x21]     "i"(CTX_X21),
          [x22]     "i"(CTX_X22),
          [x23]     "i"(CTX_X23),
          [x24]     "i"(CTX_X24),
          [x25]     "i"(CTX_X25),
          [x26]     "i"(CTX_X26),
          [x27]     "i"(CTX_X27),
          [x28]     "i"(CTX_X28),
          [x29]     "i"(CTX_X29),
          [x30]     "i"(CTX_X30),
          [x31]     "i"(CTX_X31),
          [frame]   "i"(CTX_FRAME_BYTES)
        : "memory"
    );
    __builtin_unreachable();
}
