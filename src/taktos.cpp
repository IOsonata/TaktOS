/**---------------------------------------------------------------------------
@file   taktos.cpp

@brief  TaktOS kernel  initialisation, scheduler, tick handler, and idle task

Clock note:
  TaktOSInit(&cfg) reads cfg.KernClockHz / cfg.TickHz / cfg.TickClockSrc /
  cfg.TickPriority and passes them unchanged to TaktOSTickInit() at startup.
  The kernel never interprets
  KernClockHz as the CPU frequency  it is solely the input clock of the
  tick peripheral.  The caller (application or HAL) is responsible for
  supplying the correct value for the target device.

Run queue semantics:
  pHead[pri] = TAIL of the circular singly-linked list.
    pHead[pri]->pNext = FRONT = next-to-run candidate.
    Insert: O(1)  new node becomes new tail, no list walk.
    Yield:  O(1)  current moves from front to back (new tail).
    Block:  O(n)  must find predecessor (unavoidable, singly-linked).

Safety boundary: IN  MC/DC coverage required.

@author Nguyen Hoan Hoang
@date   Apr. 2026

@license

MIT License

Copyright (c) 2026 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/
#include <stddef.h>
#include <string.h>

#include "TaktOSThread.h"
#include "TaktCompiler.h"
#include "TaktKernelTick.h"
#if defined(TAKTOS_WAKE_TRACE_ENABLE)
#include "TaktOSWakeTrace.h"
#endif

/* Port-portable fallback for the software-interrupt trigger default.
 * Ports that need a non-zero default (RV32 → CLINT MSIP) define this
 * macro in their TaktKernelCore.h (pulled in transitively above).  ARM
 * Cortex-M doesn't use this value — its TaktOSCtxSwitch is hardcoded to
 * the architecturally fixed SCB ICSR — so the 0 fallback is fine. */
#ifndef TAKT_DEFAULT_SOFT_INT_ADDR
#define TAKT_DEFAULT_SOFT_INT_ADDR  0u
#endif

//--- Run queue  definition is in TaktKernel.h (inline primitives) ---

uint32_t g_TickCount;
TaktRunQueue_t g_TaktRunQueue;

// Deferred-context-switch trigger register pointer.  Set by TaktOSInit() from
// the SoftIntAddr parameter.  Used by TaktOSCtxSwitch() in TaktOSCriticalSection.h
// for ports that need a configurable MMIO address (RISC-V).  ARM Cortex-M ports
// ignore this and use the architecturally fixed SCB ICSR.
volatile uint32_t *g_TaktSoftIntReg = nullptr;

static uint32_t s_KernClockHz;		// Tick peripheral input clock in Hz
static uint32_t s_TickFreq;			// Tick freq in Hz
static TaktOSTickClockSrc_t s_TickClockSrc;	// Selected tick clock path
static uint32_t s_TickPriority;		// Tick interrupt priority (TaktOS standard scale)
static bool s_KernelMpuActive;        // true when arch bound MPU-aware kernel handlers

// Deferred yield  set by TaktOSThreadYield() when called from an ISR or
// inside a critical section.  Stores the TCB pointer of the thread that
// requested the yield so the tick handler can verify pCurrent has not changed
// before consuming the request.  If pCurrent has changed by the time the tick
// fires (e.g. the requesting thread blocked or was preempted), the request is
// discarded rather than applied to the wrong thread.
//
// Written from a user ISR (higher priority than SysTick), read from SysTick.
// On single-core Cortex-M these accesses are always serialized, so volatile
// is sufficient  no atomic is needed.
static volatile TaktOSThread_t *s_DeferredYieldFor = nullptr;

TaktKernelCtx_t g_TaktosCtx;

#if defined(TAKTOS_WAKE_TRACE_ENABLE)
//--- Wake trace diagnostics --------------------------------------

extern "C" TaktOSWakeTrace_t g_TaktWakeTrace;
TaktOSWakeTrace_t g_TaktWakeTrace;

