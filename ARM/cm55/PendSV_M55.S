/* ─────────────────────────────────────────────────────────────────────────────
 * TaktOS — Cortex-M55 context switch
 * ARM/cm55/PendSV_M55.S
 *
 * Contains five functions:
 *   PendSV_Handler      — steady-state context switch (PSPLIM optional)
 *   PendSV_Handler_MPU  — steady-state context switch (PSPLIM + MPU region 7,
 *                         both always applied)
 *   SVC_Handler         — first task launch (PSPLIM optional)
 *   SVC_Handler_MPU     — first task launch (PSPLIM + MPU region 7)
 *   TaktOSStartFirst    — issues SVC #0 to trigger first launch; never returns
 *
 * Design:
 *   Superset of M33. Same GP register save/restore core (precomputed
 *   pNextThread, paired LDR stall hiding, TaktOSCtxSwitch() inlined in C).
 *   Adds MVE (Helium) vector context and PSPLIM on top.
 *
 *   ARMv8.1-M with Helium (M-Profile Vector Extension).
 *   FPv5-SP-D16 (single-precision, S0–S31 / D0–D15).
 *   No TrustZone use in v1.0 (NS world only).
 *
 *   PendSV_Handler_MPU / SVC_Handler_MPU are bound by TaktKernelHandlerAssign
 *   when the application relocates its vector table to RAM and passes the
 *   RAM base to TaktOSInit.  The MPU variants ALWAYS write PSPLIM (no gate),
 *   since MPU stack guard + PSPLIM are complementary and always wanted
 *   together on M55.  The default variants keep the TAKTOS_STACK_LIMIT gate.
 *
 * Compile-time gates:
 *   TAKTOS_MVE_CONTEXT=1  save/restore Q0–Q7 (8 × 128-bit = 128 bytes) and VPR.
 *                         Hardware does NOT auto-stack Helium registers.
 *                         Save order: VPR (as GP word via VMRS + STMDB),
 *                           then Q7..Q0 via VSTRW.32 pre-decrement (Q0 at lowest addr).
 *                         Restore order: Q0..Q7 via VLDRW.32 post-increment,
 *                           then VPR via LDM + VMSR.
 *                         IMPORTANT: the VLDRW restore of Q0-Q7 uses LDM r0!, {r3}
 *                           to reload VPR — this clobbers r3 which held the next TCB
 *                           pointer. After the VPR restore, r3 is reloaded from
 *                           [r1, #CTX_CURRENT] (r1 = g_TaktosCtx survives all restores).
 *                         All tasks in a system must share the same gate value.
 *                         Mixed-use (some tasks use MVE, some do not) requires
 *                         TAKTOS_MVE_CONTEXT=1 for all tasks.
 *                         Overhead: ~36cy save + ~36cy restore = ~72cy total.
 *   Always compiled in; runtime-gated by TST lr, #EXC_RETURN_FPU_BIT:
 *                         save/restore S16–S31. Hardware lazy-stacks S0–S15.
 *                         Applied after MVE save / before MVE restore.
 *                         Gate: TST lr, #EXC_RETURN_FPU_BIT; IT EQ; VSTMDB/VLDMIA.
 *                         Overhead: ~18cy.
 *   TAKTOS_STACK_LIMIT=1  write pStackBottom to PSPLIM on every switch.
 *                         r3 is reloaded from g_TaktosCtx if MVE is active
 *                         (see TAKTOS_MVE_CONTEXT note above).
 *                         Overhead: ~3cy (LDR + MSR PSPLIM).
 *   PendSV_Handler_MPU writes MPU region 7 (32-byte RO-priv stack guard, XN=1)
 *                         via RBAR + RLAR on every switch.  Always also writes
 *                         PSPLIM (no gate) — MPU + PSPLIM complement each other.
 *                         Overhead on top of PSPLIM path: ~6cy
 *                         (STR RNR + ORR RBAR + STR + ORR RLAR + STR).
 *
 * MVE frame layout (when TAKTOS_MVE_CONTEXT=1, grows downward from GP frame):
 *   [VPR  — 1 word ] ← top of MVE block (highest address, saved first)
 *   [Q7   — 4 words]
 *   [Q6   — 4 words]
 *   [Q5   — 4 words]
 *   [Q4   — 4 words]
 *   [Q3   — 4 words]
 *   [Q2   — 4 words]
 *   [Q1   — 4 words]
 *   [Q0   — 4 words] ← bottom of MVE block (lowest address, saved last)
 *   Total: 33 words = 132 bytes
 *
 * Cycle budget — no FPU, no MVE, no PSPLIM, no MPU (Cortex-M55, I-cache hit):
 * ─────────────────────────────────────────────────────────────────────────────
 *  Phase                                              Cycles (TRM)
 *  HW exception entry (auto-stack)                       12
 *  SW body (identical to M33 base case)               ~31cy
 *  HW exception return                                   10
 *                                                     ────
 *  Total (no optional gates)                          ~53cy
 *
 *  +MVE  (TAKTOS_MVE_CONTEXT=1):             +~72cy (8 VSTRW + STMDB VPR save;
 *                                                     8 VLDRW + LDM + VMSR restore)
 *  +FPU  (__VFP_FP__ && !__SOFTFP__, FPU active): +~18cy (VSTMDB/VLDMIA S16–S31)
 *  +PSPLIM (TAKTOS_STACK_LIMIT=1):           +~3cy  (LDR + MSR PSPLIM)
 *  +Reserved MPU-aware variant (not bound here): +~6cy  (LDR+ORR+LDR+STR+LDR+STR)
 *
 * TaktosCtx_t layout (taktos.cpp — static_assert enforced):
 *   offset 0: pCurrent     TaktOSThread_t*
 *   offset 4: pNextThread  TaktOSThread_t*
 *
 * TCB layout (TaktOSThread.h — static_assert enforced):
 *   offset  0: pSp          — saved stack pointer
 *   offset 16: pStackBottom — 32-byte-aligned stack floor (PSPLIM value / MPU guard base)
 *
 *
 *
 * @license
 *
 * MIT License
 *
 * Copyright (c) 2026 I-SYST inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 */

        .syntax unified
        .thumb
        .cpu    cortex-m55
