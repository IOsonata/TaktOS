/* ─────────────────────────────────────────────────────────────────────────────
 * TaktOS — Cortex-M0 / M0+ context switch
 * ARM/cm0/PendSV_M0.S
 *
 * Contains three functions:
 *   PendSV_Handler  — steady-state context switch (called on every switch)
 *   SVC_Handler     — first task launch only (called once by TaktOSStartFirst)
 *   TaktOSStartFirst — issues SVC #0 to trigger first launch; never returns
 *
 * Design:
 *   Same precomputed-pNextThread approach as M4 — no scheduler scan at
 *   switch time. ARMv6-M constraints require a different save/restore sequence:
 *
 *   Save (3-instruction overhead vs M4 STMDB):
 *     SUBS r0, #32          allocate 8-word SW frame
 *     STM  r0!, {r4-r7}     save low regs r4-r7 at frame+0 (r0 advances to +16)
 *     MOV  r4,r8 ... r7,r11 copy high regs into r4-r7
 *     STM  r0!, {r4-r7}     save r8-r11 at frame+16 (r0 advances to +32)
 *     SUBS r0, #32          rewind r0 to frame base
 *
 *   Restore (2-LDM, not 3 — eliminates one LDM vs naive):
 *     ADDS r0, #16          point at r8-r11 area (frame+16)
 *     LDM  r0!, {r4-r7}     load saved r8-r11 into r4-r7 (r0 → frame+32)
 *     MOV  r8,r4 ... r11,r7 move to real high regs
 *     SUBS r0, #32          rewind to frame+0 (r4-r7 area)
 *     LDM  r0!, {r4-r7}     restore real r4-r7 (r0 → frame+16)
 *     ADDS r0, #16          r0 = frame+32 = new PSP
 *
 * ARMv6-M constraints:
 *   - STM/LDM operate on r0-r7 only; r8-r11 require MOV through low regs.
 *   - No Thumb-2 modified immediates; ICSR constant from literal pool.
 *   - ORR is register-only; MPU block uses MOVS + ORRS for RBAR value.
 *   - No IT blocks. No FPU. SUBS/ADDS used instead of SUB/ADD.
 *
 * Compile-time gates:
 *   PendSV_Handler_MPU uses MPU region 7 stack guard. Requires Cortex-M0+ with
 *                         MPU silicon (not present on plain Cortex-M0).
 *                         TaktOSMpuInit() is a no-op if MpuRegionCount() == 0.
 *                         M0+ minimum region size: 256 bytes (RASR SIZE=7).
 *                         RASR constant: 0x1000000F.
 *                         r3 (next TCB) is preserved through both LDM blocks.
 *
 * Cycle budget (ARMv6-M, zero wait states):
 * ─────────────────────────────────────────────────────────────────────────────
 *  Phase                                                 Cycles (TRM)
 *  HW exception entry (auto-stack)                          12
 *  MRS r0, PSP                                               1
 *  Save: SUBS + STM{r4-r7} + MOV×4 + STM{r4-r7} + SUBS    16  (1+5+4+5+1)
 *  LDR r1=g_TaktosCtx, dual-load r2/r3, STR/LDR/STR        10
 *  Restore (2-LDM): ADDS + LDM + MOV×4 + SUBS + LDM + ADDS 17  (1+5+4+1+5+1)
 *  MSR PSP + BX lr (HW unstack begins)                       2
 *  HW exception return (unstack)                             12
 *                                                         ────
 *  SW body                                               ~46cy
 *  Total                                                 ~70cy  (zero wait states)
 *
 *  Note: M0+ typically runs from flash with wait states. At 48 MHz with
 *  1 wait state, the flash fetch penalty dominates and real-world total
 *  is higher. Measure with DWT on target hardware.
 *
 *  +PendSV_Handler_MPU block:  +~8cy (MOVS+ORRS+LDR+STR+LDR+STR)
 *
 * TaktosCtx_t layout (taktos.cpp — static_assert enforced):
 *   offset 0: pCurrent     TaktOSThread_t*
 *   offset 4: pNextThread  TaktOSThread_t*
 *
 * TCB layout (TaktOSThread.h — static_assert enforced):
 *   offset  0: pSp          — saved stack pointer
 *   offset 16: pStackBottom — aligned stack floor (MPU guard base)
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
        .cpu    cortex-m0

        .extern g_TaktosCtx

#define EXC_RETURN_THREAD_PSP   0xFFFFFFFD
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
 * Save: allocate 32 bytes, STM r4–r7, move r8–r11 → r4–r7, STM again.
 *
 * Restore (2-LDM, not 3): skip to r8–r11 area first, LDM into r4–r7,
 * MOV to r8–r11, rewind to frame base, LDM real r4–r7, advance to top.
 * Eliminates one LDM (4 cycles) vs the naive three-LDM restore.
 * ═══════════════════════════════════════════════════════════════════════════ */

        .section .text.PendSV_Handler, "ax", %progbits
        .global  PendSV_Handler
        .type    PendSV_Handler, %function
        .thumb_func
        .align   2

