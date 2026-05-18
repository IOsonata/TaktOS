/**---------------------------------------------------------------------------
@file   TaktKernelRiscv.cpp

@brief  TaktOS RV32 arch kernel — port implementation

Counterpart to ARM/src/TaktKernelCM.cpp.  Provides the four functions
that the kernel calls into the arch lib:

  TaktKernelHandlerAssign  Install mtvec (direct mode) pointing at the arch
                           trap entry, or the application-supplied trap base
                           if non-zero.

  TaktKernelStackInit      Build the initial 128-byte trap frame for a new
                           task — mret jumps to the task entry with MIE on.

  TaktOSTickInit           Weak.  CLINT-based default: program
                           mtimecmp = mtime + (KernClockHz / TickHz),
                           enable mie.MSIE | mie.MTIE.  Override for chips
                           outside the standard CLINT profile.

  TaktHalTrapDispatch      Weak.  CLINT-based default: decode mcause, re-arm
                           mtimecmp for the timer trap and clear MSIP for
                           the software-int trap.  Override for non-CLINT.

────────────────────────────────────────────────────────────────────────────
CLINT layout (RISC-V Privileged Spec, hart 0):

    MSIP                base + 0x0000   (software-interrupt trigger)
    mtimecmp lo / hi    base + 0x4000 / +0x4004
    mtime lo / hi       base + 0xBFF8 / +0xBFFC

g_TaktSoftIntReg (set by TaktOSInit from TaktOSCfg_t.SoftIntAddr) points
at MSIP.  mtime and mtimecmp are derived from that single address — no
second config field is needed for CLINT chips.

Standard MSIP base values:
    FE310, Nuclei N-series, BL616, R9A02G021, ESP32-C6/H2    0x02000000
    GD32VF103 (CLINT at non-standard base)                   0xD1000000

Non-CLINT chips (ESP32-C3 — INTMTX + SYSTIMER) override TaktOSTickInit
and TaktHalTrapDispatch with strong defs.  TaktOS is HAL-agnostic — the
override may live in app code, a chip-specific source the user writes,
or anywhere else convenient.

@author  Nguyen Hoan Hoang
@date    May 2026
@license MIT  Copyright (c) 2026 I-SYST inc.  All rights reserved.
----------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

#include "TaktKernel.h"
#include "TaktOSThread.h"
#include "port_rv32.h"


/* ── ctx_switch.S exports ─────────────────────────────────────────────── */
extern "C" void TaktArchTrapEntry(void);

/* ── taktos.cpp exports ───────────────────────────────────────────────── */
extern "C" void TaktOSThreadExit(void);
extern "C" void TaktKernelTickHandler(void);

/* ── This file exports (declarations here force C linkage on the weak
 *     definitions below — TaktHalTrapDispatch is not declared in any
 *     public header, and TaktOSTickInit's header declaration also
 *     reinforces C linkage). ─────────────────────────────────────────── */
extern "C" bool  TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr);
extern "C" void *TaktKernelStackInit(void *pStackTop, void (*pThreadFct)(void *), void *pArg);
extern "C" void  TaktOSTickInit(uint32_t KernClockHz, uint32_t TickHz, TaktOSTickClockSrc_t TickClockSrc);
extern "C" int   TaktHalTrapDispatch(void);


/* TCB pSp must be at offset 0 — ctx_switch.S loads it with
 * `lw sp, TCB_SP(t1)` using TCB_SP=0. */
static_assert(offsetof(TaktOSThread_t, pSp) == 0u,
              "TCB_SP must be at TaktOSThread_t offset 0 — see ctx_switch.S");


/* ── mcause codes (machine mode) ─────────────────────────────────────── */
#define TAKT_MCAUSE_M_SOFT          0x80000003u
#define TAKT_MCAUSE_M_TIMER         0x80000007u

/* ── CLINT register offsets from base ────────────────────────────────── */
#define TAKT_CLINT_MTIMECMP_LO_OFF  0x4000u
#define TAKT_CLINT_MTIMECMP_HI_OFF  0x4004u
#define TAKT_CLINT_MTIME_LO_OFF     0xBFF8u
#define TAKT_CLINT_MTIME_HI_OFF     0xBFFCu

