/**---------------------------------------------------------------------------
@file   port_rv32.h

@brief  TaktOS RISC-V RV32 port constants — context frame layout and TCB offsets

Usable from both C/C++ and assembly (no C++ constructs).
Defines the context frame layout and TCB field offsets consumed by
ctx_switch.S and task_init_riscv.cpp.

Frame: 34 words × 4 bytes = 136 bytes
  mepc, mstatus, x1(ra), x5–x31

Safety boundary: IN — read by both cert-boundary assembly and C++.

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

// ─── Context frame (136 bytes = 34 × 4) ─────────────────────────────────────
#define CTX_FRAME_BYTES   136

#define CTX_MEPC          0      // mepc   — interrupted PC
#define CTX_MSTATUS       4      // mstatus — machine status (MIE/MPIE)
#define CTX_X1            8      // ra
#define CTX_X5            12     // t0
#define CTX_X6            16     // t1
#define CTX_X7            20     // t2
#define CTX_X8            24     // s0/fp
#define CTX_X9            28     // s1
#define CTX_X10           32     // a0
#define CTX_X11           36     // a1
#define CTX_X12           40     // a2
#define CTX_X13           44     // a3
#define CTX_X14           48     // a4
#define CTX_X15           52     // a5
#define CTX_X16           56     // a6
#define CTX_X17           60     // a7
#define CTX_X18           64     // s2
#define CTX_X19           68     // s3
#define CTX_X20           72     // s4
#define CTX_X21           76     // s5
#define CTX_X22           80     // s6
#define CTX_X23           84     // s7
#define CTX_X24           88     // s8
#define CTX_X25           92     // s9
#define CTX_X26           96     // s10
#define CTX_X27           100    // s11
#define CTX_X28           104    // t3
#define CTX_X29           108    // t4
#define CTX_X30           112    // t5
#define CTX_X31           116    // t6
// words 30-33 (x2=sp, x3=gp, x4=tp): not saved — sp managed separately,
// gp and tp are ABI-defined thread-constant pointers, not context-switched.

// ─── TaktosCtx offsets (mirrors context.cpp packed struct) ──────────────────
// struct TaktosCtx { Tcb* current; Tcb* next_thread; };
#define CTX_CURRENT       0      // offsetof(TaktosCtx, current)
#define CTX_NEXT_THREAD   4      // offsetof(TaktosCtx, next_thread)

// ─── TCB offsets ─────────────────────────────────────────────────────────────
// Must stay in sync with TaktOSThread_t in TaktOSThread.h.
// The static_assert in taktos_thread.cpp validates the pStackBottom offset
// at compile time; update TCB_STACK_BOTTOM here if the assert fires.
#define TCB_SP            0      // offsetof(TaktOSThread_t, pSp) — always first

// pStackBottom: 32-byte-aligned floor of the task stack.
// Used by TAKT_PMP_ENABLE to configure the per-task PMP stack guard region,
// equivalent to the ARM MPU guard written by PendSV_M4.S / PendSV_M33.S.
// Value is the raw stack floor address; PMP entry (pmpaddr / pmpcfg) is
// computed from it in ctx_switch.S when TAKT_PMP_ENABLE=1.
#ifdef TAKT_PMP_ENABLE
#define TCB_STACK_BOTTOM  16     // offsetof(TaktOSThread_t, pStackBottom)
#endif