PendSV_Handler:

        /* ── 1. Save current thread ──────────────────────────────────── */
        MRS     r0, PSP

        SUBS    r0, #32                 /* allocate SW frame (8 regs × 4)     */
        STM     r0!, {r4-r7}            /* save r4–r7 at frame+0, r0 → +16   */
        MOV     r4, r8
        MOV     r5, r9
        MOV     r6, r10
        MOV     r7, r11
        STM     r0!, {r4-r7}            /* save r8–r11 at frame+16, r0 → +32 */
        SUBS    r0, #32                 /* r0 = frame base                    */

        LDR     r1, =g_TaktosCtx

        /* Dual-load: ARMv6-M stalls 1cy when the next instruction uses a
         * just-loaded register. One instruction in between clears the hazard.
         * Eliminates 3 stalls vs the naive sequence.                        */
        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready           */

        /* ── 2. Switch to next thread ──────────────────────────── */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready           */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */

        /* ── 3. Restore next thread (2-LDM) ─────────────────────────── */
        /* Load r8–r11 first (at frame+16), using r4–r7 as scratch.
         * Then rewind and load the real r4–r7 (at frame+0).
         * Avoids a wasted first LDM that a naive r4-first approach requires. */
        ADDS    r0, #16                 /* point at frame+16 (r8–r11 values) */
        LDM     r0!, {r4-r7}            /* load saved r8–r11, r0 → frame+32  */
        MOV     r8,  r4
        MOV     r9,  r5
        MOV     r10, r6
        MOV     r11, r7
        SUBS    r0, #32                 /* r0 = frame+0 (r4–r7 values)       */
        LDM     r0!, {r4-r7}            /* restore real r4–r7, r0 → frame+16 */
        ADDS    r0, #16                 /* r0 = frame+32 = new PSP            */

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
        .align   2

