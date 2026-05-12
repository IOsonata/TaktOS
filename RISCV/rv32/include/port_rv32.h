/**---------------------------------------------------------------------------
@file   port_rv32.h

@brief  TaktOS RV32 arch lib — context frame layout and TaktKernelCtx_t offsets

Usable from both C/C++ and assembly (no C++ constructs, no headers pulled).
Defines the context frame layout consumed by ctx_switch.S, the offsets of
the kernel scheduler context block read by the switch path, and the TCB
offset used to load each task's saved stack pointer.

Frame: 32 words (128 bytes), built on the interrupted task's stack on
trap entry and rebuilt by TaktKernelStackInit() for a fresh task.

Layout (lowest address = sp+0, highest = sp+124):

    Offset  Reg   ABI    Notes
    ─────────────────────────────────────────────────────────────────────
    0       —     mepc   interrupted PC
    4       —     mstatus MIE/MPIE/MPP at trap entry
    8       x1    ra     return address
    12      x3    gp     global pointer (saved defensively; ~4cy cost)
    16      x4    tp     thread pointer (saved defensively)
    20      x5    t0     temp
    24      x6    t1     temp
    28      x7    t2     temp
    32      x10   a0     arg 0 / return 0
    36      x11   a1     arg 1 / return 1
    40      x12   a2
    44      x13   a3
    48      x14   a4
    52      x15   a5
    56      x16   a6
    60      x17   a7
    64      x28   t3
    68      x29   t4
    72      x30   t5
    76      x31   t6
    80      x8    s0/fp  callee-save
    84      x9    s1
    88      x18   s2
    92      x19   s3
    96      x20   s4
    100     x21   s5
    104     x22   s6
    108     x23   s7
    112     x24   s8
    116     x25   s9
    120     x26   s10
    124     x27   s11

x0 is hard-wired zero — never saved.
x2 (sp) is held in the TCB.pSp field, not in the frame.

Safety boundary: IN — read by cert-boundary assembly and by C++.

@author Nguyen Hoan Hoang
@date   May 2026

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
#ifndef __PORT_RV32_H__
#define __PORT_RV32_H__

/* ── Arch-level sizing macros consumed by TaktKernelCore.h ───────────────── */
/* These are properties of the RV32 ilp32* arch, not of any HAL.             */

#define TAKT_RISCV_STACK_GUARD_ALIGN       32   /* PMP NAPOT min granularity */
#define TAKT_RISCV_THREAD_INIT_ALIGN_SLACK 15   /* up to 15 bytes lost to    */
                                                /* 16-byte RV ABI alignment  */

/* ── GP-register block (32 words = 128 bytes), present for all ABIs ──────── */
#define FRAME_GP_SIZE     128

#define FRAME_MEPC        0
#define FRAME_MSTATUS     4
#define FRAME_RA          8     /* x1  */
#define FRAME_GP          12    /* x3  */
#define FRAME_TP          16    /* x4  */
#define FRAME_T0          20    /* x5  */
#define FRAME_T1          24    /* x6  */
#define FRAME_T2          28    /* x7  */
#define FRAME_A0          32    /* x10 */
#define FRAME_A1          36    /* x11 */
#define FRAME_A2          40    /* x12 */
#define FRAME_A3          44    /* x13 */
#define FRAME_A4          48    /* x14 */
#define FRAME_A5          52    /* x15 */
#define FRAME_A6          56    /* x16 */
#define FRAME_A7          60    /* x17 */
#define FRAME_T3          64    /* x28 */
#define FRAME_T4          68    /* x29 */
#define FRAME_T5          72    /* x30 */
#define FRAME_T6          76    /* x31 */
#define FRAME_S0          80    /* x8  (fp) */
#define FRAME_S1          84    /* x9  */
#define FRAME_S2          88    /* x18 */
#define FRAME_S3          92    /* x19 */
#define FRAME_S4          96    /* x20 */
#define FRAME_S5          100   /* x21 */
#define FRAME_S6          104   /* x22 */
#define FRAME_S7          108   /* x23 */
#define FRAME_S8          112   /* x24 */
#define FRAME_S9          116   /* x25 */
#define FRAME_S10         120   /* x26 */
#define FRAME_S11         124   /* x27 */

/* ── FP-register block (only present in ilp32f / ilp32d builds) ──────────── */
/* Eager save: all 32 FP regs + fcsr saved on every context switch.          */
/* FRAME_SIZE is rounded up to the next multiple of 16 to keep stack ABI     */
/* alignment across the C call to TaktHalTrapDispatch.                       */

#if defined(__riscv_flen) && (__riscv_flen == 64)
  /* ilp32d: 32 × 8 = 256 B for F regs, +4 B for fcsr = 260 B; pad to 272.   */
  #define FRAME_F_BASE      FRAME_GP_SIZE
  #define FRAME_F_STRIDE    8
  #define FRAME_FCSR        (FRAME_F_BASE + 32*FRAME_F_STRIDE)   /* 384 */
  #define FRAME_SIZE        400         /* 128 + 260 → next mult of 16 */

#elif defined(__riscv_flen) && (__riscv_flen == 32)
  /* ilp32f: 32 × 4 = 128 B for F regs, +4 B for fcsr = 132 B; pad to 144.   */
  #define FRAME_F_BASE      FRAME_GP_SIZE
  #define FRAME_F_STRIDE    4
  #define FRAME_FCSR        (FRAME_F_BASE + 32*FRAME_F_STRIDE)   /* 256 */
  #define FRAME_SIZE        272         /* 128 + 132 → next mult of 16 */

#else
  /* ilp32: GP regs only. */
  #define FRAME_SIZE        FRAME_GP_SIZE
#endif

/* Exposed to TaktKernelCore.h for stack-overhead computation. */
#define TAKT_RISCV_THREAD_INIT_FRAME_SIZE  FRAME_SIZE

/* ── Initial mstatus value placed in a fresh task frame ──────────────────── */
/* MPP  [12:11] = 11 (M-mode)                                                 */
/* MPIE [7]     = 1  (mret will set MIE = 1, task starts with IRQs on)        */
#define MSTATUS_TASK_INIT ((3 << 11) | (1 << 7))    /* 0x1880 */

/* ── TaktKernelCtx_t offsets ─────────────────────────────────────────────── */
/* struct __TaktKernelCtx { TaktOSThread_t* pCurrent; TaktOSThread_t* pNextThread; }; */
#define CTX_CURRENT       0
#define CTX_NEXT_THREAD   4

/* ── TCB offsets ─────────────────────────────────────────────────────────── */
/* Must stay in sync with TaktOSThread_t in TaktOSThread.h.                   */
/* Validated by static_assert in task_init_riscv.cpp.                         */
#define TCB_SP            0     /* offsetof(TaktOSThread_t, pSp) */

#endif /* __PORT_RV32_H__ */
