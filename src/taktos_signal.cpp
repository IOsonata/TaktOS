/**---------------------------------------------------------------------------
@file   taktos_signal.cpp

@brief  Experimental direct thread wake notification: blocking path.

An armed wait has SigPending == 0, pWaitList == &g_TaktSigWaitList and
WakeReason == EVENT. A signal clears the marker and makes the thread ready.
Timeout and administrative cancellation clear the marker and set WakeReason
through the generic kernel-object cancellation paths.

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
#if !defined(TAKT_INLINE_OPTIMIZATION)
#define TAKT_SIGNAL_IMPL
#endif

#include "TaktOSSignal.h"

// Marker only: no thread is ever inserted into this list.
TaktKernelWaitList_t g_TaktSigWaitList = { nullptr };

TAKT_NOINLINE TaktOSErr_t TaktOSThreadSignalWaitSlowPath(uint32_t TimeoutTicks,
                                                       uint32_t IntState)
{
    TaktOSThread_t *cur = TaktOSCurrentThread();
    cur->State = TAKTOS_BLOCKED;
    cur->WakeReason = TAKT_WOKEN_BY_EVENT;

    if (!TaktBlockTask(cur))
    {
        cur->State = TAKTOS_RUNNING;
        TaktOSExitCritical(IntState);
        return TAKTOS_ERR_INVALID;
    }

    cur->pWaitList = &g_TaktSigWaitList;

    if (TimeoutTicks != TAKTOS_WAIT_FOREVER)
    {
        cur->WakeTick = TaktKernelDeadline(TimeoutTicks);
        TaktSleepListAdd(cur);
    }

    TaktOSExitCritical(IntState);
    TaktOSCtxSwitchBlock();

    const uint8_t wr = cur->WakeReason;
    if (TAKT_LIKELY(wr == TAKT_WOKEN_BY_EVENT))
    {
        return TAKTOS_OK;
    }
    return (wr == TAKT_WOKEN_BY_TIMEOUT) ? TAKTOS_ERR_TIMEOUT : TAKTOS_ERR_INTERRUPTED;
}