static inline uint32_t TaktWakeTraceCycleCount(void)
{
#if defined(TAKT_ARCH_STUB) || defined(__x86_64__) || defined(__i386__) || defined(__riscv)
    return 0u;
#else
    return *((volatile uint32_t*)0xE0001004u);   // DWT CYCCNT
#endif
}

extern "C" uint32_t TaktOSWakeTraceSupported(void)
{
#if defined(TAKTOS_WAKE_TRACE_ENABLE)
    return 1u;
#else
    return 0u;
#endif
}

extern "C" void TaktOSWakeTraceReset(void)
{
    g_TaktWakeTrace.Sequence = 0u;
    g_TaktWakeTrace.TickCount = 0u;
    g_TaktWakeTrace.ExpiredCount = 0u;
    g_TaktWakeTrace.SwitchRequested = 0u;
    g_TaktWakeTrace.CurrentPriority = 0u;
    g_TaktWakeTrace.NextPriority = 0u;
    g_TaktWakeTrace.TickEntryCycles = 0u;
    g_TaktWakeTrace.AfterWakeCycles = 0u;
    g_TaktWakeTrace.BeforePendSVCycles = 0u;
    g_TaktWakeTrace.PendSVEntryCycles = 0u;
    g_TaktWakeTrace.PendSVReturnCycles = 0u;
}

extern "C" void TaktOSWakeTraceEnable(uint32_t Enable)
{
    if (Enable != 0u)
    {
        TaktOSWakeTraceReset();
        g_TaktWakeTrace.Enabled = 1u;
    }
    else
    {
        g_TaktWakeTrace.Enabled = 0u;
    }
}

extern "C" const TaktOSWakeTrace_t *TaktOSWakeTraceSnapshot(void)
{
    return &g_TaktWakeTrace;
}

#endif

//--- Context block  ABI ---------------------------
// These offsets are consumed by PendSV/ctx_switch assembly.  Only valid on
// 32-bit targets where pointers are 4 bytes.  Host (x86_64/i386) stub builds
// use 8-byte pointers so the assert would always fail there  skip it.
#if !defined(TAKT_ARCH_STUB) && !defined(__x86_64__) && !defined(__i386__)
static_assert(offsetof(TaktKernelCtx_t, pCurrent)     == 0, "CTX_CURRENT broken");
static_assert(offsetof(TaktKernelCtx_t, pNextThread) == 4, "CTX_NEXT_THREAD broken");
#endif

//--- Idle thread -------------------------------------------------

// Idle-thread usable payload above the reserved frame/guard overhead.
#define TAKTOS_IDLE_THREAD_STACKSIZE      16u

// No special object alignment is required here.  TAKTOS_THREAD_MEM_SIZE()
// already reserves the worst-case slack needed to align pStackBottom upward and
// to align the initial stack top downward inside the supplied memory block.
static uint8_t s_IdleThreadMem[TAKTOS_THREAD_MEM_SIZE(TAKTOS_IDLE_THREAD_STACKSIZE)];

/**
 * @brief	Idle thread entry  runs when no other thread is ready.
 *
 * Spins in an infinite loop.  The CPU enters WFI each time PendSV fires and
 * nothing else is ready, so in practice the idle thread rarely executes
 * instruction cycles.  Priority 0 (TAKTOS_PRIORITY_IDLE) ensures it is
 * always pre-empted by any user thread.
 *
 * @param	pArg : Unused (pass NULL).
 */
static void IdleThread(void *pArg)
{
    (void)pArg;
    for (;;)
    {
    }
}

extern "C" TAKT_WEAK bool TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr)
{
    (void)HandlerBaseAddr;

    return false;
}


