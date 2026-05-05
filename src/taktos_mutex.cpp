/**---------------------------------------------------------------------------
@file   taktos_mutex.cpp

@brief  TaktOS binary mutex  init, fast/slow paths, IPCP boost/unboost

Two flavors share the same struct: plain (Ceiling == 0) and priority-ceiling
(Ceiling != 0).  The Ceiling field on the mutex selects the path; the test
costs one byte compare on the fast path.

Plain mutex behavior is unchanged from v1.0:
  - Lock free          -> acquire, no priority change, return OK.
  - Lock owned         -> ERR_BUSY (no-wait) or block on wait list.
  - Unlock with waiter -> hand ownership to highest-priority waiter.

IPCP mutex behavior (Immediate Priority Ceiling Protocol):
  - Lock free, caller_pri <= Ceiling -> save BasePriority on first PCP acquire,
                                        push Ceiling onto TCB.HeldCeilings,
                                        migrate caller to max(caller_pri, Ceiling).
  - Lock free, caller_pri >  Ceiling -> ERR_INVALID  (configuration error;
                                        caller is outside declared locker set).
  - Lock free, HeldCeilingTop full   -> ERR_INVALID  (nesting too deep).
  - Lock owned                       -> same as plain (ceiling check still runs first).
  - Unlock                           -> pop ceiling from caller's stack,
                                        restore caller_pri = max(BasePriority,
                                        max of remaining HeldCeilings).
  - Unlock with waiter (Ceiling!=0)  -> apply ceiling boost to new owner before
                                        making it READY.

The held-ceiling stack lives in the TCB (HeldCeilings[TAKTOS_MAX_HELD_PCP_MUTEXES],
HeldCeilingTop).  Stack is small on purpose; deeper nesting is a design smell
and overflow is reported as ERR_INVALID rather than silently corrupting the
restore arithmetic.

Timeout semantics (unchanged):
  TAKTOS_WAIT_FOREVER  blocks indefinitely.
  finite ticks         blocks up to N ticks, then returns ERR_TIMEOUT.
  Same dual-list pattern as semaphore: thread inserted in both the mutex
  wait list and g_sleepList.  Whichever fires first wins.

Fast/slow split (TAKT_INLINE_OPTIMIZATION):
  - Plain-mutex fast paths (uncontended, Ceiling == 0, no waiter on Unlock)
    are the inline forms in TaktOSMutex.h when TAKT_INLINE_OPTIMIZATION is
    defined.  When undefined, this file provides equivalent out-of-line public
    entries below behind #ifndef TAKT_INLINE_OPTIMIZATION.
  - TaktOSMutexLockSlow and TaktOSMutexUnlockSlow are the wider entry points
    that handle PCP and contention cases.  They are reached either from the
    inline header (on miss) or from the out-of-line public entries (on miss).
  - TaktOSMutexLockSlowPath and TaktOSMutexUnlockSlowPath are the deepest
    blocking entries  same bodies as in v1.0.

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
#include "TaktOSMutex.h"
#include "TaktOSThread.h"

//--- IPCP helpers (file-local) ----------------------------------

/**
 * @brief  Push @p Ceiling onto the holder's held-ceiling stack.
 *
 * Called inside the kernel critical section when a thread acquires an IPCP
 * mutex.  Records the ceiling so Unlock can restore the correct priority,
 * and saves BasePriority on the first PCP acquire (HeldCeilingTop == 0).
 *
 * @return  true on success, false if the stack is full (caller must reject
 *          the acquire with TAKTOS_ERR_INVALID).
 */
TAKT_ALWAYS_INLINE bool TaktPushHeldCeiling(TaktOSThread_t *pThread, uint8_t Ceiling)
{
    if (pThread->HeldCeilingTop >= TAKTOS_MAX_HELD_PCP_MUTEXES)
    {
        return false;
    }

    if (pThread->HeldCeilingTop == 0u)
    {
        // First PCP mutex held by this thread.  Snapshot current priority as
        // the un-boost target.  BasePriority was initialised in ThreadCreate
        // and is updated only when the thread holds zero PCP mutexes.
        pThread->BasePriority = pThread->Priority;
    }

    pThread->HeldCeilings[pThread->HeldCeilingTop] = Ceiling;
    pThread->HeldCeilingTop++;
    return true;
}

