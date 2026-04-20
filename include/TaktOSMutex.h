/**---------------------------------------------------------------------------
@file   TaktOSMutex.h

@brief  TaktOS binary mutex with priority inheritance  C and C++

Priority inheritance (PI) is deferred to v1.1. In v1.0, priority inversion is bounded only
by the scheduler's FIFO ordering within a priority level. If your application requires PI,
define TAKT_MUTEX_PI=1 at compile time  the build will fail with an explicit error message
until the implementation is ready, preventing silent reliance on a missing feature.

Recursive locking is NOT supported  a task that calls Lock() while already
holding the mutex will deadlock.

  C:
    static TaktOSMutex_t gMtx;
    TaktOSMutexInit(&gMtx);
    TaktOSMutexLock(&gMtx, true, TAKTOS_WAIT_FOREVER);
    TaktOSMutexUnlock(&gMtx);

  C++:
    static TaktOSMutex gMtx;
    gMtx.Init();
    gMtx.Lock(true, TAKTOS_WAIT_FOREVER);
    gMtx.Unlock();

Acquire hot path is inlined; slow paths (contention, PI, timeout) are in
taktos_mutex.cpp.

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
#ifndef __TAKTOSMUTEX_H__
#define __TAKTOSMUTEX_H__


#include "TaktOS.h"
#include "TaktKernel.h"

#pragma pack(push, 4)

// PI guard: TAKT_MUTEX_PI=1 signals that the caller expects priority-inheritance
// boosting on contended Lock().  That feature is NOT YET IMPLEMENTED (v1.0).
// Fail the build explicitly so no integration silently runs without PI.
#if defined(TAKT_MUTEX_PI) && (TAKT_MUTEX_PI != 0)
#  error "TAKT_MUTEX_PI=1: priority inheritance is deferred to v1.1  not yet implemented."
#endif

typedef struct __TaktOSMutex_s {
    TaktOSThread_t *pOwner;         // nullptr = free, else = owning thread
    TaktKernelWaitList_t WaitList;
    uint8_t OwnerBasePri;  			// reserved for v1.1 priority inheritance
    uint8_t _pad[3];       			// alignment padding
} TaktOSMutex_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Initialize a mutex object.
 *
 * @param	pMtx : Pointer to caller-allocated mutex object.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p pMtx is null.
 */
TaktOSErr_t TaktOSMutexInit(TaktOSMutex_t *pMtx);

/**
 * @brief	Slow path for mutex lock called when the mutex is already owned.
 *
 * Called by @ref TaktOSMutexLock after entering a critical section when the
 * mutex is found to be held by another thread.  Must not be called directly.
 *
 * @param	pMtx           : Pointer to mutex.
 * @param	savedPrimask   : Saved interrupt state from critical-section entry.
 * @param	current        : Pointer to the calling thread's TCB.
 * @param	timeoutTicks   : Timeout in ticks, or TAKTOS_WAIT_FOREVER.
 * @return	TAKTOS_OK on acquisition, TAKTOS_ERR_BUSY, TAKTOS_ERR_TIMEOUT, or
 *          TAKTOS_ERR_INTERRUPTED on failure.
 */
TaktOSErr_t TaktOSMutexLockSlowPath(TaktOSMutex_t *pMtx, uint32_t savedPrimask,
                                    TaktOSThread_t *current,
                                    uint32_t timeoutTicks);

/**
 * @brief	Slow path for mutex unlock called when waiters are present.
 *
 * Called by @ref TaktOSMutexUnlock after entering a critical section when the
 * wait list is non-empty.  Hands ownership to the highest-priority waiter and
 * may trigger a context switch.  Must not be called directly.
 *
 * @param	pMtx           : Pointer to mutex.
 * @param	savedPrimask   : Saved interrupt state from critical-section entry.
 * @param	current        : Pointer to the calling thread's TCB.
 * @return	TAKTOS_OK on success.
 */
TaktOSErr_t TaktOSMutexUnlockSlowPath(TaktOSMutex_t *pMtx, uint32_t savedPrimask,
                                      TaktOSThread_t *current);


/**
 * @brief	Acquire a mutex.
 *
 * Fast path acquires immediately when the mutex is free.  If already owned:
 *   - non-blocking (@p bBlocking == false or @p timeoutTicks == TAKTOS_NO_WAIT)
 *     returns TAKTOS_ERR_BUSY immediately.
 *   - blocking delegates to the slow path and blocks up to @p timeoutTicks.
 *
 * @param	pMtx          : Pointer to mutex.
 * @param	bBlocking     : true to block when owned by another thread.
 * @param	timeoutTicks  : Wait timeout in ticks (TAKTOS_WAIT_FOREVER to block indefinitely).
 * @return	TAKTOS_OK on success, otherwise TAKTOS_ERR_BUSY / TAKTOS_ERR_TIMEOUT /
 *          TAKTOS_ERR_INTERRUPTED / TAKTOS_ERR_INVALID.
 */
TaktOSErr_t TaktOSMutexLock(TaktOSMutex_t *pMtx, bool bBlocking, uint32_t timeoutTicks);

/**
 * @brief	Release a mutex.
 *
 * Must be called by the owning thread only.  If waiters exist in the wait list,
 * ownership is handed to the highest-priority waiter inside the slow path.
 *
 * @param	pMtx : Pointer to mutex.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p pMtx is null or
 *          the caller is not the owner.
 */
TaktOSErr_t TaktOSMutexUnlock(TaktOSMutex_t *pMtx);


#ifdef __cplusplus
}

class TaktOSMutex {
public:
    TaktOSErr_t Init() { return TaktOSMutexInit(&vMtx); }
    TaktOSErr_t Lock(bool bBlocking = true,
                     uint32_t TimeoutTicks = TAKTOS_WAIT_FOREVER) {
        return TaktOSMutexLock(&vMtx, bBlocking, TimeoutTicks);
    }
    TaktOSErr_t Unlock() { return TaktOSMutexUnlock(&vMtx); }
    TaktOSMutex_t *Handle() { return &vMtx; }
private:
    TaktOSMutex_t vMtx;
};

#endif // __cplusplus

#endif // __TAKTOSMUTEX_H__

