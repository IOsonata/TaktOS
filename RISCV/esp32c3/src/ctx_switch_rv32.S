/* ---------------------------------------------------------------------------
 * TaktOS — ESP32-C3 / ESP32-C6 RISC-V context switch
 * RISCV/esp32c3/ctx_switch_rv32.S
 *
 * Contains two functions:
 *   _takt_trap_rv32    — unified machine-mode trap handler (mtvec target)
 *   TaktOSStartFirst   — first task launch, never returns
 *
 * Interrupt assignment (configured by TaktOSTickInit / intmtx_esp32c3.h):
 *   CPU INT 2  — SYSTIMER_TARGET0 tick    (level, priority 2)
 *   CPU INT 29 — CPU_INTR_FROM_CPU_0 ctx  (edge,  priority 1)
 *
 * Both fire as machine external interrupts (mcause = 0x8000000B).
 * The active CPU INT is identified by reading INTMTX_CPU_INT_EIP_REG.
 *
 * ── Context frame layout ────────────────────────────────────────────────────
 * 128 bytes (32 words) allocated on the current task's stack.
 * x0 (zero) is never saved.  x2 (sp) is stored directly in TCB.pSp.
 *
 *  Byte   Reg       ABI name  Macro
 *  0      mepc      —         FRAME_MEPC
 *  4      mstatus   —         FRAME_MSTATUS
 *  8      x1        ra        FRAME_RA
 *  12     x3        gp        FRAME_GP
 *  16     x4        tp        FRAME_TP
 *  20     x5        t0        FRAME_T0
 *  24     x6        t1        FRAME_T1
 *  28     x7        t2        FRAME_T2
 *  32     x10       a0        FRAME_A0
 *  36     x11       a1        FRAME_A1
 *  40     x12       a2        FRAME_A2
 *  44     x13       a3        FRAME_A3
 *  48     x14       a4        FRAME_A4
 *  52     x15       a5        FRAME_A5
 *  56     x16       a6        FRAME_A6
 *  60     x17       a7        FRAME_A7
 *  64     x28       t3        FRAME_T3
 *  68     x29       t4        FRAME_T4
 *  72     x30       t5        FRAME_T5
 *  76     x31       t6        FRAME_T6
 *  80     x8        s0        FRAME_S0
 *  84     x9        s1        FRAME_S1
 *  88     x18       s2        FRAME_S2
 *  92     x19       s3        FRAME_S3
 *  96     x20       s4        FRAME_S4
 * 100     x21       s5        FRAME_S5
 * 104     x22       s6        FRAME_S6
 * 108     x23       s7        FRAME_S7
 * 112     x24       s8        FRAME_S8
 * 116     x25       s9        FRAME_S9
 * 120     x26       s10       FRAME_S10
 * 124     x27       s11       FRAME_S11
 *
 * ── Dispatch flow ───────────────────────────────────────────────────────────
 *
 *  _takt_trap_rv32
 *    verify mcause bit 31 (interrupt) + cause code 11 (external)
 *    read  INTMTX_CPU_INT_EIP_REG
 *    bit 2 set → tick path:   save full frame, call TaktKernelTickHandler,
 *                              clear SYSTIMER INT, restore frame, mret
 *                              (if TaktOSCtxSwitch was called inside handler,
 *                               CPU INT 29 is now pending; it fires after mret)
 *    bit 29 set → ctx path:   save full frame to current TCB,
 *                              switch g_TaktosCtx.pCurrent = pNextThread,
 *                              clear CPU_INTR_FROM_CPU0,
 *                              load full frame from next TCB, mret
 *
 * ── Cycle budget (Zbb, single-issue in-order, no cache misses) ──────────────
 *  Save frame     : 32 sw + 2 csrr + addi = ~37 cycles
 *  Dispatch       : mcause read + branch + EIP read + branch = ~8 cycles
 *  Switch work    : 4 loads + 4 stores + la + lw x2 = ~12 cycles
 *  Restore frame  : 32 lw + 2 csrw + addi + mret = ~38 cycles
 *  Total ctx sw   : ~95 cycles  (design target ≤108cy, validated by mcycle)
 *
 * ── Hazards & correctness notes ─────────────────────────────────────────────
 *  - We save x5 (t0) BEFORE using it as a scratch register (line "sw x5…"
 *    precedes all csrr instructions).
 *  - x3 (gp) and x4 (tp) are saved even though they are fixed in single-
 *    address-space bare-metal.  The cost is 2 sw + 2 lw ≈ 4 cycles and
 *    eliminates a correctness risk if the compiler ever generates tp-relative
 *    accesses for TLS.
 *  - Critical section: the full frame is saved with MIE = 0 (hardware clears
 *    mstatus.MIE on trap entry).  MIE is restored from mstatus.MPIE by mret.
 *
 * TaktKernelCtx_t layout (taktos.cpp — static_assert enforced):
 *   offset 0: pCurrent     TaktOSThread_t*
 *   offset 4: pNextThread  TaktOSThread_t*
 *
 * TCB layout (TaktOSThread.h — static_assert enforced):
 *   offset  0: pSp          — saved stack pointer (loaded by this handler)
 *   offset 16: pStackBottom — stack floor for PMP guard (TAKTOS_PMP_ENABLE=1)
 *
 * @license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
 * ---------------------------------------------------------------------------
 */

        .section .text, "ax"
        .option norelax                /* suppress GP-relative relaxation     */