//--- Scheduler primitives  inline in TaktKernel.h --------------
// TaktPrioBitmapSet/Clear/Top, TaktUpdateNextThread, TaktReadyTask,
// TaktForceNextThread are all defined as TAKT_ALWAYS_INLINE in
// TaktKernel.h so every caller gets the body folded at the call site.
//
// TaktBlockTask is kept out-of-line here because its body (~120 bytes)
// includes the O(n) predecessor-walk loop. Inlining that loop at every
// call site would cost ~600 bytes across the kernel for code whose loop
// is never taken in TM3/TM2 (every priority is singleton there).

/**
 * @brief	Remove a thread from the READY run queue and mark it BLOCKED.
 *          See TaktKernel.h for the full spec.  Returns true on
 *          success, false on detected scheduler-ring corruption.
 */
bool TaktBlockTask(TaktOSThread_t *pThread)
{
    uint8_t pri = pThread->Priority;
    TaktOSThread_t *tail = g_TaktRunQueue.pHead[pri];

    if (tail == nullptr)
    {
        /* Internal scheduler-state corruption: pThread->Priority points
         * at a priority ring whose head is null.  The thread cannot be
         * in this ring; the run queue and pThread are inconsistent.
         * Report the failure to the caller; the caller decides whether
         * to halt, log, drive outputs to a safe state, or attempt
         * recovery.  Kernel does not impose a policy. */
        return false;
    }

    if (tail == pThread && pThread->pNext == pThread)
    {
        // only thread at this priority  fast path
        g_TaktRunQueue.pHead[pri] = nullptr;
        TaktPrioBitmapClear(pri);

        if (pri == g_TaktRunQueue.TopPri)
        {
            g_TaktRunQueue.TopPri = TaktPrioBitmapTop();
            TaktUpdateNextThread();
        }
    }
    else
    {
        // Find predecessor.  Start from tail (wraps to front on first step),
        // but never walk forever: pNext corruption or a caller passing a thread
        // that is not in the READY ring must fail closed instead of hanging the
        // kernel inside this loop.
        TaktOSThread_t *prev = tail;

        while (prev != nullptr && prev->pNext != pThread)
        {
            prev = prev->pNext;

            if (prev == tail)
            {
                // Completed one full ring without seeing pThread.
                prev = nullptr;
                break;
            }
        }

        if (prev == nullptr)
        {
            /* Internal scheduler-state corruption: full ring walk did
             * not find pThread, but pThread->Priority claims it is in
             * this ring.  Report the failure; caller's policy. */
            return false;
        }

        TaktOSThread_t *next = pThread->pNext;
        prev->pNext = next;

        if (tail == pThread)
        {
            g_TaktRunQueue.pHead[pri] = prev;   // pThread was tail; prev becomes new tail
        }

        if (pri == g_TaktRunQueue.TopPri && g_TaktosCtx.pNextThread == pThread)
        {
            g_TaktosCtx.pNextThread = next;     // next front already known
        }
    }

    pThread->pNext = nullptr;
    return true;
}


//--- Yield  minimal Thread-mode overhead ------------------------
/*
 * For cooperative yield, the goal is to match FreeRTOS's zero-overhead
 * Thread-mode path: one ICSR write to pend PendSV.
 *
 * The challenge: pNextThread is precomputed and PendSV reads it directly,
 * so we must rotate the circular list (move current to tail) before PendSV
 * fires.  We still need a critical section to protect the two-word update
 * (pHead[] and pNextThread) against a concurrent SysTick that could write
 * pNextThread to a higher-priority thread between the two stores.
 *
 * Critical section design  unconditional CPSID I / CPSIE I:
 *   TaktOSDisableInterrupts() / TaktOSEnableInterrupts() are used here
 *   instead of the MRS/MSR PRIMASK save-restore pattern.  The reason is
 *   performance: with save-restore, GCC cannot fit all live values in the
 *   four caller-save registers (r0–r3) and must spill to a callee-save
 *   register (r4), generating PUSH {r4,lr} / POP {r4,pc}.  Combined with
 *   MRS and MSR instruction costs, this adds ~10–12 cycles per yield on
 *   Cortex-M4/M33  a measurable regression on TM2 cooperative scheduling.
 *
 * Caller rules and how they are enforced:
 *
 *   Rule 1  must not be called from an ISR (Handler mode), because
 *     g_TaktosCtx.pCurrent is stale during ISR execution.
 *
 *   Rule 2  must not be called with PRIMASK already set (Thread mode inside
 *     a critical section), because the unconditional CPSIE I below would
 *     re-enable interrupts inside the caller's locked region.
 *
 *   In both cases the caller's intent is valid  they want to yield.  The
 *   only problem is timing.  Both cases stamp s_DeferredYieldFor with the
 *   current TCB and return.  The tick handler performs the rotation on the
 *   next tick only if pCurrent still matches the stamped TCB  if the
 *   requesting thread has blocked or been preempted the stale request is
 *   discarded silently.  The caller's critical section (if any) is not
 *   disturbed because we never reach the CPSIE I.
 *
 * Singleton optimisation: only enter the critical section when there IS a
 * peer to yield to.  No lock at all when cur is the only thread at this
 * priority.
 */
