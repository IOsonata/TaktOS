/**---------------------------------------------------------------------------
@file   TaktOSMutex.h

@brief  TaktOS binary mutex  C and C++

Two flavors of mutex are provided:

  1. PLAIN MUTEX  TaktOSMutexInit / TaktOSMutex::Init
     Non-recursive binary mutex.  No priority adjustment.  Cheapest fast path.
     Use when contention is rare and bounded priority inversion is acceptable
     (e.g. single-priority lockers, or ISR-protected data with no scheduler
     interaction).

  2. PRIORITY-CEILING MUTEX  TaktOSMutexInitProtect / TaktOSMutex::InitProtect
     Non-recursive binary mutex implementing the Immediate Priority Ceiling
     Protocol (IPCP, equivalent to POSIX PTHREAD_PRIO_PROTECT).

     On Lock(): the holder's effective priority is raised to
                  max(holder->Priority, mutex->Ceiling).
                The original priority is recorded in TCB.BasePriority on the
                first PCP acquire, and the ceiling is pushed onto the
                per-thread HeldCeilings stack (size TAKTOS_MAX_HELD_PCP_MUTEXES).

     On Unlock(): the ceiling is popped, and the holder's priority is
                  restored to max(BasePriority, max of remaining held ceilings).

     Properties:
        - Bounded priority inversion: at most one critical section.
        - Deadlock-free without lock-ordering rules, provided every locker's
          priority is <= the ceiling (strict precondition).
        - Lock fails with TAKTOS_ERR_INVALID if caller priority > ceiling.

  Recursive locking is NOT supported by either flavor  a thread that calls
  Lock() while already holding the mutex will block on itself.

  C usage:
    static TaktOSMutex_t gMtx;
    TaktOSMutexInit(&gMtx);                              // plain
    TaktOSMutexInitProtect(&gPcp, TAKTOS_PRIORITY_HIGH); // IPCP

  C++ usage:
    TaktOSMutex gMtx; gMtx.Init();
    TaktOSMutex gPcp; gPcp.InitProtect(TAKTOS_PRIORITY_HIGH);

Inlining gate (TAKT_INLINE_OPTIMIZATION):
  When defined, TaktOSMutexLock and TaktOSMutexUnlock are static inline in this
  header so the plain-mutex fast path folds directly into the call site,
  eliminating function-call overhead.  The PCP path and the contended/blocking
  path remain out-of-line in taktos_mutex.cpp behind TaktOSMutexLockSlow /
  TaktOSMutexUnlockSlow.  When undefined, the public functions are out-of-line
  in taktos_mutex.cpp (default, smaller code size).

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
#include "TaktOSThread.h"   // for TaktOSCurrentThread() used by inline fast paths

#pragma pack(push, 4)

typedef struct __TaktOSMutex_s {
    TaktOSThread_t *pOwner;         // nullptr = free, else = owning thread
    TaktKernelWaitList_t WaitList;
    uint8_t Ceiling;                // 0 = plain mutex, 1..31 = IPCP ceiling priority
    uint8_t _pad[3];                // alignment padding
} TaktOSMutex_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Initialize a plain (non-PCP) mutex.
 *
 * The resulting mutex performs no priority adjustment on Lock / Unlock.
 *
 * @param	pMtx : Pointer to caller-allocated mutex object.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p pMtx is null.
 */
TaktOSErr_t TaktOSMutexInit(TaktOSMutex_t *pMtx);

/**
 * @brief	Initialize a priority-ceiling (IPCP) mutex.
 *
 * The resulting mutex applies the Immediate Priority Ceiling Protocol:
 *   - On Lock(), the holder is boosted to max(holder->Priority, Ceiling).
 *   - On Unlock(), the holder's priority is restored.
 *   - Lock() returns TAKTOS_ERR_INVALID if caller priority > Ceiling.
 *
 * The caller must declare Ceiling >= the priority of every thread that
 * will ever lock this mutex.  Locking outside that set is a configuration
 * error and is detected by the strict ceiling check at Lock-time.
 *
 * @param	pMtx    : Pointer to caller-allocated mutex object.
 * @param	Ceiling : Priority ceiling (1..TAKTOS_MAX_PRI-1).
 *                    Ceiling 0 (idle) is rejected as TAKTOS_ERR_INVALID.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p pMtx is null
 *          or @p Ceiling is out of range.
 */
TaktOSErr_t TaktOSMutexInitProtect(TaktOSMutex_t *pMtx, uint8_t Ceiling);

/**
 * @brief	Slow path for mutex lock called when the mutex is already owned.
 *
 * Internal entry point.  Must not be called directly.  Called by
 * @ref TaktOSMutexLockSlow after it has decided that the caller must block.
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
 * Internal entry point.  Hands ownership to the highest-priority waiter,
 * applies the IPCP ceiling boost to the new owner if Ceiling != 0, and may
 * trigger a context switch.  Must not be called directly.  Called by
 * @ref TaktOSMutexUnlockSlow after the PCP unboost has been applied.
 *
 * @param	pMtx           : Pointer to mutex.
 * @param	savedPrimask   : Saved interrupt state from critical-section entry.
 * @param	current        : Pointer to the calling thread's TCB.
 * @return	TAKTOS_OK on success.
 */
TaktOSErr_t TaktOSMutexUnlockSlowPath(TaktOSMutex_t *pMtx, uint32_t savedPrimask,
                                      TaktOSThread_t *current);

