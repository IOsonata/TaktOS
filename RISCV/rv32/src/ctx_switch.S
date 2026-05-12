/* ----------------------------------------------------------------------------
 * TaktOS — RV32 arch context-switch core (HAL-agnostic)
 * RISCV/rv32/ctx_switch.S
 *
 * Provides two arch-level entry points that the per-target HAL plugs into:
 *
 *   TaktArchTrapEntry  — installed (directly or via a HAL-owned vector table)
 *                        as the mtvec target.  Saves the full 128-byte frame,
 *                        calls TaktHalTrapDispatch, performs the scheduler
 *                        swap if requested, restores the frame, and mret's.
 *
 *   TaktOSStartFirst   — called once by TaktOSStart() to launch the highest-
 *                        priority ready task.  Loads its initial frame and
 *                        mret's into it.  Never returns.
 *
 * The HAL provides:
 *   int  TaktHalTrapDispatch(void)   — returns 0 = no switch, 1 = swap
 *   void TaktOSCtxSwitch(void)       — fires the deferred-switch trigger
 *
 * The scheduler context block (g_TaktosCtx) is owned by taktos.cpp:
 *   struct __TaktKernelCtx { TaktOSThread_t* pCurrent; TaktOSThread_t* pNextThread; };
 *
 * The TCB layout has pSp at offset 0 (TCB_SP), validated by static_assert.
 *
 * ── Frame layout ──────────────────────────────────────────────────────────
 *
 * 32 words / 128 bytes built on the interrupted task's stack on every trap
 * entry, also placed by TaktKernelStackInit() so the first switch into a
 * fresh task is indistinguishable from any subsequent one.
 *
 * Offsets and the initial mstatus value are defined in port_rv32.h.
 *
 * x0 is hard-wired zero, never saved.
 * x2 (sp) lives in TCB.pSp, never in the frame.
 *
 * ── Cycle budget (rv32imac, single-issue in-order, no cache misses) ─────
 *  Save frame       : 1 addi + 30 sw + 2 csrr      ~33 cy
 *  Call dispatch    : call + jalr + HAL body       depends on HAL
 *  Swap (if needed) : la + 4 lw/sw                 ~8 cy
 *  Restore frame    : 30 lw + 2 csrw + addi + mret ~34 cy
 *  Arch overhead    :                              ~75 cy + HAL dispatch
 *
 * ── Hazards & correctness notes ─────────────────────────────────────────
 *  - x5 (t0) is saved BEFORE being used as a scratch register for csrr.
 *  - The dispatch function follows the standard RISC-V calling convention.
 *    It can clobber any caller-save register; all callee-save regs (s0–s11)
 *    are already on the saved frame, so no extra save is needed across
 *    the call.
 *
 * @license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
 * ----------------------------------------------------------------------------
 */

        .option arch, +c, +zicsr
#if defined(__riscv_flen) && (__riscv_flen >= 32)
        .option arch, +f
#endif
#if defined(__riscv_flen) && (__riscv_flen >= 64)
        .option arch, +d
#endif

#include "port_rv32.h"

/* ── FP save/restore macros ──────────────────────────────────────────────── */
/* Emitted inline by the save/restore blocks for ilp32f and ilp32d builds.   */
/* fcsr is saved through t0 because csrr cannot target an FP register.       */