/* ── Frame offset constants ────────────────────────────────────────────────── */
#define FRAME_SIZE       128
#define FRAME_MEPC       0
#define FRAME_MSTATUS    4
#define FRAME_RA         8
#define FRAME_GP         12
#define FRAME_TP         16
#define FRAME_T0         20
#define FRAME_T1         24
#define FRAME_T2         28
#define FRAME_A0         32
#define FRAME_A1         36
#define FRAME_A2         40
#define FRAME_A3         44
#define FRAME_A4         48
#define FRAME_A5         52
#define FRAME_A6         56
#define FRAME_A7         60
#define FRAME_T3         64
#define FRAME_T4         68
#define FRAME_T5         72
#define FRAME_T6         76
#define FRAME_S0         80
#define FRAME_S1         84
#define FRAME_S2         88
#define FRAME_S3         92
#define FRAME_S4         96
#define FRAME_S5         100
#define FRAME_S6         104
#define FRAME_S7         108
#define FRAME_S8         112
#define FRAME_S9         116
#define FRAME_S10        120
#define FRAME_S11        124

/* ── TaktKernelCtx_t offsets ────────────────────────────────────────────────────── */
#define CTX_CURRENT      0
#define CTX_NEXT_THREAD  4
#define TCB_SP           0
#define TCB_PMP_BASE     16  /* pStackBottom — used when TAKTOS_PMP_ENABLE=1   */

/* ── Platform register constants ────────────────────────────────────────────── */
/* INTMTX_CPU_INT_EIP_REG: pending CPU INT status (read-only) */
#define INTMTX_EIP_ADDR  0x600C210Cu
/* SYSTEM_CPU_INTR_FROM_CPU_0_REG: set/clear deferred ctx switch request */
#define CTX_TRIG_ADDR    0x600C0014u
/* SYSTIMER_INT_CLR_REG: clear SYSTIMER TARGET0 interrupt */
#define SYSTIMER_CLR_ADDR 0x60023054u
#define SYSTIMER_INT_T0  0x1u

/* CPU INT bit positions */
#define TICK_CPU_INT_BIT (1u << 2)
#define CTX_CPU_INT_BIT  (1u << 29)

        .extern g_TaktosCtx
        .extern TaktKernelTickHandler


/* ===========================================================================
 * _takt_trap_rv32 — unified machine-mode trap entry (mtvec target)
 *
 * Called on ALL machine-mode traps (both exceptions and interrupts).
 * This handler only handles machine external interrupts (mcause = 0x8000000B).
 * All other causes are routed to _takt_trap_unhandled.
 * =========================================================================== */

        .global _takt_trap_rv32
        .type   _takt_trap_rv32, %function
        .align  2