/**
 * @brief  Pop @p Ceiling from the holder's held-ceiling stack.
 *
 * Called inside the kernel critical section on IPCP mutex Unlock.  Removes
 * the matching entry (typically the top, but may be any slot for nested-out-
 * of-order release patterns) and shifts subsequent entries down.
 */
TAKT_ALWAYS_INLINE void TaktPopHeldCeiling(TaktOSThread_t *pThread, uint8_t Ceiling)
{
    uint8_t i;
    for (i = pThread->HeldCeilingTop; i > 0u; i--)
    {
        if (pThread->HeldCeilings[i - 1u] == Ceiling)
        {
            // Shift later entries down (preserves order for the multi-mutex case).
            for (uint8_t j = i - 1u; j + 1u < pThread->HeldCeilingTop; j++)
            {
                pThread->HeldCeilings[j] = pThread->HeldCeilings[j + 1u];
            }
            pThread->HeldCeilingTop--;
            return;
        }
    }
    // Not found: caller drove an inconsistent state; leave stack untouched.
    // This path is reachable only via direct misuse of the kernel internals;
    // public Unlock guards on pOwner == current.
}

/**
 * @brief  Compute the effective priority for an IPCP-aware thread.
 *
 * Returns max(BasePriority, max of all entries in HeldCeilings[0..HeldCeilingTop)).
 * When HeldCeilingTop == 0 this collapses to BasePriority alone.
 */
TAKT_ALWAYS_INLINE uint8_t TaktComputeEffectivePri(const TaktOSThread_t *pThread)
{
    uint8_t pri = pThread->BasePriority;
    for (uint8_t i = 0u; i < pThread->HeldCeilingTop; i++)
    {
        if (pThread->HeldCeilings[i] > pri)
        {
            pri = pThread->HeldCeilings[i];
        }
    }
    return pri;
}

//--- Init -------------------------------------------------------

/**
 * @brief	Initialize a plain (non-PCP) mutex.  @see TaktOSMutex.h.
 */