#if defined(__riscv_flen) && (__riscv_flen == 64)
  .macro TAKT_FSAVE
        fsd     f0,  FRAME_F_BASE + 0*8(sp)
        fsd     f1,  FRAME_F_BASE + 1*8(sp)
        fsd     f2,  FRAME_F_BASE + 2*8(sp)
        fsd     f3,  FRAME_F_BASE + 3*8(sp)
        fsd     f4,  FRAME_F_BASE + 4*8(sp)
        fsd     f5,  FRAME_F_BASE + 5*8(sp)
        fsd     f6,  FRAME_F_BASE + 6*8(sp)
        fsd     f7,  FRAME_F_BASE + 7*8(sp)
        fsd     f8,  FRAME_F_BASE + 8*8(sp)
        fsd     f9,  FRAME_F_BASE + 9*8(sp)
        fsd     f10, FRAME_F_BASE + 10*8(sp)
        fsd     f11, FRAME_F_BASE + 11*8(sp)
        fsd     f12, FRAME_F_BASE + 12*8(sp)
        fsd     f13, FRAME_F_BASE + 13*8(sp)
        fsd     f14, FRAME_F_BASE + 14*8(sp)
        fsd     f15, FRAME_F_BASE + 15*8(sp)
        fsd     f16, FRAME_F_BASE + 16*8(sp)
        fsd     f17, FRAME_F_BASE + 17*8(sp)
        fsd     f18, FRAME_F_BASE + 18*8(sp)
        fsd     f19, FRAME_F_BASE + 19*8(sp)
        fsd     f20, FRAME_F_BASE + 20*8(sp)
        fsd     f21, FRAME_F_BASE + 21*8(sp)
        fsd     f22, FRAME_F_BASE + 22*8(sp)
        fsd     f23, FRAME_F_BASE + 23*8(sp)
        fsd     f24, FRAME_F_BASE + 24*8(sp)
        fsd     f25, FRAME_F_BASE + 25*8(sp)
        fsd     f26, FRAME_F_BASE + 26*8(sp)
        fsd     f27, FRAME_F_BASE + 27*8(sp)
        fsd     f28, FRAME_F_BASE + 28*8(sp)
        fsd     f29, FRAME_F_BASE + 29*8(sp)
        fsd     f30, FRAME_F_BASE + 30*8(sp)
        fsd     f31, FRAME_F_BASE + 31*8(sp)
        csrr    t0,  fcsr
        sw      t0,  FRAME_FCSR(sp)
  .endm
  .macro TAKT_FRESTORE
        lw      t0,  FRAME_FCSR(sp)
        csrw    fcsr, t0
        fld     f0,  FRAME_F_BASE + 0*8(sp)
        fld     f1,  FRAME_F_BASE + 1*8(sp)
        fld     f2,  FRAME_F_BASE + 2*8(sp)
        fld     f3,  FRAME_F_BASE + 3*8(sp)
        fld     f4,  FRAME_F_BASE + 4*8(sp)
        fld     f5,  FRAME_F_BASE + 5*8(sp)
        fld     f6,  FRAME_F_BASE + 6*8(sp)
        fld     f7,  FRAME_F_BASE + 7*8(sp)
        fld     f8,  FRAME_F_BASE + 8*8(sp)
        fld     f9,  FRAME_F_BASE + 9*8(sp)
        fld     f10, FRAME_F_BASE + 10*8(sp)
        fld     f11, FRAME_F_BASE + 11*8(sp)
        fld     f12, FRAME_F_BASE + 12*8(sp)
        fld     f13, FRAME_F_BASE + 13*8(sp)
        fld     f14, FRAME_F_BASE + 14*8(sp)
        fld     f15, FRAME_F_BASE + 15*8(sp)
        fld     f16, FRAME_F_BASE + 16*8(sp)
        fld     f17, FRAME_F_BASE + 17*8(sp)
        fld     f18, FRAME_F_BASE + 18*8(sp)
        fld     f19, FRAME_F_BASE + 19*8(sp)
        fld     f20, FRAME_F_BASE + 20*8(sp)
        fld     f21, FRAME_F_BASE + 21*8(sp)
        fld     f22, FRAME_F_BASE + 22*8(sp)
        fld     f23, FRAME_F_BASE + 23*8(sp)
        fld     f24, FRAME_F_BASE + 24*8(sp)
        fld     f25, FRAME_F_BASE + 25*8(sp)
        fld     f26, FRAME_F_BASE + 26*8(sp)
        fld     f27, FRAME_F_BASE + 27*8(sp)
        fld     f28, FRAME_F_BASE + 28*8(sp)
        fld     f29, FRAME_F_BASE + 29*8(sp)
        fld     f30, FRAME_F_BASE + 30*8(sp)
        fld     f31, FRAME_F_BASE + 31*8(sp)
  .endm

