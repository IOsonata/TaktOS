/**---------------------------------------------------------------------------
@file   taktos_thread.cpp

@brief  TaktOS thread lifecycle  create, sleep list, suspend/resume/destroy

Memory block layout:
  [ TaktOSThread_t | guard (0xDEADBEEF) | arch-reserved slack/frame | usable stack ... top ]
  Stack grows DOWN from top.  TAKTOS_THREAD_MEM_SIZE() supplies the full
  per-architecture allocation, including initial-frame bytes and any alignment
  or guard-region slack needed by the active port.

Tick wraparound safety:
  g_tickCount is uint32_t and wraps every ~49.7 days at 1 kHz.  All
  sleep-list comparisons use signed subtraction:
    (int32_t)(a - b) < 0    tick 'a' fires before tick 'b'
  This is correct across the full uint32_t rollover.

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
#include <string.h>
#include <stddef.h>
#include "TaktOS.h"
#include "TaktOSThread.h"

// TAKTOS_STACK_GUARD_ALIGN defined in TaktOSThread.h

// Assembly files load pStackBottom directly by numeric offset.
// Fires at compile time if the TCB layout changes unexpectedly.
// pStackBottom must be at offset 16 on 32-bit targets so PendSV assembly
// can load it by numeric offset (TCB_STACK_BOTTOM = 16).
// Guard for 32-bit only  pointers are 8 bytes on 64-bit hosts.
#if defined(__arm__) || defined(__thumb__) || defined(__riscv)
static_assert(offsetof(TaktOSThread_t, pStackBottom) == 16u,
              "TCB_STACK_BOTTOM mismatch -- update PendSV_M*.S defines");
#endif

#define TAKTOS_GUARD_WORD     0xDEADBEEFu
#define TAKTOS_GUARD_OFFSET   sizeof(TaktOSThread_t)

//--- Wraparound-safe tick comparison ------------------------------------
// Returns true if tick 'a' fires strictly before tick 'b'.
// Correct across the full uint32_t rollover as long as the gap < 2^31.
/**
 * @brief	Serial-number comparison for 32-bit tick counters.
 *
 * Returns true if tick value @p a is strictly before @p b in the wrap-around
 * sense.  Signed subtraction keeps comparisons correct across the UINT32_MAX0
 * boundary.
 *
 * @param	a : First tick count.
 * @param	b : Second tick count.
 * @return	true if a is before b (wrap-around aware).
 */
static inline bool tick_before(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) < 0;
}

//--- Sleep list  sorted earliest-deadline first ------------------------
static TaktOSThread_t *g_sleepList = nullptr;

/**
 * @brief	Insert a thread into the sleep list in deadline order.
 *
 * The list is sorted by WakeTick ascending so TaktThreadWakeTick() only
 * needs to check the head.  Wrap-safe via tick_before().
 * Must be called inside a critical section.
 *
 * @param	t : Thread to insert (WakeTick must already be set).
 */
void TaktSleepListAdd(TaktOSThread_t *t)
{
    TaktOSThread_t **p = &g_sleepList;
    // Walk while the existing node's deadline is at or before t's deadline.
    while (*p != nullptr && !tick_before(t->WakeTick, (*p)->WakeTick))
    {
        p = &(*p)->pNext;
    }
    t->pNext = *p;
    *p       = t;
}

/**
 * @brief	Remove a thread from the sleep list (O(n)).
 *
 * Called when a timed wait completes early or TaktThreadWakeTick() removes
 * an expired entry.  No-op if @p t is not in the list.
 * Must be called inside a critical section.
 *
 * @param	t : Thread to remove.
 */
void TaktSleepListRemove(TaktOSThread_t *t)
{
    TaktOSThread_t **p = &g_sleepList;
    while (*p != nullptr && *p != t)
    {
        p = &(*p)->pNext;
    }
    if (*p == t)
    {
        *p = t->pNext;
        t->pNext = nullptr;
    }
}

//--- Hot-path helpers ---------------------------------------------------

/**
 * @brief	Block the current thread and trigger a context switch (hot path).
 *
 * Marks the calling thread BLOCKED, removes it from the run queue inside a
 * critical section, then pends PendSV / CLINT MSIP.  Inlined to avoid an
 * extra frame on slow-path callers that immediately return after this.
 */