/* ── mie bits ────────────────────────────────────────────────────────── */
#define TAKT_MIE_MSIE               (1u << 3)
#define TAKT_MIE_MTIE               (1u << 7)


/* Tick period in mtime ticks, captured by the CLINT default of
 * TaktOSTickInit and consumed by the timer arm of TaktHalTrapDispatch.
 * Zero before init, or when a strong override programmed its own tick
 * source without writing here — the timer dispatch then leaves mtimecmp
 * unchanged and only runs the kernel tick. */
static uint32_t s_clint_period = 0u;


/* ── CLINT register helpers ────────────────────────────────────────────── */

static inline volatile uint32_t *taktClintReg(volatile uint32_t *base, uint32_t off)
{
    return (volatile uint32_t *)((uintptr_t)base + off);
}

static inline uint64_t taktClintReadMtime(volatile uint32_t *base)
{
    /* RV32 64-bit mtime read: re-read hi to guard against rollover between
     * the hi and lo reads. */
    volatile uint32_t *mtime_l = taktClintReg(base, TAKT_CLINT_MTIME_LO_OFF);
    volatile uint32_t *mtime_h = taktClintReg(base, TAKT_CLINT_MTIME_HI_OFF);
    uint32_t lo, hi, hi2;
    do {
        hi  = *mtime_h;
        lo  = *mtime_l;
        hi2 = *mtime_h;
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t taktClintReadMtimecmp(volatile uint32_t *base)
{
    volatile uint32_t *mtimecmp_l = taktClintReg(base, TAKT_CLINT_MTIMECMP_LO_OFF);
    volatile uint32_t *mtimecmp_h = taktClintReg(base, TAKT_CLINT_MTIMECMP_HI_OFF);
    return ((uint64_t)*mtimecmp_h << 32) | *mtimecmp_l;
}

static inline void taktClintWriteMtimecmp(volatile uint32_t *base, uint64_t v)
{
    volatile uint32_t *mtimecmp_l = taktClintReg(base, TAKT_CLINT_MTIMECMP_LO_OFF);
    volatile uint32_t *mtimecmp_h = taktClintReg(base, TAKT_CLINT_MTIMECMP_HI_OFF);
    /* Per RISC-V Priv Spec: set hi to max first to prevent a spurious timer
     * interrupt while lo is being updated, then write lo, then write the
     * real hi. */
    *mtimecmp_h = 0xFFFFFFFFu;
    *mtimecmp_l = (uint32_t)v;
    *mtimecmp_h = (uint32_t)(v >> 32);
}


/* ── 1. Bind mtvec ─────────────────────────────────────────────────────── */

extern "C" bool TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr)
{
    /* Direct mode mtvec — low 2 bits = 00.  When HandlerBaseAddr is 0 the
     * arch lib's own TaktArchTrapEntry receives all traps.  Non-zero means
     * the application owns its own trap base (custom vector table or a
     * different entry point); the arch lib still requires that whatever
     * entry is installed saves a 128-byte frame matching port_rv32.h and
     * calls TaktHalTrapDispatch. */
    uint32_t tvec = (uint32_t)(HandlerBaseAddr != 0u
                               ? HandlerBaseAddr
                               : (uintptr_t)TaktArchTrapEntry) & ~3u;
    __asm__ volatile ("csrw mtvec, %0" :: "r"(tvec) : "memory");
    return false;  /* PMP not configured by default */
}


/* ── 2. Build a fresh task's initial trap frame ────────────────────────── */

extern "C" void *TaktKernelStackInit(void *pStackTop,
                                     void (*pThreadFct)(void *),
                                     void *pArg)
{
    /* RISC-V ABI: 16-byte stack alignment. */
    uintptr_t sp = ((uintptr_t)pStackTop) & ~(uintptr_t)15u;

    /* Allocate the 128-byte frame. */
    sp -= FRAME_SIZE;
    uint32_t *frame = reinterpret_cast<uint32_t *>(sp);

    /* Zero the whole frame so unset regs come up at 0 on first entry —
     * matches the ARM port's behavior.  Inline loop avoids pulling in
     * <string.h> / libc memset on freestanding. */
    for (uint32_t i = 0u; i < (FRAME_SIZE / 4u); ++i) {
        frame[i] = 0u;
    }

    /* mepc = task entry; mret jumps here. */
    frame[FRAME_MEPC    / 4u] = reinterpret_cast<uint32_t>(pThreadFct);

    /* mstatus: MPP = M-mode, MPIE = 1 → mret enables MIE. */
    frame[FRAME_MSTATUS / 4u] = MSTATUS_TASK_INIT;

    /* ra = trampoline — invoked if the task function returns. */
    frame[FRAME_RA      / 4u] = reinterpret_cast<uint32_t>(TaktOSThreadExit);

    /* a0 = task argument (RISC-V ABI: first arg in x10). */
    frame[FRAME_A0      / 4u] = reinterpret_cast<uint32_t>(pArg);

    /* gp and tp: capture the values set by startup so the first mret into
     * this task gets the correct globals-pointer.  Bare-metal single-
     * address-space builds keep these constant for the life of the system,
     * so capturing once at task creation is sufficient.  Leaving them zero
     * would crash any code generated with -mrelax (GCC default) that uses
     * GP-relative addressing for global access. */
    uint32_t cur_gp;
    uint32_t cur_tp;
    __asm__ volatile ("mv %0, gp" : "=r"(cur_gp));
    __asm__ volatile ("mv %0, tp" : "=r"(cur_tp));
    frame[FRAME_GP      / 4u] = cur_gp;
    frame[FRAME_TP      / 4u] = cur_tp;

    /* All other registers (t0–t6, a1–a7, s0–s11) remain zero from the loop. */

    return reinterpret_cast<void *>(sp);
}


/* ── 3. Weak: tick source init (CLINT default) ──────────────────────────── */

__attribute__((weak))
void TaktOSTickInit(uint32_t KernClockHz, uint32_t TickHz,
                    TaktOSTickClockSrc_t TickClockSrc)
{
    (void)TickClockSrc;  /* RV32 CLINT has a single mtime clock domain */

    if (g_TaktSoftIntReg == NULL || KernClockHz == 0u || TickHz == 0u) {
        return;
    }

    s_clint_period = KernClockHz / TickHz;

    volatile uint32_t *clint_base = g_TaktSoftIntReg;
    uint64_t now = taktClintReadMtime(clint_base);
    taktClintWriteMtimecmp(clint_base, now + s_clint_period);

    /* Enable machine timer + machine software interrupts.  mstatus.MIE is
     * enabled by mret in TaktOSStartFirst via the first task's mstatus.MPIE
     * bit — not set here. */
    uint32_t mie;
    __asm__ volatile ("csrr %0, mie" : "=r"(mie));
    mie |= (TAKT_MIE_MTIE | TAKT_MIE_MSIE);
    __asm__ volatile ("csrw mie, %0" :: "r"(mie) : "memory");
}


/* ── 4. Weak: trap dispatch (CLINT default) ──────────────────────────── */

__attribute__((weak))
int TaktHalTrapDispatch(void)
{
    uint32_t mcause;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause));

    if (mcause == TAKT_MCAUSE_M_TIMER) {
        /* Re-arm mtimecmp by exactly one period — phase-aligned, drift-free.
         * If s_clint_period is 0 (strong override of TaktOSTickInit didn't
         * use the CLINT path), skip the re-arm.  The kernel tick still runs
         * so timed waits advance. */
        if (g_TaktSoftIntReg != NULL && s_clint_period != 0u) {
            volatile uint32_t *clint_base = g_TaktSoftIntReg;
            uint64_t cmp = taktClintReadMtimecmp(clint_base);
            taktClintWriteMtimecmp(clint_base, cmp + s_clint_period);
        }
        TaktKernelTickHandler();
        return 0;  /* tick handler pends a swap via TaktOSCtxSwitch if needed —
                    * the resulting MSIP fires as the next trap, mirroring
                    * the ARM SysTick → PendSV tail-chain. */
    }

    if (mcause == TAKT_MCAUSE_M_SOFT) {
        if (g_TaktSoftIntReg != NULL) {
            *g_TaktSoftIntReg = 0u;
        }
        return 1;
    }

    /* External IRQs, exceptions — leave to a strong override.  Return 0 so
     * the arch lib doesn't swap; the trap restore resumes the interrupted
     * instruction.  A synchronous exception (illegal instruction, load
     * fault, ...) loops here unless overridden. */
    return 0;
}