#elif defined(__riscv_flen) && (__riscv_flen == 32)
  .macro TAKT_FSAVE
        fsw     f0,  FRAME_F_BASE + 0*4(sp)
        fsw     f1,  FRAME_F_BASE + 1*4(sp)
        fsw     f2,  FRAME_F_BASE + 2*4(sp)
        fsw     f3,  FRAME_F_BASE + 3*4(sp)
        fsw     f4,  FRAME_F_BASE + 4*4(sp)
        fsw     f5,  FRAME_F_BASE + 5*4(sp)
        fsw     f6,  FRAME_F_BASE + 6*4(sp)
        fsw     f7,  FRAME_F_BASE + 7*4(sp)
        fsw     f8,  FRAME_F_BASE + 8*4(sp)
        fsw     f9,  FRAME_F_BASE + 9*4(sp)
        fsw     f10, FRAME_F_BASE + 10*4(sp)
        fsw     f11, FRAME_F_BASE + 11*4(sp)
        fsw     f12, FRAME_F_BASE + 12*4(sp)
        fsw     f13, FRAME_F_BASE + 13*4(sp)
        fsw     f14, FRAME_F_BASE + 14*4(sp)
        fsw     f15, FRAME_F_BASE + 15*4(sp)
        fsw     f16, FRAME_F_BASE + 16*4(sp)
        fsw     f17, FRAME_F_BASE + 17*4(sp)
        fsw     f18, FRAME_F_BASE + 18*4(sp)
        fsw     f19, FRAME_F_BASE + 19*4(sp)
        fsw     f20, FRAME_F_BASE + 20*4(sp)
        fsw     f21, FRAME_F_BASE + 21*4(sp)
        fsw     f22, FRAME_F_BASE + 22*4(sp)
        fsw     f23, FRAME_F_BASE + 23*4(sp)
        fsw     f24, FRAME_F_BASE + 24*4(sp)
        fsw     f25, FRAME_F_BASE + 25*4(sp)
        fsw     f26, FRAME_F_BASE + 26*4(sp)
        fsw     f27, FRAME_F_BASE + 27*4(sp)
        fsw     f28, FRAME_F_BASE + 28*4(sp)
        fsw     f29, FRAME_F_BASE + 29*4(sp)
        fsw     f30, FRAME_F_BASE + 30*4(sp)
        fsw     f31, FRAME_F_BASE + 31*4(sp)
        csrr    t0,  fcsr
        sw      t0,  FRAME_FCSR(sp)
  .endm
  .macro TAKT_FRESTORE
        lw      t0,  FRAME_FCSR(sp)
        csrw    fcsr, t0
        flw     f0,  FRAME_F_BASE + 0*4(sp)
        flw     f1,  FRAME_F_BASE + 1*4(sp)
        flw     f2,  FRAME_F_BASE + 2*4(sp)
        flw     f3,  FRAME_F_BASE + 3*4(sp)
        flw     f4,  FRAME_F_BASE + 4*4(sp)
        flw     f5,  FRAME_F_BASE + 5*4(sp)
        flw     f6,  FRAME_F_BASE + 6*4(sp)
        flw     f7,  FRAME_F_BASE + 7*4(sp)
        flw     f8,  FRAME_F_BASE + 8*4(sp)
        flw     f9,  FRAME_F_BASE + 9*4(sp)
        flw     f10, FRAME_F_BASE + 10*4(sp)
        flw     f11, FRAME_F_BASE + 11*4(sp)
        flw     f12, FRAME_F_BASE + 12*4(sp)
        flw     f13, FRAME_F_BASE + 13*4(sp)
        flw     f14, FRAME_F_BASE + 14*4(sp)
        flw     f15, FRAME_F_BASE + 15*4(sp)
        flw     f16, FRAME_F_BASE + 16*4(sp)
        flw     f17, FRAME_F_BASE + 17*4(sp)
        flw     f18, FRAME_F_BASE + 18*4(sp)
        flw     f19, FRAME_F_BASE + 19*4(sp)
        flw     f20, FRAME_F_BASE + 20*4(sp)
        flw     f21, FRAME_F_BASE + 21*4(sp)
        flw     f22, FRAME_F_BASE + 22*4(sp)
        flw     f23, FRAME_F_BASE + 23*4(sp)
        flw     f24, FRAME_F_BASE + 24*4(sp)
        flw     f25, FRAME_F_BASE + 25*4(sp)
        flw     f26, FRAME_F_BASE + 26*4(sp)
        flw     f27, FRAME_F_BASE + 27*4(sp)
        flw     f28, FRAME_F_BASE + 28*4(sp)
        flw     f29, FRAME_F_BASE + 29*4(sp)
        flw     f30, FRAME_F_BASE + 30*4(sp)
        flw     f31, FRAME_F_BASE + 31*4(sp)
  .endm

#else
  .macro TAKT_FSAVE
  .endm
  .macro TAKT_FRESTORE
  .endm
#endif

        .extern g_TaktosCtx
        .extern TaktHalTrapDispatch

/* ============================================================================
 * TaktArchTrapEntry — generic machine-mode trap entry
 *
 * Saves the full 128-byte frame on the current task's stack, calls into the
 * HAL to identify and service the cause, swaps stacks if the HAL signals a
 * switch is needed, restores the frame, and returns via mret.
 *
 * The HAL is responsible for:
 *   - decoding mcause / identifying which source fired
 *   - calling any per-cause handler (e.g. TaktKernelTickHandler)
 *   - clearing any interrupt latches
 *   - returning 1 if the scheduler has selected a different task
 *     (g_TaktosCtx.pNextThread != g_TaktosCtx.pCurrent), else 0
 *
 * Entered with interrupts disabled by hardware (mstatus.MIE saved into MPIE
 * by the trap mechanism).  mret restores MIE on the way back.
 * ============================================================================ */

        .section .text.TaktArchTrapEntry, "ax"
        .global TaktArchTrapEntry
        .type   TaktArchTrapEntry, %function
        .align  2