#if (defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)
        .fpu    fpv5-sp-d16
#endif

        .extern g_TaktosCtx

#define EXC_RETURN_THREAD_PSP   0xFFFFFFFD
#define EXC_RETURN_FPU_BIT      0x10
#define CONTROL_SPSEL           0x2
#define CTX_CURRENT             0
#define CTX_NEXT_THREAD         4
/* TCB_STACK_BOTTOM must match offsetof(TaktOSThread_t, pStackBottom).      */
#define TCB_STACK_BOTTOM        16

/* MPU region reserved for the per-task stack guard.  Must match
 * MPU_GUARD_REGION in takt_mpu_v8m.h (C header cannot be included in .S). */
#define TAKT_MPU_GUARD_REGION   7

/* MPU_RNR address — literal pool load used by the MPU-aware variants.
 * RBAR is at RNR + 4, RLAR is at RNR + 8 on ARMv8-M. */
#define TAKT_MPU_RNR_ADDR       0xE000ED98


        /* TaktOSCtxSwitch removed from assembly.
         * Now always_inline in ARM/include/TaktCriticalSection.h:
         *   *((volatile unsigned int*)0xE000ED04u) = (1u << 28);
         * Saves ~5cy BL/BX overhead at every call site.            */



/* ═══════════════════════════════════════════════════════════════════════════
 * PendSV_Handler — steady-state context switch
 *
 * MVE frame layout when TAKTOS_MVE_CONTEXT=1 (below the GP frame):
 *   [VPR (1 word)] [Q0–Q7 (8 × 16 bytes = 128 bytes)] ← below STMDB {r4-r11}
 *   VPR is saved as a GP word via VMRS + STMDB so it sits at the top
 *   of the MVE block; Q0 is at the lowest address (last pre-dec store).
 *   Restore is VLDRW Q0..Q7 post-inc, then LDM VPR — mirror order.
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.PendSV_Handler, "ax", %progbits
        .global  PendSV_Handler
        .type    PendSV_Handler, %function
        .thumb_func
        .align   3

PendSV_Handler:

        /* ── 1. Save current thread ──────────────────────────────────── */
        MRS     r0, PSP

