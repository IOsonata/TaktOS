/* ─────────────────────────────────────────────────────────────────────────────
 * TaktOS — Cortex-M7 context switch
 * ARM/cm7/PendSV_M7.S
 *
 * Contains three functions:
 *   PendSV_Handler  — steady-state context switch (called on every switch)
 *   SVC_Handler     — first task launch only (called once by TaktOSStartFirst)
 *   TaktOSStartFirst — issues SVC #0 to trigger first launch; never returns
 *
 * Design:
 *   Same as M4: precomputed pNextThread, paired LDR loads hiding stalls,
 *   STR slides into load-use stall slot, TaktOSCtxSwitch() inlined in C.
 *
 *   .align 3 (8-byte boundary) instead of M4's .align 2. The M7 instruction
 *   fetch unit is 64-bit wide; aligning PendSV_Handler to an 8-byte boundary
 *   ensures the first fetch covers the first two 32-bit instructions without
 *   a wasted half-line.
 *
 *   ARMv7-M (same ISA as M4). No PSPLIM. FPv5-D16 (full double precision,
 *   D0–D15). The FPU context gate saves S16–S31 (= D8–D15 low halves).
 *   D16–D31 do not exist on FPv5-D16.
 *
 * Compile-time gates:
 *   Always compiled in; runtime-gated by TST lr, #EXC_RETURN_FPU_BIT:
 *                         save/restore S16–S31. Hardware lazy-stacks S0–S15.
 *                         Gate: TST lr, #EXC_RETURN_FPU_BIT; IT EQ; VSTMDB/VLDMIA.
 *   PendSV_Handler_MPU writes MPU region 7 stack guard on every switch.
 *                         r3 (next TCB) preserved through LDMIA. ~6cy overhead.
 *
 * Cycle budget — no FPU, no MPU (Cortex-M7, I-cache + D-cache hit):
 * ─────────────────────────────────────────────────────────────────────────────
 *  Phase                                              Cycles (TRM)
 *  HW exception entry (auto-stack)                       12
 *  MRS r0, PSP                                            1
 *  STMDB r0!, {r4–r11}         TRM: 1+8 regs             9
 *  LDR r1 + dual-load r2/r3 + STR/LDR/STR           ~7   (stalls hidden)
 *  LDMIA r0!, {r4–r11}         TRM: 1+8 regs             9
 *  MSR PSP + BX lr (HW unstack begins)                   2
 *  HW exception return                                   10
 *                                                     ────
 *  SW body                                           ~29cy  (M7 pipeline hides
 *  Total                                             ~51cy   some stalls via OOO window)
 *
 *  Note: M7 has a 6-stage pipeline with limited out-of-order capability and
 *  branch prediction. STM/LDM timing is 1+N cycles on the pipeline (same as
 *  M4) but the wider fetch bus and OOO window can reduce effective stall cost.
 *  DWT on-target validation pending (STM32H7 class target).
 *
 *  +FPU  (__VFP_FP__ && !__SOFTFP__, FPU active):  +~18cy
 *  +PendSV_Handler_MPU block:                 +~6cy
 *
 * TaktosCtx_t layout (taktos.cpp — static_assert enforced):
 *   offset 0: pCurrent     TaktOSThread_t*
 *   offset 4: pNextThread  TaktOSThread_t*
 *
 * TCB layout (TaktOSThread.h — static_assert enforced):
 *   offset  0: pSp          — saved stack pointer
 *   offset 16: pStackBottom — 32-byte-aligned stack floor (MPU guard base)
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
        .cpu    cortex-m7
#if (defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)
        .fpu    fpv5-d16
#endif

        .extern g_TaktosCtx

#define EXC_RETURN_THREAD_PSP   0xFFFFFFFD
#define EXC_RETURN_FPU_BIT      0x10
#define CONTROL_SPSEL           0x2
#define CTX_CURRENT             0
#define CTX_NEXT_THREAD         4
/* TCB_STACK_BOTTOM must match offsetof(TaktOSThread_t, pStackBottom) = 16. */
#define TCB_STACK_BOTTOM        16


        /* TaktOSCtxSwitch removed from assembly.
         * Now always_inline in ARM/include/TaktCriticalSection.h:
         *   *((volatile unsigned int*)0xE000ED04u) = (1u << 28);
         * Saves ~5cy BL/BX overhead at every call site.            */



/* ═══════════════════════════════════════════════════════════════════════════
 * PendSV_Handler — steady-state context switch
 *
 * .align 3: align to 8-byte boundary so the handler starts on a 64-bit
 * fetch-unit boundary on M7, avoiding a wasted half-line on first fetch.
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

        STMDB   r0!, {r4-r11, lr}

                LDR     r1, =g_TaktosCtx

        /* Dual-load: hides 3 load-use stalls (see M4 for full analysis).    */
        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB  (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready           */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready           */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming EXC_RETURN    */

        TST     lr, #EXC_RETURN_FPU_BIT        /* INCOMING thread FPU state   */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

        MSR     PSP, r0



        BX      lr

        .size   PendSV_Handler, . - PendSV_Handler


/* ═══════════════════════════════════════════════════════════════════════════
 * PendSV_Handler_MPU — steady-state context switch with MPU stack guard
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

        /* Dual-load: hides 3 load-use stalls (see M4 for full analysis).    */
        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB  (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready           */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready           */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming EXC_RETURN    */

        TST     lr, #EXC_RETURN_FPU_BIT        /* INCOMING thread FPU state   */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

        MSR     PSP, r0

        /* r3 = next TCB — preserved through LDMIA (restores r4-r11 only)    */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]   /* 32-byte-aligned guard base   */
        ORR     r0, r0, #0x17                  /* RBAR: VALID | REGION 7       */
        LDR     r3, =0xE000ED9C                /* MPU_RBAR                     */
        STR     r0, [r3]                        /* write RBAR (selects region)  */
        LDR     r0, =0x10000009                /* RASR constant                */
        STR     r0, [r3, #4]                   /* write RASR                   */

        BX      lr

        .size   PendSV_Handler_MPU, . - PendSV_Handler_MPU


/* ═══════════════════════════════════════════════════════════════════════════
 * SVC_Handler — first task launch only
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

        MOV     r0, #CONTROL_SPSEL
        MSR     CONTROL, r0
        ISB
        LDR     lr, =EXC_RETURN_THREAD_PSP
        BX      lr

        .size   SVC_Handler, . - SVC_Handler


/* ═══════════════════════════════════════════════════════════════════════════
 * SVC_Handler_MPU — first task launch only with MPU stack guard
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.SVC_Handler_MPU, "ax", %progbits
        .global  SVC_Handler_MPU
        .type    SVC_Handler_MPU, %function
        .thumb_func
        .align   2

SVC_Handler_MPU:
        LDR     r1, =g_TaktosCtx
        LDR     r2, [r1, #CTX_NEXT_THREAD]  /* r2 = first TCB (preserved)    */
        STR     r2, [r1, #CTX_CURRENT]
        LDR     r0, [r2, #0]                 /* r0 = first task SP            */
        LDMIA   r0!, {r4-r11, lr}           /* consume SW frame incl. LR word */
        MSR     PSP, r0
        LDR     r0, [r2, #TCB_STACK_BOTTOM]  /* write guard for first task    */
        ORR     r0, r0, #0x17
        LDR     r3, =0xE000ED9C
        STR     r0, [r3]
        LDR     r0, =0x10000009
        STR     r0, [r3, #4]
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