void TaktOSThreadYield(void)
{
    // Caller-mode guards  always active in all builds, including production.
    //
    // Two situations prevent an immediate yield:
    //
    //   1. Called from Handler mode (any ISR or fault).
    //      g_TaktosCtx.pCurrent points to the last preempted Thread-mode task,
    //      not the ISR.  Performing the rotation now would write a stale
    //      pNextThread.
    //
    //   2. Called from Thread mode with PRIMASK already set (inside a critical
    //      section).  The unconditional CPSIE I below would re-enable interrupts
    //      inside the caller's locked region, breaking atomicity.
    //
    // In both cases the caller asked to yield  the intent is valid.  The only
    // problem is timing.  In both cases: stamp s_DeferredYieldFor with the
    // current TCB and return.  The tick handler consumes the request on the
    // next tick, but only if pCurrent still matches  if the requesting thread
    // blocked or was preempted in the interim, the stale request is discarded.
    // Neither case touches CPSIE, so the caller's critical section (if any)
    // is preserved intact until it releases it naturally.
#if defined(TAKT_ARCH_CM0) || defined(TAKT_ARCH_CM4) || defined(TAKT_ARCH_CM7) || \
    defined(TAKT_ARCH_CM33) || defined(TAKT_ARCH_CM55)
    {
        uint32_t ipsr, primask;
        asm volatile ("MRS %0, IPSR"    : "=r"(ipsr));
        asm volatile ("MRS %0, PRIMASK" : "=r"(primask));

        if ((ipsr & 0x1FFu) || (primask & 1u))
        {
            s_DeferredYieldFor = g_TaktosCtx.pCurrent;
            return;
        }
    }
#endif

    TaktOSThread_t *cur = g_TaktosCtx.pCurrent;

    if (cur->pNext == cur)
    {
        return;                  /* singleton  nothing to yield to */
    }

    // Both writes must be atomic w.r.t. ISRs that call TaktReadyTask.
    // An ISR between the two stores could set pNextThread to a higher-priority
    // thread and have Yield() overwrite it with the same-priority peer, losing
    // an immediate preemption decision.  Critical section is required.
    TaktOSDisableInterrupts();
    g_TaktRunQueue.pHead[cur->Priority] = cur;
    // Guard: don't overwrite pNextThread if a higher-priority thread is already
    // pending (TaktReadyTask raised TopPri between the singleton check and here).
    if (cur->Priority >= g_TaktRunQueue.TopPri)
    {
        g_TaktosCtx.pNextThread = cur->pNext;
    }
    TaktOSEnableInterrupts();

    TaktOSCtxSwitch();
}

//--- Init --------------------------------------------------------