#if ((defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)) && TAKTOS_MVE_CONTEXT
        /* Save VPR first (as a GP word at top of MVE block), then Q0–Q7.
         * VSTRW pre-decrements 16 bytes per store; Q0 ends at lowest addr. */
        VMRS    r3, VPR
        STMDB   r0!, {r3}               /* VPR → frame (4 bytes)             */
        VSTRW.32 q7, [r0, #-16]!
        VSTRW.32 q6, [r0, #-16]!
        VSTRW.32 q5, [r0, #-16]!
        VSTRW.32 q4, [r0, #-16]!
        VSTRW.32 q3, [r0, #-16]!
        VSTRW.32 q2, [r0, #-16]!
        VSTRW.32 q1, [r0, #-16]!
        VSTRW.32 q0, [r0, #-16]!       /* Q0 at lowest address              */
#endif

        TST     lr, #EXC_RETURN_FPU_BIT
        IT      EQ
        VSTMDBEQ r0!, {s16-s31}

        STMDB   r0!, {r4-r11, lr}

                LDR     r1, =g_TaktosCtx

        /* Dual-load: hides 3 load-use stalls (see M4 for full analysis).
         * NOTE: when TAKTOS_MVE_CONTEXT=1 the VPR restore (LDM r0!, {r3})
         * clobbers r3. In that path we reload the next TCB ptr from
         * [r1, #CTX_CURRENT] (r1 = g_TaktosCtx, safe through all restores)
         * before the PSPLIM and MPU blocks.                                  */
        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready           */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready           */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming EXC_RETURN    */

        TST     lr, #EXC_RETURN_FPU_BIT        /* INCOMING thread FPU state   */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

#if ((defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)) && TAKTOS_MVE_CONTEXT
        /* Restore Q0–Q7 (lowest address first = Q0), then VPR.             */
        VLDRW.32 q0, [r0], #16
        VLDRW.32 q1, [r0], #16
        VLDRW.32 q2, [r0], #16
        VLDRW.32 q3, [r0], #16
        VLDRW.32 q4, [r0], #16
        VLDRW.32 q5, [r0], #16
        VLDRW.32 q6, [r0], #16
        VLDRW.32 q7, [r0], #16
        LDM     r0!, {r3}               /* VPR word                          */
        VMSR    VPR, r3
        /* r3 was clobbered by VPR restore above. r1 (g_TaktosCtx) survived
         * all restores (LDMIA writes r4-r11, VLDRW writes q0-q7 only).    */
        LDR     r3, [r1, #CTX_CURRENT]     /* reload next TCB ptr          */
#endif

        MSR     PSP, r0

#if TAKTOS_STACK_LIMIT
        /* r3 = next TCB (reloaded above if MVE, direct if no MVE)         */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]
        MSR     PSPLIM, r0
#endif



        BX      lr

        .size   PendSV_Handler, . - PendSV_Handler


/* ═══════════════════════════════════════════════════════════════════════════
 * SVC_Handler — first task launch only
 *
 * BUG FIX: use r2 = next TCB throughout so PSPLIM can read pStackBottom
 * after the GP frame is restored. The first task gets PSPLIM set here,
 * not deferred to the first PendSV — closing the unprotected window.
 *
 * MVE / FPU state: the first task's vector/FP registers are zero-initialized
 * by TaktOSThreadCreate via memset of the TCB block. No explicit restore
 * needed here — the hardware reset state is clean.
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.SVC_Handler, "ax", %progbits
        .global  SVC_Handler
        .type    SVC_Handler, %function
        .thumb_func
        .align   2

SVC_Handler:
        LDR     r1, =g_TaktosCtx
        LDR     r2, [r1, #CTX_NEXT_THREAD]  /* r2 = next TCB (preserved)    */
        STR     r2, [r1, #CTX_CURRENT]
        LDR     r0, [r2, #0]                 /* r0 = first task SP            */
        LDMIA   r0!, {r4-r11, lr}           /* consume SW frame incl. LR word */
        MSR     PSP, r0
#if TAKTOS_STACK_LIMIT
        /* r2 = next TCB — valid, never clobbered above                      */
        LDR     r0, [r2, #TCB_STACK_BOTTOM]
        MSR     PSPLIM, r0
#endif

        MOV     r0, #CONTROL_SPSEL
        MSR     CONTROL, r0
        ISB
        LDR     lr, =EXC_RETURN_THREAD_PSP
        BX      lr

        .size   SVC_Handler, . - SVC_Handler


/* ═══════════════════════════════════════════════════════════════════════════
 * PendSV_Handler_MPU — steady-state context switch, PSPLIM + MPU region 7
 *
 * Applies BOTH protections on every switch, unconditionally:
 *   PSPLIM : pStackBottom → PSPLIM  (no TAKTOS_STACK_LIMIT gate)
 *   MPU #7 : RNR=7, RBAR=base|0x05 (SH=00, AP=10 RO-priv, XN=1),
 *            RLAR=base|0x01 (AttrIndx=0 Normal-WB, EN=1)
 *
 * ARMv8-M MPU has no 'no-access' AP encoding; AP=10 (RO-priv) causes a
 * MemManage fault on any write from privileged code (tasks run privileged).
 * Combined with PSPLIM, this gives defense-in-depth stack overflow
 * detection:
 *   PSPLIM : fires when SP descends below pStackBottom.
 *   MPU #7 : fires on any write into [pStackBottom, pStackBottom+31]
 *            without moving SP (indexed stores, stale pointers).
 *
 * MVE / FPU paths are identical to PendSV_Handler — same save/restore
 * macros, same r3-reload-after-VPR-clobber dance when TAKTOS_MVE_CONTEXT=1.
 *
 * Register liveness through the protection writes:
 *   r3 = next TCB (preserved by LDMIA; reloaded from g_TaktosCtx after VPR
 *        clobber if MVE is active).
 *   r0 = pStackBottom (from [r3, #TCB_STACK_BOTTOM] for PSPLIM, reused
 *        by the MPU ORR without reloading).
 *   r1 = scratch (RBAR/RLAR values, region number 7).
 *   r2 = MPU_RNR address (literal pool).
 *
 * No DSB/ISB in-handler: BX lr flushes the pipeline via exception return.
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.PendSV_Handler_MPU, "ax", %progbits
        .global  PendSV_Handler_MPU
        .type    PendSV_Handler_MPU, %function
        .thumb_func
        .align   3

PendSV_Handler_MPU:

        /* ── 1. Save current thread ──────────────────────────────────── */
        MRS     r0, PSP

#if ((defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)) && TAKTOS_MVE_CONTEXT
        VMRS    r3, VPR
        STMDB   r0!, {r3}
        VSTRW.32 q7, [r0, #-16]!
        VSTRW.32 q6, [r0, #-16]!
        VSTRW.32 q5, [r0, #-16]!
        VSTRW.32 q4, [r0, #-16]!
        VSTRW.32 q3, [r0, #-16]!
        VSTRW.32 q2, [r0, #-16]!
        VSTRW.32 q1, [r0, #-16]!
        VSTRW.32 q0, [r0, #-16]!
#endif

        TST     lr, #EXC_RETURN_FPU_BIT
        IT      EQ
        VSTMDBEQ r0!, {s16-s31}

        STMDB   r0!, {r4-r11, lr}

        LDR     r1, =g_TaktosCtx

        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB              */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready             */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready             */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall  */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming EXC_RETURN      */

        TST     lr, #EXC_RETURN_FPU_BIT        /* INCOMING thread FPU state     */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

#if ((defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)) && TAKTOS_MVE_CONTEXT
        VLDRW.32 q0, [r0], #16
        VLDRW.32 q1, [r0], #16
        VLDRW.32 q2, [r0], #16
        VLDRW.32 q3, [r0], #16
        VLDRW.32 q4, [r0], #16
        VLDRW.32 q5, [r0], #16
        VLDRW.32 q6, [r0], #16
        VLDRW.32 q7, [r0], #16
        LDM     r0!, {r3}                       /* VPR word — clobbers r3       */
        VMSR    VPR, r3
        /* r1 (g_TaktosCtx) survived all restores — reload next TCB ptr.        */
        LDR     r3, [r1, #CTX_CURRENT]
#endif

        MSR     PSP, r0

        /* ── 2. PSPLIM + MPU region 7 — unconditional in the _MPU variant ─── */
        /* r3 = next TCB (reloaded above if MVE, direct if not).                */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]    /* 32-byte-aligned guard base    */
        MSR     PSPLIM, r0

        LDR     r2, =TAKT_MPU_RNR_ADDR         /* MPU_RNR                       */
        MOVS    r1, #TAKT_MPU_GUARD_REGION
        STR     r1, [r2]                       /* RNR = 7                       */

        ORR     r1, r0, #0x05                  /* RBAR: base | SH=00 | AP=10 | XN=1 */
        STR     r1, [r2, #4]                   /* RBAR @ RNR+4                  */
        ORR     r1, r0, #0x01                  /* RLAR: base | AttrIndx=0 | EN=1     */
        STR     r1, [r2, #8]                   /* RLAR @ RNR+8                  */

        BX      lr

        .size   PendSV_Handler_MPU, . - PendSV_Handler_MPU


/* ═══════════════════════════════════════════════════════════════════════════
 * SVC_Handler_MPU — first task launch with PSPLIM + MPU region 7
 *
 * Same structure as SVC_Handler; additionally writes PSPLIM (unconditional)
 * and MPU region 7 for the first task so both guards are active before the
 * task executes its first instruction.
 *
 * MVE / FPU state for the first task is zero-initialized by TaktOSThreadCreate
 * (memset of the TCB block); no explicit restore here.
 *
 * Register liveness:
 *   r2 = first TCB (preserved throughout).
 *   r0 = first-task SP after LDMIA, then reloaded with pStackBottom and
 *        reused for both PSPLIM and the MPU ORR.
 *   r3 = scratch: MPU_RNR address.
 *   r1 = scratch: RBAR/RLAR values and the region number 7.
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.SVC_Handler_MPU, "ax", %progbits
        .global  SVC_Handler_MPU
        .type    SVC_Handler_MPU, %function
        .thumb_func
        .align   2

SVC_Handler_MPU:
        LDR     r1, =g_TaktosCtx
        LDR     r2, [r1, #CTX_NEXT_THREAD]     /* r2 = first TCB (preserved)   */
        STR     r2, [r1, #CTX_CURRENT]
        LDR     r0, [r2, #0]                    /* r0 = first task SP           */
        LDMIA   r0!, {r4-r11, lr}              /* consume SW frame incl. LR    */
        MSR     PSP, r0

        /* PSPLIM for first task (unconditional in _MPU variant).               */
        LDR     r0, [r2, #TCB_STACK_BOTTOM]
        MSR     PSPLIM, r0

        /* MPU region 7 for first task.                                         */
        LDR     r3, =TAKT_MPU_RNR_ADDR         /* MPU_RNR                      */
        MOVS    r1, #TAKT_MPU_GUARD_REGION
        STR     r1, [r3]                        /* RNR = 7                      */
        ORR     r1, r0, #0x05
        STR     r1, [r3, #4]                    /* RBAR                         */
        ORR     r1, r0, #0x01
        STR     r1, [r3, #8]                    /* RLAR                         */

        MOV     r0, #CONTROL_SPSEL
        MSR     CONTROL, r0
        ISB
        LDR     lr, =EXC_RETURN_THREAD_PSP
        BX      lr

        .size   SVC_Handler_MPU, . - SVC_Handler_MPU


/* ═══════════════════════════════════════════════════════════════════════════
 * TaktOSStartFirst — triggers SVC to launch first task
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.TaktOSStartFirst, "ax", %progbits
        .global  TaktOSStartFirst
        .type    TaktOSStartFirst, %function
        .thumb_func
        .align   2

TaktOSStartFirst:
        SVC     #0
        B       .                       /* never reached                      */

        .size   TaktOSStartFirst, . - TaktOSStartFirst

        .ltorg
        .end
