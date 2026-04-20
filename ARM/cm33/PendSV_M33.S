/* ─────────────────────────────────────────────────────────────────────────────
 * TaktOS — Cortex-M33 context switch
 * ARM/cm33/PendSV_M33.S
 *
 * Contains five functions:
 *   PendSV_Handler      — steady-state context switch (PSPLIM only)
 *   PendSV_Handler_MPU  — steady-state context switch (PSPLIM + MPU region 7)
 *   SVC_Handler         — first task launch (PSPLIM only)
 *   SVC_Handler_MPU     — first task launch (PSPLIM + MPU region 7)
 *   TaktOSStartFirst    — issues SVC #0 to trigger first launch; never returns
 *
 * Design:
 *   Same core as M4: precomputed pNextThread, paired LDR loads hiding stalls,
 *   STR slides into load-use stall slot, TaktOSCtxSwitch() inlined in C.
 *
 *   ARMv8-M Mainline. Adds PSPLIM (hardware stack limit register) not present
 *   on ARMv7-M (M4/M7). TrustZone not used (NS world only, v1.0).
 *
 *   PendSV_Handler_MPU / SVC_Handler_MPU are bound by TaktKernelHandlerAssign
 *   when the application relocates its vector table to RAM and passes the
 *   RAM base to TaktOSInit.  When those variants are NOT bound, the default
 *   PendSV_Handler / SVC_Handler still writes PSPLIM on every switch, so
 *   M33 parts get hardware stack-overflow detection unconditionally.
 *
 * Compile-time gates:
 *   Always compiled in; runtime-gated by TST lr, #EXC_RETURN_FPU_BIT:
 *                         save/restore S16–S31 (FPv5-SP-D16).
 *                         Hardware lazy-stacks S0–S15 and FPSCR.
 *                         Gate: TST lr, #EXC_RETURN_FPU_BIT; IT EQ; VSTMDB/VLDMIA.
 *   PendSV_Handler_MPU writes MPU region 7 (32-byte RO-priv stack guard, XN=1)
 *                         via RBAR + RLAR on every switch.  PSPLIM is still
 *                         written, so the two mechanisms complement each
 *                         other: PSPLIM catches SP descent past the limit,
 *                         region 7 catches indexed writes into the guard
 *                         without moving SP.
 *                         Overhead: ~6cy (STR RNR + ORR RBAR + STR +
 *                                         ORR RLAR + STR) on top of the PSPLIM
 *                         path.
 *
 * PSPLIM — always active (no gate):
 *   Every Cortex-M33 part has PSPLIM. Writing it on every context switch gives
 *   hardware stack-overflow detection (UsageFault) at zero configuration cost.
 *   r3 = next TCB is preserved through LDMIA (restores r4-r11 only), so
 *   LDR r0, [r3, #TCB_STACK_BOTTOM] + MSR PSPLIM, r0 costs ~3cy and requires
 *   no extra register saves.  SVC_Handler also sets PSPLIM for the first task.
 *
 * Cycle budget — no FPU, no MPU (Cortex-M33, PSPLIM always on):
 * ─────────────────────────────────────────────────────────────────────────────
 *  Phase                                              Cycles (TRM)
 *  HW exception entry (auto-stack)                       12
 *  MRS r0, PSP                                            1
 *  STMDB r0!, {r4–r11}         TRM: 1+8 regs             9
 *  LDR r1 + dual-load r2/r3 + STR/LDR/STR            ~8  (stalls hidden)
 *  LDMIA r0!, {r4–r11}         TRM: 1+8 regs             9
 *  MSR PSP, r0                                            1
 *  LDR r0, [r3, #TCB_STACK_BOTTOM]                        2
 *  MSR PSPLIM, r0                                         2
 *  BX lr  (HW unstack begins)                             1
 *  HW exception return                                   10
 *                                                     ────
 *  SW body                                            ~34cy
 *  Total                                              ~56cy
 *
 *  +FPU  (__VFP_FP__ && !__SOFTFP__, FPU active):  +~18cy
 *  +PendSV_Handler_MPU block (MPU region 7 write): +~6cy  (STR RNR + ORR + STR RBAR + ORR + STR RLAR)
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
        .cpu    cortex-m33
#if (defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)
        .fpu    fpv5-sp-d16
#endif

        .extern g_TaktosCtx

/* EXC_RETURN for first-task launch via SVC_Handler.
 * ARMv8-M Secure single mode (no NS world):
 *   0xFFFFFFFD  S=1 FType=1 Thread/PSP — Standard frame, Secure state
 *   FType=1 is mandatory: TaktOSStackInit builds a standard 8-word frame.
 *   0xFFFFFFED (FType=0 Extended/FPU) would cause hardware to unstack 26
 *   words instead of 8, corrupting PSP for the first task.               */
#define EXC_RETURN_THREAD_PSP   0xFFFFFFFD      /* Secure Thread/PSP, Standard frame */
#define EXC_RETURN_FPU_BIT      0x10
#define CONTROL_SPSEL           0x2
#define CTX_CURRENT             0
#define CTX_NEXT_THREAD         4
/* TCB_STACK_BOTTOM must match offsetof(TaktOSThread_t, pStackBottom).
 * Update here if the TCB layout changes. */
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
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.PendSV_Handler, "ax", %progbits
        .global  PendSV_Handler
        .type    PendSV_Handler, %function
        .thumb_func
        .align   3