/**
 * @brief	Initialize the kernel  @see TaktOSInit in TaktOS.h for full API.
 *
 * Implementation notes:
 *   - Zeroes the run queue, context struct, and tick counter.
 *   - Creates the idle thread (priority 0) using @c s_IdleThreadMem.
 *   - Computes pStackBottom from the stack floor inside the thread block, then
 *     rounds it up to TAKTOS_STACK_GUARD_ALIGN for ports that use it as a
 *     hardware guard-region base or stack-limit register value.
 *   - Stores KernClockHz, TickHz, and TickClockSrc for use by TaktOSStart().
 *   - KernClockHz is the input clock of the tick peripheral, NOT the CPU clock.
 *
 * @param	KernClockHz   : Tick peripheral input clock in Hz.
 * @param	TickHz        : Desired kernel tick rate in Hz.
 * @param	TickClockSrc  : Tick clock source selector.
 * @param	HandlerBaseAddr : Architecture-defined exception/trap handler base
 *                          address (ARM: RAM vector-table base for application
 *                          override, 0 to keep statically linked handlers).
 *                          See TaktOS.h for the full per-architecture spec.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if arguments are out of range.
 */
TaktOSErr_t TaktOSInit(const TaktOSCfg_t *Cfg)
{
    if (Cfg == nullptr || Cfg->KernClockHz == 0u) {
        return TAKTOS_ERR_INVALID;
    }

    /* Apply defaults for any zero field, so the minimum-effort init
     *    TaktOSCfg_t cfg = { .KernClockHz = X };  just works on the
     * standard configuration for the active port.  TickClockSrc needs
     * no special handling because TAKTOS_TICK_CLOCK_PROCESSOR == 0 — a
     * zero-initialised field naturally selects the default. */
    uint32_t   tickHz  = (Cfg->TickHz      != 0u) ? Cfg->TickHz     : TAKTOS_DEFAULT_TICK_HZ;
    uintptr_t  softInt = (Cfg->SoftIntAddr != 0u) ? Cfg->SoftIntAddr : TAKT_DEFAULT_SOFT_INT_ADDR;
    /* HandlerBaseAddr 0 already means "use port default" per TaktKernelHandlerAssign
     * convention; the port resolves it. */

    memset(&g_TaktRunQueue, 0, sizeof(g_TaktRunQueue));
    memset(&g_TaktosCtx, 0, sizeof(g_TaktosCtx));

    s_DeferredYieldFor = nullptr;   // clear any stale deferred yield from prior run
    g_TaktRunQueue.TopPri = 0u;
    g_TickCount = 0;
    s_KernClockHz     = Cfg->KernClockHz;
    s_TickFreq        = tickHz;
    s_TickClockSrc    = Cfg->TickClockSrc;
    /* TickPriority uses the TaktOS standard priority scale and is forwarded
     * unchanged.  Conversion to the interrupt controller encoding, clamping,
     * and resolution of TAKTOS_TICK_PRIORITY_DEFAULT (0) to the hardware
     * default are done by the port in TaktOSTickInit(). */
    s_TickPriority    = Cfg->TickPriority;
    s_KernelMpuActive = TaktKernelHandlerAssign(Cfg->HandlerBaseAddr);
    g_TaktSoftIntReg  = (volatile uint32_t *)softInt;

    // Idle thread at priority 0  bypasses user priority guard

    TaktOSThread_t *idle = (TaktOSThread_t*)s_IdleThreadMem;
    uint32_t *guard = (uint32_t*)((uint8_t*)s_IdleThreadMem + sizeof(TaktOSThread_t));
    *guard = 0xDEADBEEFu;
    void *stackTop = s_IdleThreadMem + sizeof(s_IdleThreadMem);
    idle->Priority   = 0u;
    idle->State      = TAKTOS_READY;
    idle->WakeReason = TAKT_WOKEN_BY_EVENT;
    idle->BasePriority = 0u;
    idle->WakeTick   = 0u;
    idle->pNext        = nullptr;
    // pStackBottom: set to the aligned stack floor (above the guard word).
    // Required when the arch uses pStackBottom as an MPU guard base or stack
    // limit register value. Null is unsafe for those paths.
    {
        uintptr_t raw  = (uintptr_t)((uint8_t*)s_IdleThreadMem + sizeof(TaktOSThread_t) + sizeof(uint32_t));
        uint32_t  mask = TAKTOS_STACK_GUARD_ALIGN - 1u;
        idle->pStackBottom = (void*)((raw + mask) & ~(uintptr_t)mask);
    }
    idle->pWaitNext    = nullptr;
    idle->pWaitList    = nullptr;
    idle->pMsg         = nullptr;
    idle->HeldCeilingTop = 0u;
    idle->pSp = TaktKernelStackInit(stackTop, IdleThread, nullptr);

    TaktReadyTask(idle);

    return TAKTOS_OK;
}