TaktArchTrapEntry:
        /* ── 1. Save x5 (t0) BEFORE using it as a scratch for csrr ──────── */
        addi    sp, sp, -FRAME_SIZE
        sw      x5,  FRAME_T0(sp)

        /* ── 2. Save mepc and mstatus (use t0 as scratch) ───────────────── */
        csrr    x5,  mepc
        sw      x5,  FRAME_MEPC(sp)
        csrr    x5,  mstatus
        sw      x5,  FRAME_MSTATUS(sp)

        /* ── 3. Save all remaining live registers ───────────────────────── */
        sw      x1,  FRAME_RA(sp)
        sw      x3,  FRAME_GP(sp)
        sw      x4,  FRAME_TP(sp)
        /* x5 (t0) already saved */
        sw      x6,  FRAME_T1(sp)
        sw      x7,  FRAME_T2(sp)
        sw      x10, FRAME_A0(sp)
        sw      x11, FRAME_A1(sp)
        sw      x12, FRAME_A2(sp)
        sw      x13, FRAME_A3(sp)
        sw      x14, FRAME_A4(sp)
        sw      x15, FRAME_A5(sp)
        sw      x16, FRAME_A6(sp)
        sw      x17, FRAME_A7(sp)
        sw      x28, FRAME_T3(sp)
        sw      x29, FRAME_T4(sp)
        sw      x30, FRAME_T5(sp)
        sw      x31, FRAME_T6(sp)
        sw      x8,  FRAME_S0(sp)
        sw      x9,  FRAME_S1(sp)
        sw      x18, FRAME_S2(sp)
        sw      x19, FRAME_S3(sp)
        sw      x20, FRAME_S4(sp)
        sw      x21, FRAME_S5(sp)
        sw      x22, FRAME_S6(sp)
        sw      x23, FRAME_S7(sp)
        sw      x24, FRAME_S8(sp)
        sw      x25, FRAME_S9(sp)
        sw      x26, FRAME_S10(sp)
        sw      x27, FRAME_S11(sp)

        /* ── 3b. Save FP register block (ilp32f / ilp32d only — no-op for ilp32) */
        TAKT_FSAVE

        /* ── 4. Call HAL dispatch ──────────────────────────────────────── */
        /* Standard RISC-V calling convention.  Returns:                    */
        /*   a0 = 0  → no switch, just restore current task                 */
        /*   a0 = 1  → swap to g_TaktosCtx.pNextThread                       */
        /* HAL is responsible for clearing all peripheral interrupt latches */
        /* and for invoking any handler such as TaktKernelTickHandler.       */
        call    TaktHalTrapDispatch

        beqz    a0, TaktArchRestoreAndReturn

        /* ── 5. Scheduler swap ─────────────────────────────────────────── */
        /*   pCurrent->pSp = sp;                                            */
        /*   pCurrent = pNextThread;                                        */
        /*   sp = pCurrent->pSp;                                            */
        la      t0, g_TaktosCtx
        lw      t1, CTX_CURRENT(t0)
        sw      sp, TCB_SP(t1)
        lw      t2, CTX_NEXT_THREAD(t0)
        sw      t2, CTX_CURRENT(t0)
        lw      sp, TCB_SP(t2)

        /* Fall through to restore */

/* ============================================================================
 * TaktArchRestoreAndReturn — shared frame restore + mret
 *
 * Restores all saved registers from the frame at sp, pops the frame, restores
 * mepc and mstatus, then issues mret.  mret atomically sets:
 *   PC          ← mepc
 *   mstatus.MIE ← mstatus.MPIE
 *   mstatus.MPP ← (reverts to U-mode if implemented, M-mode otherwise)
 *
 * For a fresh task whose initial frame has MPIE=1, mret enables MIE on first
 * entry — no explicit csrsi needed.
 * ============================================================================ */

