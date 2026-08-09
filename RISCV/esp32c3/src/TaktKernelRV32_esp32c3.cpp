/**---------------------------------------------------------------------------
@file   TaktKernelRV32_esp32c3.cpp

@brief  TaktOS ESP32-C3 / ESP32-C6 RISC-V port implementation

Implements the port functions declared in TaktKernel.h:
  TaktKernelStackInit() — build initial task stack frame for context switch
  TaktOSTickInit()   — configure SYSTIMER + interrupt matrix
  TaktKernelHandlerAssign() — bind mtvec to the application-supplied trap base

This file is the RISC-V equivalent of ARM/src/TaktKernelCM.cpp.

── Stack frame built by TaktKernelStackInit() ──────────────────────────────

TaktKernelStackInit() places a full 128-byte frame on the task stack so that
the first entry into _takt_trap_rv32 → _takt_ctx_handler → restore path
treats the first task identically to any subsequent context switch.  No
special-case code in the trap handler.

Frame layout (top of stack = highest address):
  [stack top]              ← pStackTop argument
  ...user stack grows down...
  FRAME_S11 (x27) = 0     ← word 31 (lowest address in frame)
  FRAME_S10 (x26) = 0
  ...                        (all callee-save regs = 0)
  FRAME_S0  (x8)  = 0
  FRAME_T6  (x31) = 0
  ...                        (all caller-save regs = 0 except a0, ra)
  FRAME_A0  (x10) = arg   ← task argument
  FRAME_T2  (x7)  = 0
  FRAME_T1  (x6)  = 0
  FRAME_T0  (x5)  = 0
  FRAME_TP  (x4)  = 0
  FRAME_GP  (x3)  = 0
  FRAME_RA  (x1)  = TaktOSThreadExit  ← return address if task function returns
  FRAME_MSTATUS   = 0x1880             ← MPIE=1, MPP=11 (M-mode)
  FRAME_MEPC      = task entry         ← PC when mret fires
  ← pSp stored in TCB (TaktOSThread_t.pSp = sp here)

mstatus value 0x1880:
  bits [12:11] MPP  = 11  (machine mode — all code runs in M-mode on bare metal)
  bit  [ 7]    MPIE = 1   (MIE will be set to 1 by mret → task starts with IRQs on)

── TaktOSTickInit() ─────────────────────────────────────────────────────────

1. TaktKernelHandlerAssign() binds mtvec to the application-supplied trap
   base, or to _takt_trap_rv32 when HandlerBaseAddr == 0.
2. Configure interrupt matrix (intmtx_esp32c3.h):
     SYSTIMER_TARGET0 → CPU INT 2  (level, priority 2)
     CPU_INTR_FROM_CPU_0 → CPU INT 29 (edge, priority 1)
3. Configure SYSTIMER TARGET0 in period mode at TickHz.
4. Enable machine external interrupt in mie (bit 11).

mstatus.MIE is NOT set here; it is set automatically by mret in
TaktOSStartFirst when the first task's frame (MPIE=1) is loaded.

── ESP32-C6 note ────────────────────────────────────────────────────────────
Identical source.  Compile with -march=rv32imafc_zbb to enable hardware
CLZ (Zbb extension); __builtin_clz() in TaktReadyTask/TaktBlockTask will
then emit the single-cycle 'clz' instruction.

@author  Hoang Nguyen Hoan
@date    Apr. 2026

@license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
----------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

#include "TaktKernel.h"
#include "TaktOSThread.h"

#include "systimer_esp32c3.h"
#include "intmtx_esp32c3.h"

// ── ABI with ctx_switch_rv32.S ────────────────────────────────────────────────
// Frame layout must match the frame offset constants in ctx_switch_rv32.S.
// These offsets are duplicated here to allow the C-side static_assert below.
// Update both files if the layout changes.

#define RV32_FRAME_MEPC      0u
#define RV32_FRAME_MSTATUS   4u
#define RV32_FRAME_RA        8u
#define RV32_FRAME_GP       12u
#define RV32_FRAME_TP       16u
#define RV32_FRAME_T0       20u
#define RV32_FRAME_T1       24u
#define RV32_FRAME_T2       28u
#define RV32_FRAME_A0       32u
#define RV32_FRAME_A1       36u
#define RV32_FRAME_A2       40u
#define RV32_FRAME_A3       44u
#define RV32_FRAME_A4       48u
#define RV32_FRAME_A5       52u
#define RV32_FRAME_A6       56u
#define RV32_FRAME_A7       60u
#define RV32_FRAME_T3       64u
#define RV32_FRAME_T4       68u
#define RV32_FRAME_T5       72u
#define RV32_FRAME_T6       76u
#define RV32_FRAME_S0       80u
#define RV32_FRAME_S1       84u
#define RV32_FRAME_S2       88u
#define RV32_FRAME_S3       92u
#define RV32_FRAME_S4       96u
#define RV32_FRAME_S5      100u
#define RV32_FRAME_S6      104u
#define RV32_FRAME_S7      108u
#define RV32_FRAME_S8      112u
#define RV32_FRAME_S9      116u
#define RV32_FRAME_S10     120u
#define RV32_FRAME_S11     124u
#define RV32_FRAME_SIZE    128u

// mstatus initial value:
//   MPP  [12:11] = 11 (machine mode)
//   MPIE [7]     = 1  (interrupts enabled on mret)
#define RV32_MSTATUS_TASK_INIT  ((3u << 11) | (1u << 7))  // 0x1880

// ── Context block ABI assertions ─────────────────────────────────────────────
// These must match the CTX_* constants in ctx_switch_rv32.S.
static_assert(offsetof(TaktKernelCtx_t, pCurrent)    == 0u, "CTX_CURRENT broken");
static_assert(offsetof(TaktKernelCtx_t, pNextThread) == 4u, "CTX_NEXT_THREAD broken");

// TCB.pSp must be at offset 0 (loaded by 'lw sp, TCB_SP(t1)' in assembly).
// TCB.pStackBottom must be at offset 16 (TCB_PMP_BASE in assembly).
// These are enforced by the static_assert in taktos_thread.cpp; repeated
// here for the RISC-V port's self-documentation.
static_assert(offsetof(TaktOSThread_t, pSp)         == 0u,  "TCB_SP broken");
static_assert(offsetof(TaktOSThread_t, pStackBottom) == 16u, "TCB_PMP_BASE broken");

/**
 * @brief	ESP32-C3 machine-mode trap entry point (defined in ctx_switch_rv32.S).
 *
 * Bound through TaktKernelHandlerAssign() before the scheduler starts.
 * Routes machine external interrupts (mcause 11) to the tick or context-switch
 * handler based on which CPU interrupt line is pending.
 */