// Test-only accessor  not compiled in normal production or release builds.
#ifdef TAKT_TEST_ACCESS
/**
 * @brief	Return the thread currently registered for deferred yield (test only).
 *
 * Exposes @c s_DeferredYieldFor for unit tests that need to inspect scheduler
 * state without executing a context switch.
 * Only available when TAKT_TEST_ACCESS is defined.
 *
 * @return	Pointer to the pending deferred-yield thread, or nullptr if none.
 */
extern "C" TaktOSThread_t *TaktTestGetDeferredYieldFor(void)
{
    return (TaktOSThread_t *)s_DeferredYieldFor;
}

/**
 * @brief	Inject a deferred-yield thread for testing (test only).
 *
 * Directly sets @c s_DeferredYieldFor so tests can verify that the tick
 * handler consumes or discards it correctly.
 * Only available when TAKT_TEST_ACCESS is defined.
 *
 * @param	t : Thread to register as the deferred-yield target, or nullptr to clear.
 */
extern "C" void TaktTestSetDeferredYieldFor(TaktOSThread_t *t)
{
    s_DeferredYieldFor = t;
}
#endif

/**
 * @brief	Start the scheduler  @see TaktOSStart in TaktOS.h.
 *
 * Initialises the tick source, selects the first thread to run, then calls the
 * arch-specific TaktOSStartFirst() to hand control to that thread.  Never returns.
 */
void TaktOSStart(void)
{
    TaktUpdateNextThread();
    TaktOSTickInit(s_KernClockHz, s_TickFreq, s_TickClockSrc, s_TickPriority);
    TaktOSStartFirst();

    for (;;)
    {
    }
}

/**
 * @brief	Return the configured kernel tick rate  @see TaktOSGetTickHz in TaktOS.h.
 *
 * Reads s_TickFreq, written once by TaktOSInit() and never modified
 * afterwards.  No critical section needed — single-write/multi-read.
 */
uint32_t TaktOSGetTickHz(void)
{
    return s_TickFreq;
}

//--- Tick --------------------------------------------------------
/*
 * Critical section is required.  SysTick runs at lowest priority (0xFF) so
 * any user ISR with a numerically lower priority value can preempt it.  If
 * that ISR calls a kernel API (TaktOSSemGive, TaktOSThreadResume, etc.) it
 * enters TaktReadyTask/TaktBlockTask, which mutate g_TaktRunQueue and pNextThread
 * concurrently with TaktThreadWakeTick doing the same  classic race.
 *
 * Pattern matches TaktOSSemGive: hold PRIMASK across all scheduler state
 * mutation, capture the preemption decision inside the lock, release PRIMASK,
 * then pend PendSV outside.  TaktOSCtxSwitch() is a single ICSR store and
 * is safe to call without a lock.
 */
