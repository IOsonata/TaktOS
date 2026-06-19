/* ---------------------------------------------------------------------------
 * TaktOS — Cortex-M4 context switch
 * ARM/cm4/PendSV_M4.S
 *
 * Contains three functions:
 *   PendSV_Handler  — steady-state context switch (called on every switch)
 *   PendSV_Handler_MPU — same, plus the per-task MPU region-7 stack guard
 *   TaktOSStartFirst — bootstraps the first task through PendSV; never returns
 *
 * First-task launch is SVC-free: it pends PendSV and reuses the identical
 * restore path as every context switch (see TaktOSStartFirst below). This
 * leaves the SVCall vector free for a SoftDevice-style stack that owns it.
 *
 * Design:
 *   pNextThread is precomputed by TaktReadyTask/TaktBlockTask on every
 *   scheduler change. PendSV reads it directly — no scheduler scan,
 *   no CLZ, no list walk at switch time.
 *
 *   TaktOSCtxSwitch() is a single ICSR write inlined in TaktOSCriticalSection.h.
 *   It is not in this file.
 *
 *   .align 2 (Thumb-2 minimum). .balign 64 was tested but shifts surrounding
 *   hot functions (TaktReadyTask, TaktBlockTask, semaphore code) to worse
 *   128-byte prefetch positions in TM3/TM7 binaries — net loss.
 *
 * Compile-time gates (Eclipse Defined Symbols):
 *   Always compiled in; runtime-gated by TST lr, #EXC_RETURN_FPU_BIT:
 *                         save/restore S16–S31 (callee-save VFP regs).
 *                         S0–S15 and FPSCR are handled by hardware lazy stacking.
 *                         Gate: TST lr, #EXC_RETURN_FPU_BIT; IT EQ; VSTMDB/VLDMIA.
 *                         Overhead: ~18cy per switch when FPU active.
 *   PendSV_Handler_MPU writes MPU region 7 (32-byte no-access stack guard)
 *                         for the incoming task on every switch. r3 (next TCB)
 *                         is preserved through LDMIA and available for LDR.
 *                         Overhead: ~6cy (LDR + ORR + LDR + STR + LDR + STR).
 *
 * Cycle budget — no FPU, no MPU (Cortex-M4, I-cache hit, zero wait states):
 * ---------------------------------------------------------------------------
 *  Phase                                              Cycles (TRM)
 *  HW exception entry (auto-stack r0–r3,r12,lr,pc,xpsr)  12
 *  MRS r0, PSP                                             1
 *  TST lr, #EXC_RETURN_FPU_BIT  +  IT EQ  (FPU gate)       2
 *  STMDB r0!, {r4–r11, lr}      TRM: 1+9 regs            10
 *  LDR r1, =g_TaktosCtx         literal pool load          2
 *  LDR r2, [r1, #0]             current TCB (load-use)     2
 *  LDR r3, [r1, #4]             next TCB (hides r2 stall)  1
 *  STR r0, [r2, #0]             save SP (r2 ready)         1
 *  LDR r0, [r3, #0]             new SP  (load-use)         2
 *  STR r3, [r1, #0]             current=next (hides r0 stall) 1
 *  LDMIA r0!, {r4–r11, lr}      TRM: 1+9 regs            10
 *  TST lr, #EXC_RETURN_FPU_BIT  +  IT EQ  (FPU gate)       2
 *  MSR PSP, r0                                             1
 *  BX lr  (HW unstack begins)                              1
 *  HW exception return (unstack r0–r3,r12,lr,pc,xpsr)    10
 *                                                       ────
 *  SW body                                             ~35cy
 *  Total                                               ~57cy
 *
 *  Load-use stall elimination: LDR r2 and LDR r3 are issued back-to-back;
 *  each hides the other's 1-cycle latency. STR r3 slides into the stall
 *  slot before LDMIA. Net saving: 3 stalls eliminated vs naive sequence.
 *
 *  LR is saved/restored with r4-r11 so the incoming task's EXC_RETURN
 *  drives unstack — a task switched in with FPU context returns with the
 *  FPU bit set in lr, and a task without FPU context returns with it
 *  clear. Without this, the incoming task would inherit the outgoing
 *  task's lr and corrupt the exception return frame.
 *
 *  +FPU  (__VFP_FP__ && !__SOFTFP__, FPU active):  +~18cy (VSTMDB/VLDMIA S16-S31)
 *  +PendSV_Handler_MPU block:                 +~6cy  (LDR+ORR+LDR+STR+LDR+STR)
 *
 * TaktosCtx_t layout (taktos.cpp — static_assert enforced):
 *   offset 0: pCurrent     TaktOSThread_t*  — currently running thread
 *   offset 4: pNextThread  TaktOSThread_t*  — precomputed next thread
 *
 * TCB layout (TaktOSThread.h — static_assert enforced):
 *   offset  0: pSp          — saved stack pointer (loaded directly by assembly)
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
 * ---------------------------------------------------------------------------
 */

        .syntax unified
        .thumb
        .cpu    cortex-m4
