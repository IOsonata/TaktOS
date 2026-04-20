/**---------------------------------------------------------------------------
@file   taktos_sem.cpp

@brief  TaktOS counting semaphore  slow paths and init

Fast paths (Give / Take, no contention) are inlined in TaktOSSem.h.
Cold slow paths (blocking, timeout, priority-ordered wake) live here.

Timeout semantics:
  TAKTOS_NO_WAIT         handled in the inline fast path (returns ERR_EMPTY).
  TAKTOS_WAIT_FOREVER    blocks indefinitely; no sleep-list insertion.
  finite ticks           blocks for up to N ticks, then returns ERR_TIMEOUT.
                          Implemented via dual-list insertion: the thread is
                          placed in both the semaphore wait list and the
                          global sleep list.  Whichever fires first wins:
                            * Give path: removes from sleep list, returns OK.
                            * Tick ISR:  removes from sem wait list, sets
                              wakeReason = TIMEOUT, returns ERR_TIMEOUT.

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
#include "TaktOSSem.h"
#include "TaktOSThread.h"

//--- Init -------------------------------------------------------

/**
 * @brief	Initialize a semaphore  implementation.  @see TaktOSSem.h.
 */
TaktOSErr_t TaktOSSemInit(TaktOSSem_t *pSem, uint32_t Initial, uint32_t MaxCount)
{
    if (pSem == nullptr || MaxCount == 0u || Initial > MaxCount)
    {
    	return TAKTOS_ERR_INVALID;
    }

    pSem->Count    = Initial;
    pSem->MaxCount = MaxCount;
    pSem->WaitList.pHead = nullptr;

    return TAKTOS_OK;
}

//--- Give slow path ---------------------------------------------
// Entered when waitHead != nullptr (wake a waiter) or count==maxCount (full).

/**
 * @brief	Slow path for TaktOSSemGive  implementation.  @see TaktOSSem.h.
 *
 * Pops the highest-priority waiter, stamps TAKT_WOKEN_BY_EVENT, makes it
 * ready, and preempts if it has higher priority than the current thread.
 * The critical section (Primask) is exited before the context switch.
 */
TAKT_COLD TAKT_NOINLINE TaktOSErr_t TaktOSSemGiveSlowPath(TaktOSSem_t *pSem, uint32_t Primask)
{
    TaktOSThread_t *waiter = pSem->WaitList.pHead;

    if (waiter != nullptr)
    {
        pSem->WaitList.pHead = waiter->pWaitNext;
        waiter->pWaitNext = nullptr;

        // pWaitList is always set for kernel-object waits.
        // Use WakeTick to distinguish timed waits (in sleep list) from
        // WAIT_FOREVER waits (not in sleep list).
        waiter->pWaitList = nullptr;   // clear before making READY

        if (waiter->WakeTick != 0u)
        {
            TaktSleepListRemove(waiter);
            waiter->WakeTick = 0u;
        }
        waiter->WakeReason = TAKT_WOKEN_BY_EVENT;
        waiter->State      = TAKTOS_READY;
        TaktReadyTask(waiter);

        TaktOSThread_t *cur = TaktOSCurrentThread();
        const bool needSwitch = (cur == nullptr) || (waiter->Priority > cur->Priority) ||
        						(cur->State != TAKTOS_READY && cur->State != TAKTOS_RUNNING);

        TaktOSExitCritical(Primask);
        if (needSwitch)
        {
            TaktOSCtxSwitch();
        }
        {
        	return TAKTOS_OK;
        }
    }

    TaktOSExitCritical(Primask);

    return TAKTOS_ERR_FULL;
}

//--- Take slow path (Bug 1 fix) ---------------------------------
// Entered when count==0 and blocking==true.

/**
 * @brief	Slow path for TaktOSSemTake  implementation.  @see TaktOSSem.h.
 *
 * Inserts the calling thread into the wait list in priority order, optionally
 * arms the sleep list for a timed wait, then blocks the caller.  Returns
 * TAKTOS_ERR_TIMEOUT when the deadline expires without a Give.
 */