extern "C" __attribute__((aligned(16))) void TaktKernelTickHandler(void)
{
#if defined(TAKTOS_WAKE_TRACE_ENABLE)
    if (g_TaktWakeTrace.Enabled != 0u)
    {
        g_TaktWakeTrace.Sequence = g_TaktWakeTrace.Sequence + 1u;
        g_TaktWakeTrace.TickEntryCycles = TaktWakeTraceCycleCount();
        g_TaktWakeTrace.ExpiredCount = 0u;
        g_TaktWakeTrace.SwitchRequested = 0u;
        g_TaktWakeTrace.TickCount = g_TickCount + 1u;
        g_TaktWakeTrace.CurrentPriority = g_TaktosCtx.pCurrent ? g_TaktosCtx.pCurrent->Priority : 0u;
        g_TaktWakeTrace.NextPriority = g_TaktosCtx.pNextThread ? g_TaktosCtx.pNextThread->Priority : 0u;
        g_TaktWakeTrace.AfterWakeCycles = 0u;
        g_TaktWakeTrace.BeforePendSVCycles = 0u;
        g_TaktWakeTrace.PendSVEntryCycles = 0u;
        g_TaktWakeTrace.PendSVReturnCycles = 0u;
    }
#endif

    uint32_t primask = TaktOSEnterCritical();

    g_TickCount++;
    TaktThreadWakeTick(g_TickCount);
#if defined(TAKTOS_WAKE_TRACE_ENABLE)
    if (g_TaktWakeTrace.Enabled != 0u)
    {
        g_TaktWakeTrace.AfterWakeCycles = TaktWakeTraceCycleCount();
    }
#endif
    // current is always valid after TaktOSStart  no null check needed
    bool needSwitch = (g_TaktosCtx.pNextThread->Priority > g_TaktosCtx.pCurrent->Priority);

    // Deferred yield  consume the request set by TaktOSThreadYield() when
    // called from an ISR or inside a critical section.  Only honour it if
    // pCurrent is still the thread that requested it; if the requesting thread
    // has blocked, slept or been preempted since the request was made, discard
    // the request rather than rotating the wrong thread.
    if (s_DeferredYieldFor != nullptr)
    {
        TaktOSThread_t *requester = (TaktOSThread_t *)s_DeferredYieldFor;
        s_DeferredYieldFor = nullptr;
        TaktOSThread_t *cur = g_TaktosCtx.pCurrent;

        if (requester == cur && cur->pNext != cur)
        {
            // Rotate: cur becomes new tail, pNextThread advances to cur->pNext.
            // Guard: same as TaktOSThreadYield  don't overwrite pNextThread
            // if a higher-priority thread was made ready since the last switch.
            g_TaktRunQueue.pHead[cur->Priority] = cur;

            if (cur->Priority >= g_TaktRunQueue.TopPri)
            {
                g_TaktosCtx.pNextThread = cur->pNext;
                needSwitch = true;
            }
        }
    }

    // Stack overflow guard check  only when no hardware stack limit is present.
    // On M33/M55: PSPLIM fires before PSP can reach the guard word  not needed.
    // On M4/M7 with MPU: MPU region 7 fires before PSP can reach guard word  not needed.
    // On M0, M4/M7 without MPU, RISC-V: software guard word is the only protection.
#if defined(TAKTOS_SOFT_STACK_CHECK)
    if (!s_KernelMpuActive && !TaktOSThreadGuardCheck(g_TaktosCtx.pCurrent))
    {
        TaktOSExitCritical(primask);
        TaktOSStackOverflowHandler(g_TaktosCtx.pCurrent);

        return;   // hook should not return; this is a safety backstop
    }
#endif

    TaktOSExitCritical(primask);

#if defined(TAKTOS_WAKE_TRACE_ENABLE)
    if (g_TaktWakeTrace.Enabled != 0u)
    {
        g_TaktWakeTrace.SwitchRequested = needSwitch ? 1u : 0u;
        g_TaktWakeTrace.CurrentPriority = g_TaktosCtx.pCurrent ? g_TaktosCtx.pCurrent->Priority : 0u;
        g_TaktWakeTrace.NextPriority = g_TaktosCtx.pNextThread ? g_TaktosCtx.pNextThread->Priority : 0u;
        g_TaktWakeTrace.BeforePendSVCycles = TaktWakeTraceCycleCount();
    }
#endif

    if (needSwitch)
    {
        TaktOSCtxSwitch();
    }
}