PendSV_Handler_MPU:

        /* ── 1. Save current thread ──────────────────────────────────── */
        MRS     r0, PSP

        SUBS    r0, #32                 /* allocate SW frame (8 regs × 4)     */
        STM     r0!, {r4-r7}            /* save r4–r7 at frame+0, r0 → +16   */
        MOV     r4, r8
        MOV     r5, r9
        MOV     r6, r10
        MOV     r7, r11
        STM     r0!, {r4-r7}            /* save r8–r11 at frame+16, r0 → +32 */
        SUBS    r0, #32                 /* r0 = frame base                    */

        LDR     r1, =g_TaktosCtx

        /* Dual-load: ARMv6-M stalls 1cy when the next instruction uses a
         * just-loaded register. One instruction in between clears the hazard.
         * Eliminates 3 stalls vs the naive sequence.                        */
        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready           */

        /* ── 2. Switch to next thread ──────────────────────────── */
        LDR     r0, [r3, #0]                   /* new SP;  r3 ready           */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */

        /* ── 3. Restore next thread (2-LDM) ─────────────────────────── */
        /* Load r8–r11 first (at frame+16), using r4–r7 as scratch.
         * Then rewind and load the real r4–r7 (at frame+0).
         * Avoids a wasted first LDM that a naive r4-first approach requires. */
        ADDS    r0, #16                 /* point at frame+16 (r8–r11 values) */
        LDM     r0!, {r4-r7}            /* load saved r8–r11, r0 → frame+32  */
        MOV     r8,  r4
        MOV     r9,  r5
        MOV     r10, r6
        MOV     r11, r7
        SUBS    r0, #32                 /* r0 = frame+0 (r4–r7 values)       */
        LDM     r0!, {r4-r7}            /* restore real r4–r7, r0 → frame+16 */
        ADDS    r0, #16                 /* r0 = frame+32 = new PSP            */

        MSR     PSP, r0

        /* r2 = next TCB — preserved through both LDM blocks (r0-r1,r3-r7 used,
         * r2 never touched). r0-r3 will be restored by HW exception return.  */
        /* ARMv6-M: ORR is register-only; use MOVS+ORRS to build RBAR value. */
        /* r3 = next TCB — not clobbered by LDM restore (writes r4-r11 only) */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]    /* 256-byte-aligned guard base */
        MOVS    r1, #0x17                        /* VALID | REGION 7           */
        ORRS    r0, r1                            /* RBAR = base | 0x17         */
        LDR     r1, =0xE000ED9C                  /* MPU_RBAR                   */
        STR     r0, [r1]                          /* write RBAR                 */
        LDR     r0, =0x1000000F                  /* RASR: XN, no-access, 256B  */
        STR     r0, [r1, #4]                     /* write RASR                 */

        BX      lr

        .size   PendSV_Handler_MPU, . - PendSV_Handler_MPU


/* ═══════════════════════════════════════════════════════════════════════════
 * SVC_Handler — first task launch only
 *
 * Same 2-LDM restore pattern as PendSV_Handler.
 * Never called after TaktOSStart().
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
        LDR     r0, [r2, #0]                 /* r0 = first task SP             */

        /* 2-LDM restore — identical pattern to PendSV */
        ADDS    r0, #16
        LDM     r0!, {r4-r7}
        MOV     r8,  r4
        MOV     r9,  r5
        MOV     r10, r6
        MOV     r11, r7
        SUBS    r0, #32
        LDM     r0!, {r4-r7}
        ADDS    r0, #16                 /* r0 = frame+32 = initial PSP (no LR word on M0) */

        MSR     PSP, r0



        MOVS    r0, #CONTROL_SPSEL
        MSR     CONTROL, r0
        ISB
        LDR     r1, =EXC_RETURN_THREAD_PSP
        MOV     lr, r1
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
        LDR     r0, [r2, #0]                 /* r0 = first task SP             */

        /* 2-LDM restore — identical pattern to PendSV */
        ADDS    r0, #16
        LDM     r0!, {r4-r7}
        MOV     r8,  r4
        MOV     r9,  r5
        MOV     r10, r6
        MOV     r11, r7
        SUBS    r0, #32
        LDM     r0!, {r4-r7}
        ADDS    r0, #16                 /* r0 = frame+32 = initial PSP (no LR word on M0) */

        MSR     PSP, r0

        /* r2 = first TCB — not clobbered by LDM or MOV sequences above     */
        LDR     r3, [r2, #TCB_STACK_BOTTOM]
        MOVS    r1, #0x17
        ORRS    r3, r1
        LDR     r1, =0xE000ED9C
        STR     r3, [r1]
        LDR     r3, =0x1000000F
        STR     r3, [r1, #4]

        MOVS    r0, #CONTROL_SPSEL
        MSR     CONTROL, r0
        ISB
        LDR     r1, =EXC_RETURN_THREAD_PSP
        MOV     lr, r1
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