TAKT_COLD TAKT_NOINLINE TaktOSErr_t TaktOSSemTakeSlowPath(TaktOSSem_t *pSem, uint32_t Primask,
                                                          uint32_t timeoutTicks)
{
    TaktOSThread_t *current = TaktOSCurrentThread();

    current->State      = TAKTOS_BLOCKED;
    current->WakeReason = TAKT_WOKEN_BY_EVENT;
    current->pWaitNext  = nullptr;

    TaktBlockTask(current);
    TaktWaitListInsert(&pSem->WaitList, current);

    // Always store pWaitList so Resume/HandOff can cancel this wait.
    // WakeTick == 0 distinguishes WAIT_FOREVER (not in sleep list) from
    // a timed wait (in sleep list).  pWaitList must never be null for
    // a thread blocked inside a kernel-object wait.
    current->pWaitList = &pSem->WaitList;

    if (timeoutTicks != TAKTOS_WAIT_FOREVER)
    {
        current->WakeTick = TaktOSTickCount() + timeoutTicks;
        TaktSleepListAdd(current);
    }
    // WAIT_FOREVER: WakeTick is always 0 on entry  cleared by all three
    // waker paths (GiveSlowPath, TaktThreadWakeTick, Resume/HandOff).

    TaktOSExitCritical(Primask);
    TaktOSCtxSwitch();

    // After wakeup: check how we were woken.
    // WakeTick: already cleared by whoever woke us
    //   GiveSlowPath:       waiter->WakeTick = 0u  (line ~54)
    //   TaktThreadWakeTick: t->WakeTick      = 0u  (tick expiry)
    //   Resume/HandOff:     t->WakeTick      = 0u  (explicit cancel)
    // WakeReason: reset to TAKT_WOKEN_BY_EVENT at ENTRY on the next call.
    // Therefore neither field needs clearing here.
    // CBNZ on 'wr' (already loaded, == 0 for normal wakeup) compiles
    // to two instructions on the hot path: LDRB + CBNZ, ~2 cycles.
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

#ifndef TAKT_INLINE_OPTIMIZATION

// Give  add one token. blocking param reserved for v1.1; pass false.
// ISR-safe: uses critical section internally.
/**
 * @brief	Give a semaphore  non-inline fallback.  @see TaktOSSem.h.
 */
TaktOSErr_t TaktOSSemGive(TaktOSSem_t *pSem, bool bBlocking)
{
    (void)bBlocking;

    if (pSem == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();

    if (TAKT_LIKELY(pSem->WaitList.pHead == nullptr))
    {
        const uint32_t count = pSem->Count;

        if (TAKT_LIKELY(pSem->MaxCount == 1u))
        {
            if (TAKT_LIKELY(count == 0u))
            {
            	pSem->Count = 1u;
                TaktOSExitCritical(state);

                return TAKTOS_OK;
            }
        }
        else if (TAKT_LIKELY(count < pSem->MaxCount))
        {
        	pSem->Count = count + 1u;
            TaktOSExitCritical(state);

            return TAKTOS_OK;
        }
        // count == maxCount, no waiters  full, nothing to wake.
        // Handle inline: avoids a COLD NOINLINE call just to return ERR_FULL.
        TaktOSExitCritical(state);

        return TAKTOS_ERR_FULL;
    }

    // waitList.head != nullptr  waiter present; delegate to slow path.
    return TaktOSSemGiveSlowPath(pSem, state);
}

// Take  consume one token.
// blocking=false / timeoutTicks=TAKTOS_NO_WAIT: return immediately if empty.
// blocking=true  / timeoutTicks=TAKTOS_WAIT_FOREVER: block indefinitely.
/**
 * @brief	Take a semaphore  non-inline fallback.  @see TaktOSSem.h.
 */
TaktOSErr_t TaktOSSemTake(TaktOSSem_t *pSem, bool bBlocking, uint32_t TimeoutTicks)
{
    if (pSem == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();
    const uint32_t count = pSem->Count;

    if (TAKT_LIKELY(count > 0u))
    {
    	pSem->Count = count - 1u;
        TaktOSExitCritical(state);

        return TAKTOS_OK;
    }

    if (TAKT_UNLIKELY(!bBlocking || TimeoutTicks == TAKTOS_NO_WAIT))
    {
        TaktOSExitCritical(state);

        return TAKTOS_ERR_EMPTY;
    }

    return TaktOSSemTakeSlowPath(pSem, state, TimeoutTicks);
}
#endif