extern "C" void _takt_trap_rv32(void);

extern "C" bool TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr)
{
    uint32_t tvec = (uint32_t)(HandlerBaseAddr != 0u ? HandlerBaseAddr :
                               (uintptr_t)_takt_trap_rv32) & ~3u;
    __asm__ volatile ("csrw mtvec, %0" :: "r"(tvec) : "memory");

    return false;
}

// ── g_TaktosCtx is owned by taktos.cpp ──────────────────────────────────────
extern TaktKernelCtx_t g_TaktosCtx;
extern "C" void TaktOSThreadExit(void);


/**
 * @brief  Build the initial RISC-V exception frame for a new task.
 *
 * Lays out a 128-byte frame on the stack matching the save sequence in
 * ctx_switch_rv32.S, so the first context switch into this task is
 * indistinguishable from any subsequent one.
 *
 * @param  pStackTop    Pointer to the top of the allocated stack region.
 *                      Must be 4-byte aligned (16-byte preferred per RISC-V ABI).
 * @param  pThreadFct   Task entry function.
 * @param  pArg         Argument passed to pThreadFct in a0 (x10).
 * @return  New stack pointer (bottom of the initial frame, stored in TCB.pSp).
 */
void *TaktKernelStackInit(void *pStackTop, void (*pThreadFct)(void*), void *pArg)
{
    // Align down to 16 bytes (RISC-V ABI stack alignment requirement).
    uintptr_t sp = ((uintptr_t)pStackTop) & ~(uintptr_t)15u;

    // Allocate the context frame on the stack.
    sp -= RV32_FRAME_SIZE;
    uint32_t *frame = (uint32_t*)sp;

    // Zero the entire frame first.
    for (uint32_t i = 0u; i < (RV32_FRAME_SIZE / 4u); i++)
    {
        frame[i] = 0u;
    }

    // MEPC = task entry point.  mret jumps here to start the task.
    frame[RV32_FRAME_MEPC  / 4u] = (uint32_t)pThreadFct;

    // MSTATUS: MPIE=1 (interrupts enabled after mret), MPP=11 (M-mode).
    frame[RV32_FRAME_MSTATUS / 4u] = RV32_MSTATUS_TASK_INIT;

    // RA = TaktOSThreadExit — if the task function returns, TaktOSThreadExit
    // is called to cleanly remove the thread from the scheduler.
    frame[RV32_FRAME_RA / 4u] = (uint32_t)TaktOSThreadExit;

    // A0 (x10) = task argument.
    frame[RV32_FRAME_A0 / 4u] = (uint32_t)pArg;

    // All other registers (gp, tp, t0-t6, a1-a7, s0-s11) remain zero.
    // gp (x3) and tp (x4) are zero; bare-metal ESP32-C3 initialises these
    // at startup — they will be set correctly on first trap entry because
    // the initial frame is built from the boot-time register state.

    return (void*)sp;
}