_takt_trap_rv32:
        /* ── 1. Save x5 (t0) BEFORE using it as a scratch register ─────── */
        /* We need t0 for csrr; save it first so the original value is safe.  */
        addi    sp, sp, -FRAME_SIZE
        sw      x5, FRAME_T0(sp)          /* original t0 safely parked       */

        /* ── 2. Save mepc and mstatus (use t0 as scratch) ─────────────── */
        csrr    x5, mepc
        sw      x5, FRAME_MEPC(sp)
        csrr    x5, mstatus
        sw      x5, FRAME_MSTATUS(sp)

        /* ── 3. Save all other working registers ──────────────────────── */
        sw      x1,  FRAME_RA(sp)
        sw      x3,  FRAME_GP(sp)
        sw      x4,  FRAME_TP(sp)
        /* x5 already saved above */
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

        /* ── 4. Verify: must be an interrupt (mcause bit 31 set) ───────── */
        csrr    t0, mcause
        bgez    t0, _takt_trap_unhandled  /* bit 31 clear = synchronous exception */

        /* ── 5. Verify cause code = 11 (machine external interrupt) ────── */
        slli    t0, t0, 1                 /* clear bit 31                    */
        srli    t0, t0, 1                 /* t0 = cause code                 */
        li      t1, 11
        bne     t0, t1, _takt_trap_unhandled

        /* ── 6. Identify which CPU INT fired via EIP status register ────── */
        li      t0, INTMTX_EIP_ADDR
        lw      t1, 0(t0)                 /* t1 = pending CPU INT bitmask    */

        /* ── 7. Dispatch: tick (CPU INT 2) has higher priority; check first */
        li      t0, TICK_CPU_INT_BIT
        and     t0, t1, t0
        bnez    t0, _takt_tick_handler

        li      t0, CTX_CPU_INT_BIT
        and     t0, t1, t0
        bnez    t0, _takt_ctx_handler

        /* Neither tick nor ctx — unexpected CPU INT. Fall through to unhandled. */

_takt_trap_unhandled:
        /* Catch-all: re-raise as a software trap for debugger visibility.    */
        /* Application may override _takt_trap_unhandled if needed.           */
        j       _takt_trap_unhandled     /* intentional infinite loop         */


/* ===========================================================================
 * _takt_tick_handler — SYSTIMER TARGET0 tick path
 *
 * Frame is already saved on task stack.  Call TaktKernelTickHandler()
 * (a C function — RISC-V ABI: arguments in a0-a7, return in a0-a1).
 * TaktKernelTickHandler may call TaktOSCtxSwitch() which sets CPU INT 29
 * pending.  After mret, if CPU INT 29 is pending it fires immediately
 * (priority 1 > threshold 0), triggering _takt_ctx_handler to do the
 * actual register-level switch.
 * =========================================================================== */

        .align  2
_takt_tick_handler:
        /* Clear the SYSTIMER TARGET0 interrupt flag (before calling C handler
         * to prevent immediate re-entry on mret).                             */
        li      t0, SYSTIMER_CLR_ADDR
        li      t1, SYSTIMER_INT_T0
        sw      t1, 0(t0)

        /* Call TaktKernelTickHandler() — C calling convention.
         * Callee-save registers (s0-s11) are already on the stack frame so
         * the C function can clobber them; they will be restored on frame pop. */
        call    TaktKernelTickHandler

        /* Restore frame and mret.  If CPU INT 29 is pending it fires after   */
        /* the mret completes, switching tasks without returning to the task.  */
        j       _takt_restore_and_return


/* ===========================================================================
 * _takt_ctx_handler — deferred context switch path (CPU INT 29)
 *
 * Full register state of the interrupted task is already on stack.
 * Performs the scheduler switch:
 *   1. Save current sp to current TCB.pSp.
 *   2. Load next TCB from g_TaktosCtx.pNextThread.
 *   3. Update g_TaktosCtx.pCurrent = pNextThread.
 *   4. Load new sp from next TCB.pSp.
 *   5. Clear CPU_INTR_FROM_CPU_0 (acknowledge the deferred request).
 *   6. Optionally: apply PMP guard for incoming task (TAKTOS_PMP_ENABLE=1).
 *   7. Restore full frame from new task's stack.
 *   8. mret — resumes execution of the new task.
 * =========================================================================== */

        .align  2
_takt_ctx_handler:
        /* ── a. Save current sp → current TCB.pSp ─────────────────────── */
        la      t0, g_TaktosCtx                /* t0 = &g_TaktosCtx          */
        lw      t1, CTX_CURRENT(t0)            /* t1 = pCurrent              */
        sw      sp, TCB_SP(t1)                 /* pCurrent->pSp = sp         */

        /* ── b. Switch to next thread ─────────────────────────────────── */
        lw      t2, CTX_NEXT_THREAD(t0)        /* t2 = pNextThread           */
        sw      t2, CTX_CURRENT(t0)            /* pCurrent = pNextThread     */
        lw      sp, TCB_SP(t2)                 /* sp = pNextThread->pSp      */

        /* ── c. Acknowledge the context-switch request ──────────────────── */
        li      t0, CTX_TRIG_ADDR
        sw      zero, 0(t0)                    /* clear CPU_INTR_FROM_CPU_0   */