PendSV_Handler:

        /* ── 1. Save current thread ──────────────────────────────────── */
        MRS     r0, PSP

        TST     lr, #EXC_RETURN_FPU_BIT
        IT      EQ
        VSTMDBEQ r0!, {s16-s31}

        /* Save LR (EXC_RETURN) in the SW frame so the restore path reads the
         * INCOMING thread's FPU state rather than the outgoing thread's.
         * Without this, a FPU-active→no-FPU switch runs VLDMIA against a
         * context that has no S16-S31 region, corrupting PSP and crashing.  */
        STMDB   r0!, {r4-r11, lr}

                LDR     r1, =g_TaktosCtx

        /* Dual-load: hides 3 load-use stalls (see M4 for full analysis).    */
        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB  (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready           */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready           */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming thread's EXC_RETURN */

        TST     lr, #EXC_RETURN_FPU_BIT        /* test INCOMING thread's FPU state */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

        MSR     PSP, r0

        /* r3 = next TCB — safe: LDMIA writes r4-r11 only, r3 untouched     */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]   /* 32-byte-aligned guard base   */
        MSR     PSPLIM, r0



        BX      lr

        .size   PendSV_Handler, . - PendSV_Handler


/* ═══════════════════════════════════════════════════════════════════════════
 * SVC_Handler — first task launch only
 *
 * BUG FIX: use r2 = next TCB throughout so PSPLIM can read pStackBottom
 * from r2 after the stack is restored. The previous version used r0 for
 * the next_thread load, lost the TCB pointer, then read r2 undefined.
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.SVC_Handler, "ax", %progbits
        .global  SVC_Handler
        .type    SVC_Handler, %function
        .thumb_func
        .align   2

SVC_Handler:
        LDR     r1, =g_TaktosCtx
        LDR     r2, [r1, #CTX_NEXT_THREAD]  /* r2 = first TCB (preserved)    */
        STR     r2, [r1, #CTX_CURRENT]
        LDR     r0, [r2, #0]                 /* r0 = first task SP            */
        LDMIA   r0!, {r4-r11, lr}           /* consume SW frame incl. LR word */
        MSR     PSP, r0
        LDR     r0, [r2, #TCB_STACK_BOTTOM]
        MSR     PSPLIM, r0

        MOV     r0, #CONTROL_SPSEL
        MSR     CONTROL, r0
        ISB
        LDR     lr, =EXC_RETURN_THREAD_PSP
        BX      lr

        .size   SVC_Handler, . - SVC_Handler


/* ═══════════════════════════════════════════════════════════════════════════
 * PendSV_Handler_MPU — steady-state context switch with MPU region 7 guard
 *
 * Identical GP-register save/restore and PSPLIM path as PendSV_Handler.
 * Additionally writes MPU region 7 for the incoming task:
 *   RNR  = 7
 *   RBAR = pStackBottom | 0x05     (SH=00, AP=10 RO-priv, XN=1)
 *   RLAR = pStackBottom | 0x01     (AttrIndx=0 Normal-WB, EN=1)
 *
 * ARMv8-M MPU has no 'no-access' AP encoding.  AP=10 (RO-priv) causes a
 * MemManage fault on any write from privileged code (tasks run privileged)
 * and on any access from unprivileged code.  Combined with PSPLIM, this
 * gives defense-in-depth stack overflow detection:
 *   PSPLIM : fires when SP descends below pStackBottom.
 *   MPU #7 : fires on any write into the 32-byte guard [pStackBottom,
 *            pStackBottom+31] without moving SP (indexed stores, stale
 *            pointers landing in guard).
 *
 * No DSB/ISB required in-handler: the exception-return BX lr flushes the
 * pipeline before the incoming task resumes.  Same assumption as the
 * M4/M7 PendSV_Handler_MPU.
 *
 * Register liveness through the guard write:
 *   r3 = next TCB (preserved by LDMIA, which only writes r4-r11).
 *   r0 = pStackBottom (loaded from [r3, #TCB_STACK_BOTTOM] for PSPLIM).
 *        Reused below to build RBAR/RLAR via ORR — no extra load.
 *   r1 = scratch (RBAR/RLAR values, region number 7).
 *   r2 = MPU_RNR address (via literal pool).
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.PendSV_Handler_MPU, "ax", %progbits
        .global  PendSV_Handler_MPU
        .type    PendSV_Handler_MPU, %function
        .thumb_func
        .align   3

PendSV_Handler_MPU:

        /* ── 1. Save current thread ──────────────────────────────────── */
        MRS     r0, PSP

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

        MSR     PSP, r0

        /* ── 2. PSPLIM + MPU region 7 for incoming task ──────────────── */
        /* r3 = next TCB — preserved through LDMIA (writes r4-r11 only).       */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]    /* 32-byte-aligned guard base    */
        MSR     PSPLIM, r0

        /* Select region 7 via RNR, then write RBAR and RLAR.
         * pStackBottom is 32-byte aligned, so low 5 bits are zero — ORR is
         * safe for the encoding bits.                                          */
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
 * Same structure as SVC_Handler; additionally writes MPU region 7 for the
 * first task so the guard is active before the task executes its first
 * instruction.  Without this, the first task would run unprotected until
 * the first preemption triggered PendSV_Handler_MPU.
 *
 * Register liveness:
 *   r2 = next TCB (never clobbered).
 *   r0 = first-task SP after LDMIA, then reloaded with pStackBottom for
 *        PSPLIM and reused for the MPU writes.
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

        /* PSPLIM for first task.                                               */
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