#if (defined(__VFP_FP__) && !defined(__SOFTFP__)) || defined(__ARMVFP__)
        .fpu    fpv4-sp-d16
#endif

        .extern g_TaktosCtx
#if defined(TAKTOS_WAKE_TRACE_ENABLE)
        .extern g_TaktWakeTrace
#endif

#define EXC_RETURN_THREAD_PSP   0xFFFFFFFD
#define EXC_RETURN_FPU_BIT      0x10
#define CONTROL_SPSEL           0x2
#define CTX_CURRENT             0
#define CTX_NEXT_THREAD         4
/* TCB_STACK_BOTTOM must match offsetof(TaktOSThread_t, pStackBottom) = 16.
 * Update here if the TCB layout changes. */
#define TCB_STACK_BOTTOM        16

#define TRACE_ENABLED           0
#define TRACE_PENDSV_ENTRY      40
#define TRACE_PENDSV_RETURN     44
#define DWT_CYCCNT              0xE0001004


        /* TaktOSCtxSwitch removed from assembly.
         * Now always_inline in ARM/include/TaktOSCriticalSection.h:
         *   *((volatile unsigned int*)0xE000ED04u) = (1u << 28);
         * Saves ~5cy BL/BX overhead at every call site.            */



/* ===========================================================================
 * PendSV_Handler -- steady-state context switch, no first-launch check
 * =========================================================================== */

        .section .text.PendSV_Handler, "ax", %progbits
        .global  PendSV_Handler
        .type    PendSV_Handler, %function
        .thumb_func
        .balign  16                     /* ICACHE line on nRF52 NVMC = 16 B.
                                         * Aligning the handler entry to a
                                         * cache-line boundary guarantees its
                                         * first line is not shared with any
                                         * other function — the handler hits
                                         * a fresh cache line on every entry
                                         * regardless of which task code ran
                                         * just before SysTick fired.  The
                                         * older .balign 64 is still wrong
                                         * (TM3/TM7 surrounding-function
                                         * shift); 16 is the minimum that
                                         * still hits this property and is
                                         * compatible with the 16 B ICACHE
                                         * line on nRF52 / 32 B on Cortex-M7
                                         * (M7 cache lines are 32 B but
                                         * accept 16 B alignment fine — line
                                         * fill always starts at the line
                                         * boundary regardless).         */

PendSV_Handler:

#if defined(TAKTOS_WAKE_TRACE_ENABLE)
        LDR     r0, = g_TaktWakeTrace
        LDR     r1, [r0, #TRACE_ENABLED]
        CBZ     r1, 1f
        LDR     r1, = DWT_CYCCNT
        LDR     r1, [r1]
        STR     r1, [r0, #TRACE_PENDSV_ENTRY]
1:
#endif

        /* -- 1. Save current thread ---------------------------------------- */
        MRS     r0, PSP

        TST     lr, #EXC_RETURN_FPU_BIT
        IT      EQ
        VSTMDBEQ r0!, {s16-s31}

        STMDB   r0!, {r4-r11, lr}              /* save LR so restore uses incoming EXC_RETURN */

        LDR     r1, = g_TaktosCtx

        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready (2 away)  */

        /* -- 2. Switch to next thread --------------------------------------- */
        LDR     r0, [r3, #0]                   /* new SP; r3 ready (2 away)   */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */

        /* -- 3. Restore next thread ---------------------------------------- */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming thread's EXC_RETURN */

        TST     lr, #EXC_RETURN_FPU_BIT        /* test INCOMING thread's FPU state */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

        MSR     PSP, r0

#if defined(TAKTOS_WAKE_TRACE_ENABLE)
        LDR     r0, = g_TaktWakeTrace
        LDR     r1, [r0, #TRACE_ENABLED]
        CBZ     r1, 2f
        LDR     r1, = DWT_CYCCNT
        LDR     r1, [r1]
        STR     r1, [r0, #TRACE_PENDSV_RETURN]
2:
#endif

        BX      lr

        .size   PendSV_Handler, . - PendSV_Handler


/* ===========================================================================
 * PendSV_Handler_MPU -- steady-state context switch with MPU stack guard
 * =========================================================================== */

        .section .text.PendSV_Handler_MPU, "ax", %progbits
        .global  PendSV_Handler_MPU
        .type    PendSV_Handler_MPU, %function
        .thumb_func
        .balign  16                     /* ICACHE-line aligned (see notes on
                                         * PendSV_Handler above).             */

PendSV_Handler_MPU:

#if defined(TAKTOS_WAKE_TRACE_ENABLE)
        LDR     r0, = g_TaktWakeTrace
        LDR     r1, [r0, #TRACE_ENABLED]
        CBZ     r1, 1f
        LDR     r1, = DWT_CYCCNT
        LDR     r1, [r1]
        STR     r1, [r0, #TRACE_PENDSV_ENTRY]
1:
#endif

        /* -- 1. Save current thread ---------------------------------------- */
        MRS     r0, PSP

        TST     lr, #EXC_RETURN_FPU_BIT
        IT      EQ
        VSTMDBEQ r0!, {s16-s31}

        STMDB   r0!, {r4-r11, lr}              /* save LR so restore uses incoming EXC_RETURN */

        LDR     r1, = g_TaktosCtx

        LDR     r2, [r1, #CTX_CURRENT]         /* r2 = current TCB            */
        LDR     r3, [r1, #CTX_NEXT_THREAD]     /* r3 = next TCB (hides r2 stall) */

        STR     r0, [r2, #0]                   /* save SP; r2 ready (2 away)  */

        /* -- 2. Switch to next thread --------------------------------------- */
        LDR     r0, [r3, #0]                   /* new SP; r3 ready (2 away)   */
        STR     r3, [r1, #CTX_CURRENT]         /* current=next; hides r0 stall */

        /* -- 3. Restore next thread ---------------------------------------- */
        LDMIA   r0!, {r4-r11, lr}              /* lr = incoming thread's EXC_RETURN */

        TST     lr, #EXC_RETURN_FPU_BIT        /* test INCOMING thread's FPU state */
        IT      EQ
        VLDMIAEQ r0!, {s16-s31}

        MSR     PSP, r0

        /* r3 = next TCB -- preserved through LDMIA (restores r4-r11 only)    */
        /* Write region 7 stack guard for the incoming task.                  */
        /* RBAR: pStackBottom | 0x17 (VALID=1, REGION=7)                      */
        /* RASR: XN=1, AP=000 no-access, SIZE=4 (32 bytes), ENABLE=1          */
        LDR     r0, [r3, #TCB_STACK_BOTTOM]    /* 32-byte-aligned guard base  */
        ORR     r0, r0, #0x17                   /* RBAR: VALID | REGION 7      */
        LDR     r2, =0xE000ED9C                 /* MPU_RBAR                    */
        STR     r0, [r2]                         /* write RBAR (selects region) */
        LDR     r0, =0x10000009                 /* RASR constant               */
        STR     r0, [r2, #4]                    /* write RASR                  */

#if defined(TAKTOS_WAKE_TRACE_ENABLE)
        LDR     r0, = g_TaktWakeTrace
        LDR     r1, [r0, #TRACE_ENABLED]
        CBZ     r1, 2f
        LDR     r1, = DWT_CYCCNT
        LDR     r1, [r1]
        STR     r1, [r0, #TRACE_PENDSV_RETURN]
2:
#endif

        BX      lr

        .size   PendSV_Handler_MPU, . - PendSV_Handler_MPU


/* ===========================================================================
 * TaktOSStartFirst -- SVC-free first-task launch via a PendSV bootstrap
 *
 * Replaces the old "SVC #0" launch so the SVCall vector stays free for a
 * SoftDevice-style stack that statically owns SVC_Handler (e.g. nRF54L
 * sdk-nrf-bm).  The first task is launched through PendSV, reusing the exact
 * restore path of every context switch -- no duplicated frame logic and no
 * first-launch branch in the steady-state PendSV hot path.
 *
 *   1. pCurrent = g_TaktBootDiscard so the bootstrap PendSV's save half writes
 *      the throwaway boot context into discardable scratch (never read again).
 *   2. PSP = top of g_TaktBootScratch, then switch CONTROL to PSP.  Writing
 *      CONTROL=#SPSEL also clears FPCA, so the bootstrap save uses a standard
 *      (no-FP) frame -- the scratch only needs to hold one standard frame.
 *      Entering PendSV from Thread/PSP makes hardware set EXC_RETURN to
 *      0xFFFFFFFD, so the restore returns to Thread/PSP.
 *   3. Pend PendSV; it tail-chains immediately (lowest priority, nothing else
 *      pending) and restores the first real task.  Never returns here.
 * =========================================================================== */

        .section .text.TaktOSStartFirst, "ax", %progbits
        .global  TaktOSStartFirst
        .type    TaktOSStartFirst, %function
        .thumb_func
        .align   2

TaktOSStartFirst:
        LDR     r1, =g_TaktosCtx
        LDR     r2, =g_TaktBootDiscard
        STR     r2, [r1, #CTX_CURRENT]           /* pCurrent = discard target  */

        LDR     r0, =g_TaktBootScratchTop
        LDR     r0, [r0]                          /* r0 = top of boot scratch   */
        MSR     PSP, r0

        MOV     r0, #CONTROL_SPSEL                /* SPSEL=1 (PSP); clears FPCA  */
        MSR     CONTROL, r0
        ISB

        LDR     r0, =0xE000ED04                   /* SCB ICSR                    */
        LDR     r1, =0x10000000                   /* PENDSVSET                   */
        STR     r1, [r0]
        DSB
        ISB
        CPSIE   i                                 /* allow PendSV to fire        */
1:      B       1b                                /* never reached               */

        .size   TaktOSStartFirst, . - TaktOSStartFirst

        .ltorg
        .end