TaktArchRestoreAndReturn:
        /* Restore FP block first (uses t0 internally for fcsr) — no-op for ilp32 */
        TAKT_FRESTORE

        /* Restore mepc and mstatus (use t0 as scratch, restored last) */
        lw      x5,  FRAME_MEPC(sp)
        csrw    mepc, x5
        lw      x5,  FRAME_MSTATUS(sp)
        csrw    mstatus, x5

        /* Restore all general-purpose registers except sp (x2) */
        lw      x1,  FRAME_RA(sp)
        lw      x3,  FRAME_GP(sp)
        lw      x4,  FRAME_TP(sp)
        lw      x6,  FRAME_T1(sp)
        lw      x7,  FRAME_T2(sp)
        lw      x10, FRAME_A0(sp)
        lw      x11, FRAME_A1(sp)
        lw      x12, FRAME_A2(sp)
        lw      x13, FRAME_A3(sp)
        lw      x14, FRAME_A4(sp)
        lw      x15, FRAME_A5(sp)
        lw      x16, FRAME_A6(sp)
        lw      x17, FRAME_A7(sp)
        lw      x28, FRAME_T3(sp)
        lw      x29, FRAME_T4(sp)
        lw      x30, FRAME_T5(sp)
        lw      x31, FRAME_T6(sp)
        lw      x8,  FRAME_S0(sp)
        lw      x9,  FRAME_S1(sp)
        lw      x18, FRAME_S2(sp)
        lw      x19, FRAME_S3(sp)
        lw      x20, FRAME_S4(sp)
        lw      x21, FRAME_S5(sp)
        lw      x22, FRAME_S6(sp)
        lw      x23, FRAME_S7(sp)
        lw      x24, FRAME_S8(sp)
        lw      x25, FRAME_S9(sp)
        lw      x26, FRAME_S10(sp)
        lw      x27, FRAME_S11(sp)
        /* Restore x5 (t0) last — used above as scratch for csrw */
        lw      x5,  FRAME_T0(sp)

        addi    sp,  sp, FRAME_SIZE
        mret

        .size TaktArchTrapEntry, . - TaktArchTrapEntry


/* ============================================================================
 * TaktOSStartFirst — launch the first task
 *
 * Called once by TaktOSStart() after TaktOSInit has populated g_TaktosCtx
 * with the highest-priority ready task.  Identical structural shape to the
 * "swap then restore" tail of TaktArchTrapEntry, except there's no current
 * task to save and no dispatch step.
 *
 * Never returns — control passes via mret into the first task with MIE = 1.
 * ============================================================================ */

        .section .text.TaktOSStartFirst, "ax"
        .global TaktOSStartFirst
        .type   TaktOSStartFirst, %function
        .align  2

TaktOSStartFirst:
        csrci   mstatus, 0x8                /* defensive: ensure MIE off    */

        la      t0, g_TaktosCtx
        lw      t1, CTX_NEXT_THREAD(t0)
        sw      t1, CTX_CURRENT(t0)
        lw      sp, TCB_SP(t1)

        /* Restore FP block first — no-op for ilp32 */
        TAKT_FRESTORE

        /* Restore the initial frame placed by TaktKernelStackInit().       */
        /* mstatus.MPIE = 1 → mret will enable MIE.                          */
        lw      x5,  FRAME_MEPC(sp)
        csrw    mepc, x5
        lw      x5,  FRAME_MSTATUS(sp)
        csrw    mstatus, x5

        lw      x1,  FRAME_RA(sp)
        lw      x3,  FRAME_GP(sp)
        lw      x4,  FRAME_TP(sp)
        lw      x6,  FRAME_T1(sp)
        lw      x7,  FRAME_T2(sp)
        lw      x10, FRAME_A0(sp)
        lw      x11, FRAME_A1(sp)
        lw      x12, FRAME_A2(sp)
        lw      x13, FRAME_A3(sp)
        lw      x14, FRAME_A4(sp)
        lw      x15, FRAME_A5(sp)
        lw      x16, FRAME_A6(sp)
        lw      x17, FRAME_A7(sp)
        lw      x28, FRAME_T3(sp)
        lw      x29, FRAME_T4(sp)
        lw      x30, FRAME_T5(sp)
        lw      x31, FRAME_T6(sp)
        lw      x8,  FRAME_S0(sp)
        lw      x9,  FRAME_S1(sp)
        lw      x18, FRAME_S2(sp)
        lw      x19, FRAME_S3(sp)
        lw      x20, FRAME_S4(sp)
        lw      x21, FRAME_S5(sp)
        lw      x22, FRAME_S6(sp)
        lw      x23, FRAME_S7(sp)
        lw      x24, FRAME_S8(sp)
        lw      x25, FRAME_S9(sp)
        lw      x26, FRAME_S10(sp)
        lw      x27, FRAME_S11(sp)
        lw      x5,  FRAME_T0(sp)

        addi    sp,  sp, FRAME_SIZE
        mret                                /* never returns                */

        .size TaktOSStartFirst, . - TaktOSStartFirst

        .end
