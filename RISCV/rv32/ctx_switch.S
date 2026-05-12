// ─────────────────────────────────────────────────────────────────────────────
// ctx_switch.s  —  TaktOS RISC-V RV32 context switch handler
//
// Entered as machine-mode software interrupt (mcause = 3), triggered by
// TriggerContextSwitch() writing CLINT MSIP = 1.
//
// Full context frame: 34 words × 4 bytes = 136 bytes
//   word  0: mepc        (interrupted PC)
//   word  1: mstatus     (MIE/MPIE bits)
//   words 2-33: x1-x31  (all general-purpose registers except x0)
//
// No hardware frame push on RISC-V — all registers saved manually.
//
// Safety boundary: IN — full branch + line coverage required.
// LOC: ~60 (excluding comments).
// Cycle budget: ~88cy (Zbb) / ~98cy (no Zbb) — see Engineering Spec §5.4.
// ─────────────────────────────────────────────────────────────────────────────

#include "port_rv32.h"      // CTX_* offset constants

    .section .text
    .global  _takt_ctx_switch_rv
    .type    _takt_ctx_switch_rv, @function
    .align   2

_takt_ctx_switch_rv:

    // ── Step 1: Allocate frame, save x5 (t0) FIRST, then save mepc/mstatus ────
    // CRITICAL ORDER: x5 must be saved before the first csrr that clobbers t0
    // with the mepc value.  Saving after would restore mstatus into the
    // interrupted task's t0 register, silently corrupting its state.
    addi    sp, sp, -CTX_FRAME_BYTES       // 34 words = 136 bytes

    sw      x5,   CTX_X5(sp)     // t0 — save FIRST, before csrr clobbers it

    csrr    t0, mepc              // now safe to use t0 as scratch
    sw      t0,  CTX_MEPC(sp)

    csrr    t0, mstatus
    sw      t0,  CTX_MSTATUS(sp)

    // ── Step 2: Save remaining general-purpose registers ─────────────────────
    // x0 = always zero, never saved.  x5 already saved in Step 1 above.
    sw      x1,   CTX_X1(sp)     // ra
    // x5 saved above — do not repeat
    sw      x6,   CTX_X6(sp)     // t1
    sw      x7,   CTX_X7(sp)     // t2
    sw      x8,   CTX_X8(sp)     // s0/fp
    sw      x9,   CTX_X9(sp)     // s1
    sw      x10,  CTX_X10(sp)    // a0
    sw      x11,  CTX_X11(sp)    // a1
    sw      x12,  CTX_X12(sp)    // a2
    sw      x13,  CTX_X13(sp)    // a3
    sw      x14,  CTX_X14(sp)    // a4
    sw      x15,  CTX_X15(sp)    // a5
    sw      x16,  CTX_X16(sp)    // a6
    sw      x17,  CTX_X17(sp)    // a7
    sw      x18,  CTX_X18(sp)    // s2
    sw      x19,  CTX_X19(sp)    // s3
    sw      x20,  CTX_X20(sp)    // s4
    sw      x21,  CTX_X21(sp)    // s5
    sw      x22,  CTX_X22(sp)    // s6
    sw      x23,  CTX_X23(sp)    // s7
    sw      x24,  CTX_X24(sp)    // s8
    sw      x25,  CTX_X25(sp)    // s9
    sw      x26,  CTX_X26(sp)    // s10
    sw      x27,  CTX_X27(sp)    // s11
    sw      x28,  CTX_X28(sp)    // t3
    sw      x29,  CTX_X29(sp)    // t4
    sw      x30,  CTX_X30(sp)    // t5
    sw      x31,  CTX_X31(sp)    // t6

    // ── Step 3: Save current task SP into TCB ─────────────────────────────────
    // g_TaktosCtx is a packed {pCurrent*, pNextThread*} at offsets 0 and 4.
    la      t0, g_TaktosCtx
    lw      t1, CTX_CURRENT(t0)            // t1 = currentTCB*
    sw      sp, 0(t1)                      // TCB.sp = sp  (sp @ TCB offset 0)

    // ── Step 4: Switch to next task ───────────────────────────────────────────
    // next_thread was updated by the C++ scheduler (UpdateNextThread) when the
    // last ready-list mutation happened — no bitmap scan needed here.
    lw      t2, CTX_NEXT_THREAD(t0)        // t2 = nextTCB* (precomputed)
    beqz    t2, .use_current               // idle guard: should never fire
    sw      t2, CTX_CURRENT(t0)            // current = next
    lw      sp, 0(t2)                      // sp = nextTCB.sp
    j       .switch_done

.use_current:
    // Safety net: if next_thread is null keep running the current task.
    // This branch must never fire in a correctly configured system.
    lw      sp, 0(t1)

.switch_done:

    // ── Step 5: Clear CLINT MSIP (acknowledge software interrupt) ─────────────
    li      t0, TAKT_CLINT_BASE
    sw      zero, 0(t0)

    // ── Step 6: Optional PMP stack guard reload ───────────────────────────────
    // When TAKT_PMP_ENABLE=1, write the next task's stack guard into a
    // dedicated PMP entry (region 6, NAPOT, no-access, 32-byte aligned).
    // pStackBottom (TCB_STACK_BOTTOM) holds the 32-byte-aligned stack floor —
    // the same field used by ARM MPU on the Cortex-M ports.
    // _takt_ApplyPMP receives the stack floor address in a0.
#if TAKT_PMP_ENABLE
    lw      t0, CTX_NEXT_THREAD(t0)        // next TCB* (already switched above)
    lw      a0, TCB_STACK_BOTTOM(t0)       // pStackBottom — 32-byte-aligned floor
    call    _takt_ApplyPMP                 // configure PMP stack guard
#endif

    // ── Step 7: Restore mepc and mstatus ─────────────────────────────────────
    lw      t0, CTX_MEPC(sp)
    csrw    mepc, t0

    lw      t0, CTX_MSTATUS(sp)
    csrw    mstatus, t0

    // ── Step 8: Restore all general-purpose registers ─────────────────────────
    lw      x1,   CTX_X1(sp)
    lw      x5,   CTX_X5(sp)
    lw      x6,   CTX_X6(sp)
    lw      x7,   CTX_X7(sp)
    lw      x8,   CTX_X8(sp)
    lw      x9,   CTX_X9(sp)
    lw      x10,  CTX_X10(sp)
    lw      x11,  CTX_X11(sp)
    lw      x12,  CTX_X12(sp)
    lw      x13,  CTX_X13(sp)
    lw      x14,  CTX_X14(sp)
    lw      x15,  CTX_X15(sp)
    lw      x16,  CTX_X16(sp)
    lw      x17,  CTX_X17(sp)
    lw      x18,  CTX_X18(sp)
    lw      x19,  CTX_X19(sp)
    lw      x20,  CTX_X20(sp)
    lw      x21,  CTX_X21(sp)
    lw      x22,  CTX_X22(sp)
    lw      x23,  CTX_X23(sp)
    lw      x24,  CTX_X24(sp)
    lw      x25,  CTX_X25(sp)
    lw      x26,  CTX_X26(sp)
    lw      x27,  CTX_X27(sp)
    lw      x28,  CTX_X28(sp)
    lw      x29,  CTX_X29(sp)
    lw      x30,  CTX_X30(sp)
    lw      x31,  CTX_X31(sp)

    // ── Step 9: Free frame and return from machine-mode trap ──────────────────
    addi    sp, sp, CTX_FRAME_BYTES
    mret                                   // restores MIE from mstatus.MPIE

    .size _takt_ctx_switch_rv, . - _takt_ctx_switch_rv
