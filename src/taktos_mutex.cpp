/**---------------------------------------------------------------------------
@file   taktos_mutex.cpp

@brief  TaktOS binary mutex  slow paths, priority inheritance, and init

Fast path (uncontended acquire) is inlined in TaktOSMutex.h.
Cold slow paths (contention, PI boosting, timeout, release with waiter) live here.

Timeout semantics:
  TAKTOS_WAIT_FOREVER  blocks indefinitely.
  finite ticks         blocks up to N ticks, then returns ERR_TIMEOUT.
  Same dual-list pattern as semaphore: thread inserted in both the mutex
  wait list and g_sleepList.  Whichever fires first wins.

Priority inheritance (v1.0):
  NOT IMPLEMENTED in v1.0.  ownerBasePri is reserved for a future release.
  Priority inversion is bounded only by FIFO ordering within a priority level.
  Define TAKT_MUTEX_PI=1 to get an explicit build error if your application
  requires PI semantics; do not rely on the OwnerBasePri field.

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

//--- Init -------------------------------------------------------

/**
 * @brief	Initialize a mutex  implementation.  @see TaktOSMutex.h.
 */
TaktOSErr_t TaktOSMutexInit(TaktOSMutex_t *pMtx)
{
    if (pMtx == nullptr)
    {
    	return TAKTOS_ERR_INVALID;
    }

    pMtx->pOwner    = nullptr;
    pMtx->WaitList.pHead = nullptr;

    return TAKTOS_OK;
}

//--- Lock slow path (Bug 2 fix) ---------------------------------

/**
 * @brief	Slow path for TaktOSMutexLock  implementation.  @see TaktOSMutex.h.
 *
 * Inserts the caller into the wait list, optionally boosts the owner's priority
 * (PI, v1.1), arms the sleep timer if TimeoutTicks is finite, then blocks.
 */
TaktOSErr_t TaktOSMutexLockSlowPath(TaktOSMutex_t *pMtx, uint32_t IntState,
                                    TaktOSThread_t *current, uint32_t TimeoutTicks)
{

    current->State      = TAKTOS_BLOCKED;
    current->WakeReason = TAKT_WOKEN_BY_EVENT;
    current->WakeTick   = 0u;
    current->pWaitNext  = nullptr;

    TaktBlockTask(current);
    TaktWaitListInsert(&pMtx->WaitList, current);

    // Always store pWaitList so Resume/HandOff can cancel this wait.
    current->pWaitList = &pMtx->WaitList;

    if (TimeoutTicks != TAKTOS_WAIT_FOREVER)
    {
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

//--- Unlock slow path -------------------------------------------

/**
 * @brief	Slow path for TaktOSMutexUnlock  implementation.  @see TaktOSMutex.h.
 *
 * Pops the highest-priority waiter, transfers ownership, and preempts if the
 * new owner has higher priority than the current thread.
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

/**
 * @brief Acquire a mutex.
 *
 * Fast path acquires immediately when unlocked. If already owned:
 * - non-blocking (or @ref TAKTOS_NO_WAIT) returns @ref TAKTOS_ERR_BUSY
 * - blocking delegates to slow path and may wait up to @p timeoutTicks
 *
 * @param mtx           Pointer to mutex.
 * @param blocking      True to block when owned by another thread.
 * @param timeoutTicks  Wait timeout in ticks (default caller-managed).
 *
 * @return TAKTOS_OK on success, otherwise TAKTOS_ERR_BUSY / timeout / interrupted / invalid.
 */
TaktOSErr_t TaktOSMutexLock(TaktOSMutex_t *pMtx, bool bBlocking, uint32_t TimeoutTicks)
{
    // Null check is unconditional: a null handle causes UB on the first field
    // access.  In release builds this is the only guard; TAKT_DEBUG_CHECKS adds
    // further diagnostics but must not be the only line of defence.
    if (pMtx == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();
    TaktOSThread_t *current = TaktOSCurrentThread();

    if (TAKT_LIKELY(pMtx->pOwner == nullptr))
    {
    	pMtx->pOwner = current;
        TaktOSExitCritical(state);

        return TAKTOS_OK;
    }

    if (TAKT_UNLIKELY(!bBlocking || TimeoutTicks == TAKTOS_NO_WAIT))
    {
        TaktOSExitCritical(state);

        return TAKTOS_ERR_BUSY;   // Bug fix: was TAKTOS_ERR_TIMEOUT
    }

    return TaktOSMutexLockSlowPath(pMtx, state, current, TimeoutTicks);
}

/**
 * @brief Release a mutex.
 *
 * Must be called by the owning thread. If waiters exist, ownership may be
 * handed to the highest-priority waiter in slow path.
 *
 * @param mtx Pointer to mutex.
 * @return TAKTOS_OK on success, TAKTOS_ERR_INVALID on null or non-owner call.
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

    if (TAKT_LIKELY(pMtx->WaitList.pHead == nullptr))
    {
    	pMtx->pOwner = nullptr;
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    return TaktOSMutexUnlockSlowPath(pMtx, state, current);
}