/**
 * @brief  Initialise the TaktOS tick source and interrupt infrastructure.
 *
 * Called once from TaktOSStart() before TaktOSStartFirst().
 *
 * Steps:
 *  1. Configure interrupt matrix (see IntMtxTaktOSInit).
 *  2. Configure SYSTIMER TARGET0 in period mode at TickHz.
 *  3. Enable machine external interrupt in mie.
 *
 * @param  CoreClkHz  Core clock frequency (Hz).  Not used directly —
 *                    SYSTIMER runs at a fixed 80 MHz on ESP32-C3/C6.
 * @param  TickHz     Desired tick rate in Hz (typically 1000).
 * @param  TickClockSrc Selected tick clock source. Ignored on ESP32-C3/C6
 *                      because SYSTIMER is not SysTick-based.
 * @param  TickPriority Tick priority on the TaktOS standard scale
 *                      (TAKTOS_PRIORITY_LOWEST(1)..TAKTOS_PRIORITY_CRITICAL(31),
 *                      higher = more urgent).  TaktArchTickPrioMap() converts
 *                      it to the INTMTX 1..7 encoding;
 *                      TAKTOS_TICK_PRIORITY_DEFAULT (0) selects
 *                      TAKT_TICK_CPU_INT_PRIORITY.
 */
void TaktOSTickInit(uint32_t CoreClkHz, uint32_t TickHz, TaktOSTickClockSrc_t TickClockSrc,
                    uint32_t TickPriority)
{
    (void)CoreClkHz;      // SYSTIMER clock is independent of CPU frequency
    (void)TickClockSrc;   // Not applicable on this target

    // ── Step 1: configure interrupt matrix ─────────────────────────────────
    // Routes SYSTIMER_TARGET0 → CPU INT 2 (level, mapped TickPriority)
    //        CPU_INTR_FROM_CPU_0 → CPU INT 29 (edge, priority 1)
    // Enables both CPU INT lines; sets threshold = 0.
    IntMtxTaktOSInit(TaktArchTickPrioMap(TickPriority));

    // ── Step 2: configure SYSTIMER TARGET0 in period mode ──────────────────
    // SYSTIMER clock = 80 MHz (fixed on both ESP32-C3 and ESP32-C6).
    // Period = 80_000_000 / TickHz ticks per tick interrupt.
    SysTimerSetFrequency(TickHz);

    // ── Step 3: enable machine external interrupt in mie (bit 11 = MEIE) ───
    // mstatus.MIE is NOT set here; it is restored from mstatus.MPIE by mret
    // inside TaktOSStartFirst when the first task's initial frame is loaded.
    uint32_t mie;
    __asm__ volatile ("csrr %0, mie" : "=r"(mie));
    mie |= (1u << 11);  // MEIE: machine external interrupt enable
    __asm__ volatile ("csrw mie, %0" :: "r"(mie) : "memory");
}
