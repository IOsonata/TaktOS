/**---------------------------------------------------------------------------
@file   TaktOSSignal.h

@brief  Experimental direct thread wake notification.

An ISR calls TaktOSThreadSignal(hThread) after publishing device work.
The thread waits with TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER),
then checks and drains that work. Device status stays in the driver.

One pending notification is latched when the thread is not waiting.
Repeated pending posts coalesce. A post that wakes a waiter is consumed
by that wait; another post before the thread runs remains pending for
its next wait. No event mask, event payload, counter, or separate object.

The pending byte occupies existing TCB padding. Signal waits use the
empty g_TaktSigWaitList marker for normal timeout / cancellation cleanup.
Wait must be called from a running thread with interrupts enabled.
Signal is ISR safe under the same interrupt rules as semaphore Give.

Safety boundary: IN  MC/DC coverage required.

@author Nguyen Hoan Hoang
@date   Oct. 2026

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
#ifndef __TAKTOSSIGNAL_H__
#define __TAKTOSSIGNAL_H__

#include "TaktOS.h"
#include "TaktKernel.h"
#include "TaktOSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

// Internal: entered with the critical section held; exits it before blocking.
TaktOSErr_t TaktOSThreadSignalWaitSlowPath(uint32_t TimeoutTicks, uint32_t IntState);

#if defined(TAKT_INLINE_OPTIMIZATION)
#  define TAKT_SIGNAL_FCT TAKT_ALWAYS_INLINE
#  define TAKT_SIGNAL_BODY
#elif defined(TAKT_SIGNAL_IMPL)
#  define TAKT_SIGNAL_FCT
#  define TAKT_SIGNAL_BODY
#endif

#if defined(TAKT_SIGNAL_BODY)

/**
 * Post one wake notification. ISR/thread safe; never blocks.
 * Returns TAKTOS_OK, or TAKTOS_ERR_INVALID for a null handle.
 * The target must be a live thread.
 */
TAKT_SIGNAL_FCT TaktOSErr_t TaktOSThreadSignal(hTaktOSThread_t hThread)
{
    if (hThread == NULL)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();

    if (hThread->pWaitList != &g_TaktSigWaitList)
    {
        hThread->SigPending = 1u;
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    // Deliver directly. Pending is already zero while a signal wait is armed.
    // Every cancellation path clears pWaitList, so an armed wait still has
    // the EVENT reason written by the waiter; no reason store is needed here.
    hThread->pWaitList = NULL;

    if (hThread->WakeTick != 0u)
    {
        TaktSleepListRemove(hThread);
        hThread->WakeTick = 0u;
    }

    hThread->State = TAKTOS_READY;
    TaktReadyTask(hThread);

    TaktOSThread_t *cur = g_TaktosCtx.pCurrent;
    bool needSwitch = (cur != NULL) && (g_TaktosCtx.pNextThread != cur);
    TaktOSExitCritical(state);

    if (needSwitch)
    {
        TaktOSCtxSwitch();
    }
    return TAKTOS_OK;
}

/**
 * Consume one pending wake notification, or wait for one.
 * No device event information is transferred.
 * Returns OK, EMPTY (nonblocking), TIMEOUT, INTERRUPTED, or INVALID (ISR).
 * Must run in thread context with interrupts enabled.
 */
TAKT_SIGNAL_FCT TaktOSErr_t TaktOSThreadSignalWait(bool bBlocking, uint32_t TimeoutTicks)
{
    if (TaktOSInIsr())
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();
    TaktOSThread_t *cur = TaktOSCurrentThread();

    if (cur->SigPending != 0u)
    {
        cur->SigPending = 0u;
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    if (!bBlocking || TimeoutTicks == TAKTOS_NO_WAIT)
    {
        TaktOSExitCritical(state);
        return TAKTOS_ERR_EMPTY;
    }

    return TaktOSThreadSignalWaitSlowPath(TimeoutTicks, state);
}

#else

TaktOSErr_t TaktOSThreadSignal(hTaktOSThread_t hThread);
TaktOSErr_t TaktOSThreadSignalWait(bool bBlocking, uint32_t TimeoutTicks);

#endif

#ifdef __cplusplus
}
#endif

#endif // __TAKTOSSIGNAL_H__