static inline void TaktSuspendCurrentFast(void)
{
    uint32_t primask = TaktOSEnterCritical();
    TaktOSThread_t *cur = TaktOSCurrentThread();
    cur->State = TAKTOS_BLOCKED;
    TaktBlockTask(cur);
    TaktOSExitCritical(primask);
    TaktOSCtxSwitch();
}

/**
 * @brief	Make a blocked thread ready and preempt if higher priority (hot path).
 *
 * Marks @p t READY, inserts it into the run queue inside a critical section,
 * then pends a context switch if @p t has strictly higher priority than the
 * current thread.  Inlined into all resume hot paths.
 *
 * @param	t : Thread to resume (must currently be BLOCKED or SLEEPING).
 */
static inline void TaktResumeBlockedFast(TaktOSThread_t *t)
{
    uint32_t primask = TaktOSEnterCritical();
    t->State = TAKTOS_READY;
    TaktReadyTask(t);
    TaktOSThread_t *cur = TaktOSCurrentThread();
    bool preempt = (cur != nullptr) && (t->Priority > cur->Priority);
    TaktOSExitCritical(primask);
    if (preempt)
    {
        TaktOSCtxSwitch();
    }
}

//--- Create -------------------------------------------------------------

/**
 * @brief	Create a thread  implementation.
 *
 * Validates the memory block, aligns pStackBottom to TAKTOS_STACK_GUARD_ALIGN,
 * writes the 0xDEADBEEF guard word, calls TaktKernelStackInit() to build the initial
 * exception frame at an ABI-aligned stack top, and inserts the thread into the
 * run queue.
 *
 * @see TaktOSThreadCreate in TaktOSThread.h for the full API.
 */
hTaktOSThread_t TaktOSThreadCreate(void *pMem, uint32_t MemSize,
                                    void (*pEntry)(void*), void *pArg,
                                    uint8_t Priority)
{
    if (pMem == nullptr || pEntry == nullptr)
    {
        return nullptr;
    }

    if (MemSize < TAKTOS_THREAD_MEM_SIZE(TAKTOS_THREAD_MIN_STACK_SIZE) ||
    	Priority == 0u || Priority >= TAKTOS_MAX_PRI)
    {
        return nullptr;
    }

    TaktOSThread_t *t  = (TaktOSThread_t*)pMem;
    uint32_t *guard    = (uint32_t*)((uint8_t*)pMem + sizeof(TaktOSThread_t));//TAKTOS_GUARD_OFFSET);
    *guard = TAKTOS_GUARD_WORD;

    t->Priority     = Priority;
    t->State        = TAKTOS_READY;
    t->WakeReason   = TAKT_WOKEN_BY_EVENT;
    t->_pad1        = 0u;
    t->WakeTick     = 0u;
    t->pNext        = nullptr;
    t->pStackBottom = nullptr;
    t->pWaitNext    = nullptr;
    t->pWaitList    = nullptr;
    t->pMsg         = nullptr;
    t->pSp          = TaktKernelStackInit((uint8_t*)pMem + MemSize, pEntry, pArg);

    {
        uintptr_t raw  = (uintptr_t)((uint8_t*)pMem + TAKTOS_GUARD_OFFSET + sizeof(uint32_t));
        uint32_t mask = TAKTOS_STACK_GUARD_ALIGN - 1u;
        t->pStackBottom = (void*)((raw + mask) & ~mask);
    }

    uint32_t primask = TaktOSEnterCritical();
    TaktReadyTask(t);
    TaktOSExitCritical(primask);

    return t;
}

//--- Suspend ------------------------------------------------------------

/**
 * @brief	Suspend a thread  implementation.  @see TaktOSThread.h.
 */