/**
 * @brief  Wider slow entry for mutex Lock  handles PCP acquire and contended
 *         blocking, and is invoked from the inline plain-mutex fast path on
 *         miss.  Internal entry point  must not be called directly.
 *
 * Reaching this function means the inline plain-mutex fast path did NOT take
 * (either Ceiling != 0, or pOwner != NULL, or both).  The function decides
 * which sub-case applies and either:
 *   - performs the PCP acquire (push held ceiling, migrate priority), or
 *   - returns TAKTOS_ERR_BUSY for a non-blocking caller, or
 *   - delegates to @ref TaktOSMutexLockSlowPath to block.
 *
 * @param	pMtx          : Pointer to mutex.
 * @param	savedPrimask  : Saved interrupt state from critical-section entry.
 * @param	bBlocking     : true if the caller permits blocking.
 * @param	timeoutTicks  : Timeout in ticks (TAKTOS_WAIT_FOREVER permitted).
 * @return	Status code per TaktOSMutexLock spec.
 */
TaktOSErr_t TaktOSMutexLockSlow(TaktOSMutex_t *pMtx, uint32_t savedPrimask,
                                bool bBlocking, uint32_t timeoutTicks);

/**
 * @brief  Wider slow entry for mutex Unlock  handles PCP unboost and waiter
 *         handoff, invoked from the inline plain-mutex fast path on miss.
 *         Internal entry point  must not be called directly.
 *
 * Reaching this function means the inline plain-mutex fast path did NOT take
 * (either Ceiling != 0, or a waiter is present, or both).  The function:
 *   - applies the PCP unboost if Ceiling != 0, then
 *   - takes the no-waiter path (clearing pOwner) when WaitList is empty, or
 *   - delegates to @ref TaktOSMutexUnlockSlowPath to hand ownership to the
 *     highest-priority waiter.
 *
 * @param	pMtx          : Pointer to mutex.
 * @param	savedPrimask  : Saved interrupt state from critical-section entry.
 * @param	current       : Pointer to the calling thread's TCB.
 * @return	Status code per TaktOSMutexUnlock spec.
 */
TaktOSErr_t TaktOSMutexUnlockSlow(TaktOSMutex_t *pMtx, uint32_t savedPrimask,
                                  TaktOSThread_t *current);


#ifdef TAKT_INLINE_OPTIMIZATION

/**
 * @brief	Acquire a mutex  inline plain-mutex fast path.  See file header
 *          for the full spec.  Out-of-line cases delegate to
 *          @ref TaktOSMutexLockSlow.
 */
static TAKT_ALWAYS_INLINE TaktOSErr_t TaktOSMutexLock(TaktOSMutex_t *pMtx,
                                                     bool bBlocking,
                                                     uint32_t timeoutTicks)
{
    if (pMtx == NULL)
    {
        return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();

    // Plain-mutex fast path: pOwner == NULL  AND  Ceiling == 0.
    // Order is intentional  pOwner is the more variable field at run time;
    // testing it first lets the compiler short-circuit when contended without
    // also loading the (mostly-static) Ceiling byte.
    if (TAKT_LIKELY(pMtx->pOwner == NULL) && TAKT_LIKELY(pMtx->Ceiling == 0u))
    {
        pMtx->pOwner = TaktOSCurrentThread();
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    return TaktOSMutexLockSlow(pMtx, state, bBlocking, timeoutTicks);
}

/**
 * @brief	Release a mutex  inline plain-mutex fast path.  See file header
 *          for the full spec.  Out-of-line cases delegate to
 *          @ref TaktOSMutexUnlockSlow.
 */
static TAKT_ALWAYS_INLINE TaktOSErr_t TaktOSMutexUnlock(TaktOSMutex_t *pMtx)
{
    if (pMtx == NULL)
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

    // Plain-mutex fast path: Ceiling == 0  AND  no waiter.
    if (TAKT_LIKELY(pMtx->Ceiling == 0u) &&
        TAKT_LIKELY(pMtx->WaitList.pHead == NULL))
    {
        pMtx->pOwner = NULL;
        TaktOSExitCritical(state);
        return TAKTOS_OK;
    }

    return TaktOSMutexUnlockSlow(pMtx, state, current);
}

#else // TAKT_INLINE_OPTIMIZATION

/**
 * @brief	Acquire a mutex  out-of-line variant.
 *
 * Active when TAKT_INLINE_OPTIMIZATION is not defined.  See file header
 * for the full spec.  Identical semantics to the inline form.
 *
 * @param	pMtx          : Pointer to mutex.
 * @param	bBlocking     : true to block when owned by another thread.
 * @param	timeoutTicks  : Wait timeout in ticks (TAKTOS_WAIT_FOREVER to block indefinitely).
 * @return	TAKTOS_OK on success, otherwise TAKTOS_ERR_BUSY / TAKTOS_ERR_TIMEOUT /
 *          TAKTOS_ERR_INTERRUPTED / TAKTOS_ERR_INVALID.
 */
TaktOSErr_t TaktOSMutexLock(TaktOSMutex_t *pMtx, bool bBlocking, uint32_t timeoutTicks);

/**
 * @brief	Release a mutex  out-of-line variant.
 *
 * Active when TAKT_INLINE_OPTIMIZATION is not defined.  See file header
 * for the full spec.  Identical semantics to the inline form.
 *
 * @param	pMtx : Pointer to mutex.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p pMtx is null or
 *          the caller is not the owner.
 */
TaktOSErr_t TaktOSMutexUnlock(TaktOSMutex_t *pMtx);

#endif // TAKT_INLINE_OPTIMIZATION


#ifdef __cplusplus
}

class TaktOSMutex {
public:
    TaktOSErr_t Init() { return TaktOSMutexInit(&vMtx); }
    TaktOSErr_t InitProtect(uint8_t Ceiling) { return TaktOSMutexInitProtect(&vMtx, Ceiling); }
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