#if TAKTOS_PMP_ENABLE
        /* ── d. Apply PMP guard region for incoming task (optional) ──────── */
        /* pStackBottom is at TCB offset 16.  Configure PMP entry 0 as a      */
        /* NAPOT no-access guard covering 32 bytes at the stack floor.         */
        /* pmpcfg0 bits: L=0, A=NAPOT(11), X=0, W=0, R=0 → 0x18              */
        lw      t0, TCB_PMP_BASE(t2)           /* t0 = pStackBottom           */
        beqz    t0, .Lskip_pmp
        srli    t1, t0, 2                       /* pmpaddr = base >> 2         */
        /* NAPOT encoding for 32-byte region: pmpaddr |= (32/8 - 1) = 0x3    */
        ori     t1, t1, 3
        csrw    pmpaddr0, t1
        li      t1, 0x18                        /* NAPOT, no R/W/X            */
        csrw    pmpcfg0, t1
.Lskip_pmp:
#endif

        /* ── e. Restore full frame of next task and return ──────────────── */
        /* Fall through to _takt_restore_and_return */


/* ===========================================================================
 * _takt_restore_and_return — shared frame restore and mret
 *
 * Restores all saved registers from the frame at sp, pops the frame,
 * restores mepc and mstatus, then issues mret.
 * mret: PC ← mepc; mstatus.MIE ← mstatus.MPIE; mstatus.MPP ← U (or M).
 * For tasks whose initial frame was built with MPIE=1, interrupts are
 * re-enabled automatically on first entry (no manual csrsi needed).
 * =========================================================================== */

_takt_restore_and_return:
        /* Restore mepc and mstatus first (using t0 as scratch, then restoring t0) */
        lw      t0, FRAME_MEPC(sp)
        csrw    mepc, t0
        lw      t0, FRAME_MSTATUS(sp)
        csrw    mstatus, t0

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
        /* Restore t0 (x5) last — it was our scratch register */
        lw      x5,  FRAME_T0(sp)

        addi    sp, sp, FRAME_SIZE     /* pop frame; sp is now task sp        */
        mret                           /* PC←mepc; MIE←MPIE; return to task  */

        .size _takt_trap_rv32, . - _takt_trap_rv32


/* ===========================================================================
 * TaktOSStartFirst — launch the first task (called once by TaktOSStart)
 *
 * g_TaktosCtx.pNextThread is the highest-priority ready task (set by
 * TaktOSInit → TaktReadyTask → UpdateNextThread before TaktOSStart).
 *
 * Sets up pCurrent, loads the initial stack frame built by TaktKernelStackInit,
 * and issues mret to enter the first task with interrupts enabled.
 * The initial frame has mstatus.MPIE=1 so mret enables MIE automatically.
 *
 * Never returns.
 * =========================================================================== */

        .global TaktOSStartFirst
        .type   TaktOSStartFirst, %function
        .align  2

TaktOSStartFirst:
        /* Disable interrupts during first-task setup (should already be off) */
        csrci   mstatus, 0x8

        la      t0, g_TaktosCtx
        lw      t1, CTX_NEXT_THREAD(t0)    /* t1 = first task TCB            */
        sw      t1, CTX_CURRENT(t0)        /* pCurrent = pNextThread         */
        lw      sp, TCB_SP(t1)             /* sp = first task's saved SP     */

#if TAKTOS_PMP_ENABLE
        /* Apply PMP guard for first task before enabling interrupts */
        lw      t0, TCB_PMP_BASE(t1)
        beqz    t0, .Lfirst_skip_pmp
        srli    t2, t0, 2
        ori     t2, t2, 3
        csrw    pmpaddr0, t2
        li      t2, 0x18
        csrw    pmpcfg0, t2
.Lfirst_skip_pmp:
#endif
        /* Load mepc and mstatus from the initial frame built by TaktKernelStackInit.
         * mstatus has MPIE=1 → mret will enable MIE (interrupts on).
         * mepc = task entry function.                                         */
        lw      t0, FRAME_MEPC(sp)
        csrw    mepc, t0
        lw      t0, FRAME_MSTATUS(sp)
        csrw    mstatus, t0

        /* Restore all registers (same sequence as _takt_restore_and_return)  */
        lw      x1,  FRAME_RA(sp)
        lw      x3,  FRAME_GP(sp)
        lw      x4,  FRAME_TP(sp)
        lw      x6,  FRAME_T1(sp)
        lw      x7,  FRAME_T2(sp)
        lw      x10, FRAME_A0(sp)         /* x10 (a0) = task argument        */
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
        lw      x5,  FRAME_T0(sp)         /* t0 last (was scratch)           */

        addi    sp, sp, FRAME_SIZE
        mret                               /* enter first task; never returns */

        .size TaktOSStartFirst, . - TaktOSStartFirst

        .end