TaktOSErr_t TaktOSMutexInit(TaktOSMutex_t *pMtx)
{
    if (pMtx == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    pMtx->pOwner    = nullptr;
    pMtx->WaitList.pHead = nullptr;
    pMtx->Ceiling   = 0u;             // 0 = non-PCP

    return TAKTOS_OK;
}

/**
 * @brief	Initialize a priority-ceiling (IPCP) mutex.  @see TaktOSMutex.h.
 */
TaktOSErr_t TaktOSMutexInitProtect(TaktOSMutex_t *pMtx, uint8_t Ceiling)
{
    if (pMtx == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    // Ceiling 0 (idle) is reserved.  Ceiling >= TAKTOS_MAX_PRI is out of range.
    if (Ceiling == 0u || Ceiling >= TAKTOS_MAX_PRI)
    {
        return TAKTOS_ERR_INVALID;
    }

    pMtx->pOwner    = nullptr;
    pMtx->WaitList.pHead = nullptr;
    pMtx->Ceiling   = Ceiling;

    return TAKTOS_OK;
}

//--- Lock slow path (deepest: blocks on contended mutex) --------

/**
 * @brief	Slow path for TaktOSMutexLock  implementation.  @see TaktOSMutex.h.
 *
 * Called when the caller has already failed to acquire the mutex (held by
 * another thread).  Inserts the caller into the wait list, arms the sleep
 * timer if TimeoutTicks is finite, then blocks.  IPCP boost on the caller's
 * eventual acquire is applied by the unlocking owner inside
 * TaktOSMutexUnlockSlowPath.
 */
TaktOSErr_t TaktOSMutexLockSlowPath(TaktOSMutex_t *pMtx, uint32_t IntState,
                                    TaktOSThread_t *current, uint32_t TimeoutTicks)
{
    /* ISR-context guard.  Blocking from an ISR would block the preempted
     * thread (passed as `current` by the caller's TaktOSCurrentThread()
     * read), corrupting the scheduler.  Cert boundary: reject without
     * touching state.  Slow path only  uncontended Lock does not run
     * this check. */
    if (TAKT_UNLIKELY(TaktOSInIsr()))
    {
        TaktOSExitCritical(IntState);
        return TAKTOS_ERR_INVALID;
    }

    current->State      = TAKTOS_BLOCKED;
    current->WakeReason = TAKT_WOKEN_BY_EVENT;
    current->WakeTick   = 0u;
    current->pWaitNext  = nullptr;

    if (!TaktBlockTask(current))
    {
        /* Scheduler-ring corruption detected.  Restore state, exit
         * the lock, return so the application gets a clean
         * ERR_INVALID and applies its own recovery policy. */
        current->State = TAKTOS_RUNNING;
        TaktOSExitCritical(IntState);
        return TAKTOS_ERR_INVALID;
    }
    TaktWaitListInsert(&pMtx->WaitList, current);

    // Always store pWaitList so Resume/HandOff can cancel this wait.
    current->pWaitList = &pMtx->WaitList;

    if (TimeoutTicks != TAKTOS_WAIT_FOREVER)
    {
        // Tick timeout expires when the global tick reaches now + TimeoutTicks.
        current->WakeTick = TaktOSTickCount() + TimeoutTicks;
        TaktSleepListAdd(current);
    }
    // WAIT_FOREVER: WakeTick already 0 from the init above.

    TaktOSExitCritical(IntState);
    TaktOSCtxSwitch();

    const uint8_t wr = current->WakeReason;

    if (__builtin_expect((int)wr, 0))
    {
        if (wr == TAKT_WOKEN_BY_TIMEOUT)
        {
            return TAKTOS_ERR_TIMEOUT;
        }

        return TAKTOS_ERR_INTERRUPTED;   // TAKT_WOKEN_BY_RESUME
    }

    return TAKTOS_OK;
}

//--- Unlock slow path (deepest: hands off to waiter) ------------

/**
 * @brief	Slow path for TaktOSMutexUnlock  implementation.  @see TaktOSMutex.h.
 *
 * Pops the highest-priority waiter, transfers ownership, applies the IPCP
 * boost to the new owner if the mutex has a Ceiling, and preempts if the new
 * owner outranks the (possibly already un-boosted) caller.
 */
TaktOSErr_t TaktOSMutexUnlockSlowPath(TaktOSMutex_t *pMtx, uint32_t IntState,
                                      TaktOSThread_t *current)
{
    TaktOSThread_t *next = TaktWaitListPop(&pMtx->WaitList);

    if (next != nullptr)
    {
        // Cancel the waiter's sleep-list entry if it had a finite timeout.
        next->pWaitList = nullptr;   // clear before making READY

        if (next->WakeTick != 0u)
        {
            TaktSleepListRemove(next);
            next->WakeTick = 0u;
        }
        next->WakeReason = TAKT_WOKEN_BY_EVENT;

        // IPCP: apply ceiling boost to the new owner BEFORE making it READY,
        // so it is enqueued at the boosted priority and selected accordingly.
        // Stack overflow on the new owner is theoretically possible but in
        // practice means the new owner already holds TAKTOS_MAX_HELD_PCP_MUTEXES
        // PCP mutexes  application bug.  We skip the push and let the caller
        // observe the priority anomaly rather than corrupt the held stack.
        if (pMtx->Ceiling != 0u)
        {
            (void)TaktPushHeldCeiling(next, pMtx->Ceiling);
            uint8_t boostedPri = TaktComputeEffectivePri(next);
            next->Priority = boostedPri;
        }

        pMtx->pOwner = next;
        next->State = TAKTOS_READY;
        TaktReadyTask(next);

        bool needSwitch = (next->Priority > current->Priority) ||
        				  (current->State != TAKTOS_READY &&
        				   current->State != TAKTOS_RUNNING);

        TaktOSExitCritical(IntState);

        if (needSwitch)
        {
        	TaktOSCtxSwitch();
        }
    }
    else
    {
        pMtx->pOwner = nullptr;
        TaktOSExitCritical(IntState);
    }

    return TAKTOS_OK;
}

//--- Wider slow entries (PCP + contention dispatch) -------------

/**
 * @brief  Wider slow entry for Lock  reached when the inline plain-mutex
 *         fast path declined.  See header for the spec.
 *
 * Caller has already done: NULL check on pMtx, TaktOSEnterCritical().  At
 * least one of the two fast-path conditions failed: either Ceiling != 0 or
 * pOwner != NULL (or both).  This function decides which sub-case applies:
 *   - Ceiling != 0 + pOwner == NULL  -> PCP acquire.
 *   - Ceiling != 0 + pOwner != NULL  -> contended; ERR_BUSY or block.
 *   - Ceiling == 0 + pOwner != NULL  -> contended; ERR_BUSY or block.
 */
TaktOSErr_t TaktOSMutexLockSlow(TaktOSMutex_t *pMtx, uint32_t state,
                                bool bBlocking, uint32_t timeoutTicks)
{
    TaktOSThread_t *current = TaktOSCurrentThread();
    const uint8_t ceiling = pMtx->Ceiling;

    // IPCP precondition: caller priority must not exceed the declared ceiling.
    // Strict-on-violation: configuration errors fail loudly rather than silently
    // running with the PCP invariant broken.  Compares BasePriority (not the
    // possibly-boosted current Priority) so a thread that is currently boosted
    // by another PCP mutex is judged on its design-time priority.  Skipped for
    // plain mutex (ceiling == 0).
    if (ceiling != 0u && current->BasePriority > ceiling)
    {
        TaktOSExitCritical(state);
        return TAKTOS_ERR_INVALID;
    }

    if (TAKT_LIKELY(pMtx->pOwner == nullptr))
    {
        if (ceiling != 0u)
        {
            // PCP acquire: push held ceiling, set owner, migrate priority.
            if (!TaktPushHeldCeiling(current, ceiling))
            {
                TaktOSExitCritical(state);
                return TAKTOS_ERR_INVALID;
            }

            pMtx->pOwner = current;

            // Migrate to boosted priority.  No-op if ceiling <= current Priority
            // (already running at or above ceiling because of an outer PCP mutex
            // with a higher ceiling).
            const uint8_t boosted = TaktComputeEffectivePri(current);
            if (boosted != current->Priority)
            {
                if (!TaktMigratePriority(current, boosted))
                {
                    /* Scheduler corruption detected during the priority
                     * boost.  Roll back the held-
                     * ceiling push and the owner assignment so the mutex
                     * is left in its pre-Lock state for the application
                     * fault hook. */
                    TaktPopHeldCeiling(current, ceiling);
                    pMtx->pOwner = nullptr;
                    TaktOSExitCritical(state);
                    return TAKTOS_ERR_INVALID;
                }
            }

            TaktOSExitCritical(state);
            return TAKTOS_OK;
        }

        // Plain mutex, uncontended.  Reached when the caller used the
        // out-of-line public entry (TAKT_INLINE_OPTIMIZATION undefined) and
        // arrived here because the public entry didn't take its inlined
        // fast-path branch  e.g. mid-function debug build.  Behaviour is
        // identical to the inline form.
        pMtx->pOwner = current;
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    // Contended (pOwner != NULL).
    if (TAKT_UNLIKELY(!bBlocking || timeoutTicks == TAKTOS_NO_WAIT))
    {
        TaktOSExitCritical(state);
        return TAKTOS_ERR_BUSY;
    }

    return TaktOSMutexLockSlowPath(pMtx, state, current, timeoutTicks);
}

/**
 * @brief  Wider slow entry for Unlock  reached when the inline plain-mutex
 *         fast path declined.  See header for the spec.
 *
 * Caller has already done: NULL check, EnterCritical, owner check
 * (pOwner == current).  At least one fast-path condition failed: either
 * Ceiling != 0 or WaitList.pHead != NULL (or both).
 */
TaktOSErr_t TaktOSMutexUnlockSlow(TaktOSMutex_t *pMtx, uint32_t state,
                                  TaktOSThread_t *current)
{
    const uint8_t ceiling = pMtx->Ceiling;

    // IPCP un-boost: pop the released ceiling and recompute caller priority
    // BEFORE deciding whether to wake a waiter or yield.  If the un-boost
    // drops below the run-queue top, the unlock-with-no-waiter path will
    // ctx-switch; the slow path handles the with-waiter case below.
    if (ceiling != 0u)
    {
        TaktPopHeldCeiling(current, ceiling);
        const uint8_t restored = TaktComputeEffectivePri(current);
        if (restored != current->Priority)
        {
            if (!TaktMigratePriority(current, restored))
            {
                /* Scheduler corruption detected during the priority
                 * un-boost.  Mutex ownership
                 * stays with current  caller still holds the lock
                 * conceptually; an application fault hook gets a
                 * clean ERR_INVALID and can drive recovery. */
                TaktOSExitCritical(state);
                return TAKTOS_ERR_INVALID;
            }
        }
    }

    if (TAKT_LIKELY(pMtx->WaitList.pHead == nullptr))
    {
        pMtx->pOwner = nullptr;

        // After un-boost, a higher-priority READY thread may now be available.
        // For non-PCP mutexes, current->Priority did not change, TopPri did
        // not change, no switch needed.  For PCP mutexes, check explicitly.
        bool needSwitch = (ceiling != 0u) &&
                          (current->Priority < g_TaktRunQueue.TopPri);

        TaktOSExitCritical(state);

        if (needSwitch)
        {
            TaktOSCtxSwitch();
        }
        return TAKTOS_OK;
    }

    return TaktOSMutexUnlockSlowPath(pMtx, state, current);
}

//--- Out-of-line public entries (when inlining disabled) --------

#ifndef TAKT_INLINE_OPTIMIZATION

/**
 * @brief  Acquire a mutex  out-of-line variant.  See TaktOSMutex.h.
 *
 * Body matches the inline form in the header: plain-mutex fast path first,
 * delegate to TaktOSMutexLockSlow on miss.  Built when TAKT_INLINE_OPTIMIZATION
 * is undefined  smaller code, slightly slower hot path due to BL/BX overhead.
 */
TaktOSErr_t TaktOSMutexLock(TaktOSMutex_t *pMtx, bool bBlocking, uint32_t timeoutTicks)
{
    if (pMtx == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();

    if (TAKT_LIKELY(pMtx->pOwner == nullptr) && TAKT_LIKELY(pMtx->Ceiling == 0u))
    {
        pMtx->pOwner = TaktOSCurrentThread();
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    return TaktOSMutexLockSlow(pMtx, state, bBlocking, timeoutTicks);
}

/**
 * @brief  Release a mutex  out-of-line variant.  See TaktOSMutex.h.
 *
 * Body matches the inline form in the header.
 */
TaktOSErr_t TaktOSMutexUnlock(TaktOSMutex_t *pMtx)
{
    if (pMtx == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();
    TaktOSThread_t *current = TaktOSCurrentThread();

    if (TAKT_UNLIKELY(pMtx->pOwner != current))
    {
        TaktOSExitCritical(state);
        return TAKTOS_ERR_INVALID;
    }

    if (TAKT_LIKELY(pMtx->Ceiling == 0u) &&
        TAKT_LIKELY(pMtx->WaitList.pHead == nullptr))
    {
        pMtx->pOwner = nullptr;
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    return TaktOSMutexUnlockSlow(pMtx, state, current);
}

#endif // !TAKT_INLINE_OPTIMIZATION