TaktOSErr_t TaktOSThreadSuspend(hTaktOSThread_t hThread)
{
    if (hThread == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    // Self-suspend: the fast path calls TaktSuspendCurrentFast() which
    // handles its own locking.  This check is safe because TaktOSCurrentThread()
    // reads a pointer that is only written by the context-switch handler, and
    // the context-switch handler cannot fire between two consecutive reads of the
    // same register on a single-core MCU (no partial preemption of a comparison).
    if (hThread == TaktOSCurrentThread())
    {
        TaktSuspendCurrentFast();
        return TAKTOS_OK;
    }

    // Foreign-thread suspend: ALL state reads must happen under the lock.
    //
    // The original code read hThread->State before TaktOSEnterCritical() and
    // branched on the unlocked value.  Two failure modes:
    //
    //   Lost suspend  target was BLOCKED, ISR wakes it (BLOCKEDREADY),
    //   then Suspend() returns OK with the thread now running.
    //
    //   TaktBlockTask() on non-queued thread  target was READY at the unlocked
    //   read but a concurrent operation blocked it before we took the lock.
    //   The else branch at line 215 called TaktBlockTask() which assumes the
    //   thread is in the run queue; on a circular singly-linked list this walks
    //   forever (infinite loop) or corrupts the list.
    uint32_t primask = TaktOSEnterCritical();
    TaktOSState_t state = hThread->State;

    if (state == TAKTOS_DEAD)
    {
        TaktOSExitCritical(primask);
        return TAKTOS_ERR_INVALID;
    }

    if (state == TAKTOS_BLOCKED)
    {
        // Thread is already suspended.  If there is no active timed wait,
        // nothing to do.  If there is, cancel it so a subsequent Give/Unlock
        // never wakes a suspended thread.
        if (hThread->pWaitList == nullptr && hThread->WakeTick == 0u)
        {
            TaktOSExitCritical(primask);
            return TAKTOS_OK;
        }
        if (hThread->pWaitList != nullptr)
        {
            TaktWaitListRemove(hThread->pWaitList, hThread);
            hThread->pWaitList  = nullptr;
            // Administrative cancellation by Suspend  not a real timeout.
            // Slow paths check for TAKT_WOKEN_BY_RESUME and return
            // TAKTOS_ERR_INTERRUPTED so callers can distinguish external
            // cancellation from an actual deadline expiry.
            hThread->WakeReason = TAKT_WOKEN_BY_RESUME;
        }
        if (hThread->WakeTick != 0u)
        {
            TaktSleepListRemove(hThread);
            hThread->WakeTick = 0u;
        }
        TaktOSExitCritical(primask);
        return TAKTOS_OK;
    }

    if (state == TAKTOS_SLEEPING)
    {
        TaktSleepListRemove(hThread);
        hThread->WakeTick = 0u;
        if (hThread->pWaitList != nullptr)
        {
            TaktWaitListRemove(hThread->pWaitList, hThread);
            hThread->pWaitList  = nullptr;
            hThread->WakeReason = TAKT_WOKEN_BY_RESUME;  // same: administrative cancellation
        }
        hThread->State = TAKTOS_BLOCKED;
        TaktOSExitCritical(primask);
        return TAKTOS_OK;
    }

    // READY or RUNNING: remove from the run queue and block.
    hThread->State = TAKTOS_BLOCKED;
    TaktBlockTask(hThread);
    TaktOSExitCritical(primask);
    return TAKTOS_OK;
}

//--- ResumeSlowPath  SLEEPING state (cold, out-of-line) ---------------
// Called only from the inline TaktOSThreadResume fast path when
// t->State == TAKTOS_SLEEPING.  Removes from the sleep list, cancels
// any active timed kernel-object wait, then makes the thread READY.
// Kept out-of-line so the BLOCKED hot path in the inline caller has no
// function-call overhead and no extra register spill in its frame.

TAKT_COLD TAKT_NOINLINE
/**
 * @brief	Slow path for TaktOSThreadResume  handles SLEEPING threads.
 *
 * Called when the thread is found in the SLEEPING state.  Removes it from the
 * sleep list, stamps TAKT_WOKEN_BY_RESUME, and makes it ready.  The critical
 * section entered by the caller is transferred to this function which exits it.
 *
 * @param	t        : Thread to resume (must be SLEEPING).
 * @param	primask  : Saved interrupt state from the caller's critical section.
 * @return	TAKTOS_OK on success.
 */
TaktOSErr_t TaktOSThreadResumeSlowPath(TaktOSThread_t *t, uint32_t primask)
{
    // Must remove from sleep list before TaktReadyTask 
    // pNext is shared between the run queue and the sleep list.
    TaktSleepListRemove(t);
    t->WakeTick = 0u;  // Bug fix: clear stale WakeTick
    if (t->pWaitList != nullptr)
    {
        TaktWaitListRemove(t->pWaitList, t);
        t->pWaitList = nullptr;
    }

    t->State = TAKTOS_READY;
    TaktReadyTask(t);

    TaktOSThread_t *cur = TaktOSCurrentThread();
    bool preempt = (cur != nullptr) && (t->Priority > cur->Priority);
    TaktOSExitCritical(primask);
    if (preempt)
    {
        TaktOSCtxSwitch();
    }
    return TAKTOS_OK;
}

//--- Sleep --------------------------------------------------------------
// Bug fix: TaktOSCtxSwitch() is only called when t == current.
// Previously it was called unconditionally, causing the calling thread
// to lose the CPU even when sleeping a different (foreign) thread.

/**
 * @brief	Sleep a thread  implementation.  @see TaktOSThread.h.
 */
TaktOSErr_t TaktOSThreadSleep(hTaktOSThread_t hThread, uint32_t ticks)
{
    if (hThread == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }
    if (ticks == 0u)
    {
        return TAKTOS_OK;
    }

    bool isCurrent = (hThread == TaktOSCurrentThread());

    uint32_t primask = TaktOSEnterCritical();
    // Guard: TaktBlockTask assumes t is in the run queue (READY).
    // Sleeping a SLEEPING or BLOCKED foreign thread would loop forever.
    if (!isCurrent && hThread->State != TAKTOS_READY)
    {
        TaktOSExitCritical(primask);
        return TAKTOS_ERR_INVALID;
    }
    hThread->WakeTick   = TaktOSTickCount() + ticks;
    hThread->State      = TAKTOS_SLEEPING;
    hThread->pWaitList = nullptr;   // plain sleep  not a timed kernel-object wait
    TaktBlockTask(hThread);
    TaktSleepListAdd(hThread);
    TaktOSExitCritical(primask);

    if (isCurrent)
    {
        TaktOSCtxSwitch();
    }

    return TAKTOS_OK;
}

//--- Destroy ------------------------------------------------------------

/**
 * @brief	Destroy a thread  implementation.  @see TaktOSThread.h.
 *
 * Removes the thread from the run queue, sleep list, and any kernel-object
 * wait list it may be blocked on, then marks the TCB as TAKTOS_DEAD.
 */
TaktOSErr_t TaktOSThreadDestroy(hTaktOSThread_t hThread)
{
    if (hThread == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t primask = TaktOSEnterCritical();

    // Guard against double-destroy or stale-handle calls.
    if (hThread->State == TAKTOS_DEAD)
    {
        TaktOSExitCritical(primask);
        return TAKTOS_ERR_INVALID;
    }

    if (hThread->State == TAKTOS_SLEEPING)
    {
        TaktSleepListRemove(hThread);
        hThread->WakeTick = 0u;  // node unlinked; clear stale deadline
        if (hThread->pWaitList != nullptr)
        {
            TaktWaitListRemove(hThread->pWaitList, hThread);
            hThread->pWaitList = nullptr;
        }
    }
    else if (hThread->State == TAKTOS_BLOCKED)
    {
        // Thread may be in a kernel-object wait list (sem/mutex/queue).
        // Pull it out now so Give/Unlock never wakes a destroyed TCB.
        if (hThread->pWaitList != nullptr)
        {
            TaktWaitListRemove(hThread->pWaitList, hThread);
            hThread->pWaitList = nullptr;
        }
        // Also remove from sleep list if it had a finite timeout.
        if (hThread->WakeTick != 0u)
        {
            TaktSleepListRemove(hThread);
            hThread->WakeTick = 0u;
        }
    }
    else if (hThread->State == TAKTOS_READY)
    {
        TaktBlockTask(hThread);
    }

    // Stamp DEAD before clearing the guard word, while still holding the lock.
    // Any concurrent Resume/Suspend that is spinning waiting for the lock will
    // see DEAD on the next iteration and return ERR_INVALID cleanly.
    hThread->State = TAKTOS_DEAD;
    uint32_t *guard = (uint32_t*)((uint8_t*)hThread + TAKTOS_GUARD_OFFSET);
    *guard = 0u;

    bool isCurrent = (hThread == TaktOSCurrentThread());
    TaktOSExitCritical(primask);

    if (isCurrent)
    {
        TaktOSCtxSwitch();
    }

    return TAKTOS_OK;
}

//--- HandOff ------------------------------------------------------------

/**
 * @brief	Hand off to a specific thread  implementation.  @see TaktOSThread.h.
 *
 * Wakes @p hNext (cancelling any pending blocked wait), calls
 * TaktForceNextThread() to ensure it runs next, then blocks the caller.
 * Both operations execute inside a single critical section to prevent a
 * race where a higher-priority thread becomes ready between the two steps.
 */
TaktOSErr_t TaktOSThreadHandOff(hTaktOSThread_t hNext)
{
    if (hNext == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    TaktOSThread_t *cur = TaktOSCurrentThread();

    if (hNext == cur)
    {
        TaktOSThreadYield();
        return TAKTOS_OK;
    }

    uint32_t primask = TaktOSEnterCritical();

    if (hNext->State == TAKTOS_BLOCKED)
    {
        // Cancel any kernel-object wait this thread was blocked in.
        // Stamp RESUME so the slow path reports ERR_INTERRUPTED, not OK.
        if (hNext->pWaitList != nullptr)
        {
        	hNext->WakeReason = TAKT_WOKEN_BY_RESUME;
            TaktWaitListRemove(hNext->pWaitList, hNext);
            hNext->pWaitList = nullptr;
            if (hNext->WakeTick != 0u)
            {
                TaktSleepListRemove(hNext);
                hNext->WakeTick = 0u;
            }
        }
        hNext->State = TAKTOS_READY;
        TaktReadyTask(hNext);
    }
    else if (hNext->State == TAKTOS_SLEEPING)
    {
        TaktSleepListRemove(hNext);
        hNext->WakeTick = 0u;
        if (hNext->pWaitList != nullptr)
        {
            // Thread was mid-timed kernel-object wait (sem/mutex/queue).
            // Stamp RESUME so the blocking slow path returns ERR_INTERRUPTED,
            // not OK  the token/lock/item was NOT transferred.
            hNext->WakeReason = TAKT_WOKEN_BY_RESUME;
            TaktWaitListRemove(hNext->pWaitList, hNext);
            hNext->pWaitList = nullptr;
        }
        hNext->State = TAKTOS_READY;
        TaktReadyTask(hNext);
    }
    else if (hNext->State != TAKTOS_READY)
    {
        TaktOSExitCritical(primask);
        return TAKTOS_ERR_INVALID;
    }

    cur->State = TAKTOS_BLOCKED;
    TaktBlockTask(cur);

    // Guarantee the handoff: TaktReadyTask inserts hNext at the tail of its
    // priority ring, but pNextThread always points to the front (oldest entry).
    // At same priority hNext would not run next; at lower priority it would
    // never run next.  TaktForceNextThread sets pNextThread = hNext unless a
    // strictly higher-priority preemptor is already waiting  overriding that
    // would be a priority inversion.
    TaktForceNextThread(hNext);

    TaktOSExitCritical(primask);
    TaktOSCtxSwitch();
    return TAKTOS_OK;
}

//--- Guard check --------------------------------------------------------

/**
 * @brief	Check the stack guard word  implementation.  @see TaktOSThread.h.
 */
bool TaktOSThreadGuardCheck(TaktOSThread_t *pThread)
{
    const uint32_t *guard = (const uint32_t*)((const uint8_t*)pThread + TAKTOS_GUARD_OFFSET);
    return (*guard == TAKTOS_GUARD_WORD);
}

//--- Stack overflow hook (weak default) ---------------------------------

/**
 * @brief	Stack overflow hook  weak default implementation.
 *
 * Called from TaktThreadWakeTick() with interrupts disabled when the guard
 * word of a thread is found corrupt.  This default traps immediately.
 * Applications may override this symbol with a strong definition to log the
 * offending thread handle before halting or triggering a system reset.
 *
 * @see TaktOSStackOverflowHandler in TaktOSThread.h for the full API.
 */
TAKT_WEAK void TaktOSStackOverflowHandler(hTaktOSThread_t)
{
    TAKT_TRAP();
    for (;;)
    {
    }
}

//--- TaktThreadWakeTick  called by TaktKernelTickHandler() 
//
// Sole tick-driven action of the kernel.  Walks the sleep list and
// makes every thread whose deadline has arrived ready to run.
//
// Wraparound-safe: (int32_t)(WakeTick - tickCount) <= 0 fires correctly
// even when WakeTick has wrapped past 0.
//
// Timed kernel-object waits (pWaitList != nullptr):
//   Thread is in BOTH the sleep list AND an object wait list.
//   On timeout: remove from object wait list, mark WakeReason=TIMEOUT.
//
// Note: software timers, interrupt management, and memory are NOT
// handled here or anywhere in the kernel  those belong to the HAL
// (IOsonata).  See TaktKernel.h for the full ownership boundary.

/**
 * @brief	Wake threads whose sleep deadline has expired  implementation.
 *
 * Called by TaktKernelTickHandler() on every tick.  Walks the sorted sleep
 * list from the head and wakes all threads whose WakeTick  tickCount.
 * Stamps TAKT_WOKEN_BY_TIMEOUT so TaktOSSemTake / TaktOSMutexLock can
 * distinguish a timeout from a normal wakeup.
 *
 * @param	tickCount : Current global tick count.
 */
void TaktThreadWakeTick(uint32_t tickCount)
{
    while (g_sleepList != nullptr
           && (int32_t)(g_sleepList->WakeTick - tickCount) <= 0)
    {
        TaktOSThread_t *t = g_sleepList;
        g_sleepList = t->pNext;
        t->pNext    = nullptr;
        t->WakeTick = 0u;
        t->State    = TAKTOS_READY;

        if (t->pWaitList != nullptr)
        {
            // Timed kernel-object wait expired.
            TaktWaitListRemove(t->pWaitList, t);
            t->pWaitList = nullptr;
            t->WakeReason = TAKT_WOKEN_BY_TIMEOUT;
        }
        // else: plain sleep  wakeReason stays TAKT_WOKEN_BY_EVENT (0)

        TaktReadyTask(t);
    }

}

/**
 * @brief	Resume a thread  implementation.  @see TaktOSThread.h.
 */
TaktOSErr_t TaktOSThreadResume(hTaktOSThread_t hThread)
{
    if (hThread == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    // Optimistic unlocked pre-check.  READY/RUNNING and DEAD are common
    // early-exit cases that avoid the EnterCritical overhead entirely.
    // This read is racy  we re-verify every branch under the lock below.
    TaktOSState_t stateFast = hThread->State;
    if (stateFast == TAKTOS_READY || stateFast == TAKTOS_RUNNING)
    {
        return TAKTOS_OK;
    }
    if (stateFast == TAKTOS_DEAD)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t primask = TaktOSEnterCritical();

    // Re-read State under the lock.  An ISR may have woken the thread
    // (BLOCKED  READY) or destroyed it in the window between the unlocked
    // read above and TaktOSEnterCritical().  Without this second check we
    // would call TaktReadyTask() on a thread already in the run queue,
    // corrupting the circular ready list.
    TaktOSState_t state = hThread->State;

    if (state == TAKTOS_READY || state == TAKTOS_RUNNING)
    {
        TaktOSExitCritical(primask);
        return TAKTOS_OK;
    }
    if (state == TAKTOS_DEAD)
    {
        TaktOSExitCritical(primask);
        return TAKTOS_ERR_INVALID;
    }

    if (TAKT_UNLIKELY(state == TAKTOS_SLEEPING))
    {
        return TaktOSThreadResumeSlowPath(hThread, primask);
    }

    // BLOCKED  READY.  Remove from kernel-object wait list if the thread
    // was blocked by sem/mutex/queue (pWaitList always set for those waits,
    // including WAIT_FOREVER).  For threads blocked by explicit suspend,
    // pWaitList is null  no cleanup needed.
    if (hThread->pWaitList != nullptr)
    {
        // Thread was blocked inside a kernel-object wait.  Mark as
        // externally woken so the slow path returns ERR_INTERRUPTED
        // instead of falsely reporting a successful token/lock/item transfer.
    	hThread->WakeReason = TAKT_WOKEN_BY_RESUME;
        TaktWaitListRemove(hThread->pWaitList, hThread);
        hThread->pWaitList = nullptr;
        if (hThread->WakeTick != 0u)
        {
            TaktSleepListRemove(hThread);
            hThread->WakeTick = 0u;
        }
    }
    hThread->State = TAKTOS_READY;
    TaktReadyTask(hThread);

    TaktOSThread_t *cur = TaktOSCurrentThread();
    bool preempt = (cur != nullptr) && (hThread->Priority > cur->Priority);
    TaktOSExitCritical(primask);
    if (preempt)
    {
        TaktOSCtxSwitch();
    }
    return TAKTOS_OK;
}


//--- Thread exit --------------------------------------------------------

/**
 * @brief Thread exit trampoline called when an entry function returns.
 */
extern "C" void TaktOSThreadExit(void)
{
    TaktOSThreadDestroy(TaktOSCurrentThread());
    for (;;)
    {
    }
}
