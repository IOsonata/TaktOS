/**---------------------------------------------------------------------------
@file   TaktOSSem.h

@brief  TaktOS counting semaphore  C and C++

Binary semaphore:   TaktOSSemInit(&sem, 0, 1)    starts empty
Counting semaphore: TaktOSSemInit(&sem, n, max)  starts with n tokens

User declares TaktOSSem_t directly (no hidden allocation).  The struct is
self-contained (12 bytes) and statically allocated.

  C:
    static TaktOSSem_t gSem;
    TaktOSSemInit(&gSem, 0u, 1u);
    TaktOSSemGive(&gSem, false);          // ISR-safe (blocking=false)
    TaktOSSemTake(&gSem, true, 100u);     // block up to 100 ticks

  C++:
    static TaktOSSem gSem;
    gSem.Init(0u, 1u);
    gSem.Give(false);
    gSem.Take(true, 100u);

GiveFromISR hot path is inlined; slow paths (contention, timeout) are in
taktos_sem.cpp.

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
#ifndef __TAKTOSSEM_H__
#define __TAKTOSSEM_H__

#include <memory.h>

#include "TaktOS.h"
#include "TaktKernel.h"

#pragma pack(push, 4)

typedef struct __TaktOSSem_s {
    uint32_t Count;         // current token count
    uint32_t MaxCount;      // ceiling (1 = binary)
    TaktKernelWaitList_t WaitList;
} TaktOSSem_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Initialize a semaphore object.
 *
 * Binary semaphore:   TaktOSSemInit(&sem, 0u, 1u)    starts empty.
 * Counting semaphore: TaktOSSemInit(&sem, n,  max)   starts with n tokens.
 *
 * @param	pSem     : Pointer to caller-allocated semaphore object.
 * @param	Initial  : Initial token count (must be  MaxCount).
 * @param	MaxCount : Maximum token count ceiling (1 = binary semaphore).
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p pSem is null or
 *          Initial > MaxCount.
 */
TaktOSErr_t TaktOSSemInit(TaktOSSem_t *pSem, uint32_t Initial, uint32_t MaxCount);

/**
 * @brief	Slow path for TaktOSSemGive  called when a waiter is present.
 *
 * Must not be called directly.  Called by @ref TaktOSSemGive after the fast
 * path detects a non-empty wait list.  The critical section is already held
 * on entry; this function takes ownership and exits it before returning.
 *
 * @param	pSem     : Pointer to semaphore.
 * @param	IntState : Saved interrupt state from critical-section entry.
 * @return	TAKTOS_OK on success.
 */
TaktOSErr_t TaktOSSemGiveSlowPath(TaktOSSem_t *pSem, uint32_t IntState);

/**
 * @brief	Slow path for TaktOSSemTake  called when no token is available.
 *
 * Must not be called directly.  Called by @ref TaktOSSemTake after the fast
 * path detects an empty semaphore and blocking is permitted.  The critical
 * section is already held on entry; this function takes ownership and exits
 * it before blocking the caller.
 *
 * @param	pSem          : Pointer to semaphore.
 * @param	IntState      : Saved interrupt state from critical-section entry.
 * @param	TimeoutTicks  : Maximum ticks to wait, or TAKTOS_WAIT_FOREVER.
 * @return	TAKTOS_OK on token acquisition, TAKTOS_ERR_TIMEOUT if the deadline
 *          expired, or TAKTOS_ERR_INTERRUPTED if woken by an external Resume.
 */
TaktOSErr_t TaktOSSemTakeSlowPath(TaktOSSem_t *pSem, uint32_t IntState, uint32_t TimeoutTicks);

#ifdef TAKT_INLINE_OPTIMIZATION

/**
 * @brief	Give (post) a semaphore  add one token.
 *
 * ISR-safe.  If one or more threads are blocked waiting on this semaphore the
 * highest-priority waiter is woken via the slow path; otherwise the token count
 * is incremented in the fast path with no context switch.
 *
 * @param	pSem      : Pointer to semaphore.
 * @param	bBlocking : Reserved for v1.1  pass false.  Has no effect in v1.0.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_FULL if the count is already at
 *          MaxCount, TAKTOS_ERR_INVALID if @p pSem is null.
 */
// Give  add one token. blocking param reserved for v1.1; pass false.
// ISR-safe: uses critical section internally.
static TAKT_ALWAYS_INLINE TaktOSErr_t TaktOSSemGive(TaktOSSem_t *pSem, bool bBlocking)
{
    (void)bBlocking;

    // Null check unconditional  null handles cause UB in both debug and release.
    if (pSem == NULL)
    {
    	return TAKTOS_ERR_INVALID;
    }

    uint32_t state = TaktOSEnterCritical();

    if (TAKT_LIKELY(pSem->WaitList.pHead == NULL))
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
 * @brief	Take (wait on) a semaphore  consume one token.
 *
 * If a token is available it is consumed immediately and the function returns
 * TAKTOS_OK without blocking.  If the semaphore is empty:
 *   - @p bBlocking == false or @p TimeoutTicks == TAKTOS_NO_WAIT returns
 *     TAKTOS_ERR_EMPTY immediately.
 *   - Otherwise the caller blocks for up to @p TimeoutTicks ticks.
 *
 * @param	pSem          : Pointer to semaphore.
 * @param	bBlocking     : true to block when empty; false for a non-blocking poll.
 * @param	TimeoutTicks  : Maximum ticks to block (TAKTOS_WAIT_FOREVER to block
 *                         indefinitely; TAKTOS_NO_WAIT for immediate return).
 * @return	TAKTOS_OK on token acquisition, TAKTOS_ERR_EMPTY if non-blocking and
 *          empty, TAKTOS_ERR_TIMEOUT on deadline expiry, TAKTOS_ERR_INTERRUPTED
 *          if woken by an external Resume, or TAKTOS_ERR_INVALID on null pointer.
 */
static TAKT_ALWAYS_INLINE TaktOSErr_t TaktOSSemTake(TaktOSSem_t *pSem, bool bBlocking, uint32_t TimeoutTicks)
{
    // Null check unconditional  null handles cause UB in both debug and release.
    if (pSem == NULL)
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
#else
/**
 * @brief	Give (post) a semaphore  add one token.
 *
 * Non-inline variant active when TAKT_INLINE_OPTIMIZATION is not defined.
 * See the inline version above for full semantics.
 *
 * @param	pSem      : Pointer to semaphore.
 * @param	bBlocking : Reserved for v1.1  pass false.
 * @return	TAKTOS_OK, TAKTOS_ERR_FULL, or TAKTOS_ERR_INVALID.
 */
TaktOSErr_t TaktOSSemGive(TaktOSSem_t *pSem, bool bBlocking);

/**
 * @brief	Take (wait on) a semaphore  consume one token.
 *
 * Non-inline variant active when TAKT_INLINE_OPTIMIZATION is not defined.
 * See the inline version above for full semantics.
 *
 * @param	pSem          : Pointer to semaphore.
 * @param	bBlocking     : true to block when empty.
 * @param	TimeoutTicks  : Maximum ticks to block (TAKTOS_WAIT_FOREVER / TAKTOS_NO_WAIT).
 * @return	TAKTOS_OK, TAKTOS_ERR_EMPTY, TAKTOS_ERR_TIMEOUT, TAKTOS_ERR_INTERRUPTED,
 *          or TAKTOS_ERR_INVALID.
 */
TaktOSErr_t TaktOSSemTake(TaktOSSem_t *pSem, bool bBlocking, uint32_t TimeoutTicks);

#endif

#ifdef __cplusplus
}

class TaktOSSem {
public:
    TaktOSErr_t Init(uint32_t Initial, uint32_t MaxCount) {
        return TaktOSSemInit(&vSem, Initial, MaxCount);
    }
    TaktOSErr_t Give(bool bBlocking = false) {
        return TaktOSSemGive(&vSem, bBlocking);
    }
    TaktOSErr_t Take(bool bBlocking = true, uint32_t TimeoutTicks = TAKTOS_WAIT_FOREVER) {
        return TaktOSSemTake(&vSem, bBlocking, TimeoutTicks);
    }
    TaktOSSem_t *Handle() { return &vSem; }

private:
    TaktOSSem_t vSem;
};

#endif // __cplusplus

#endif // __TAKTOSSEM_H__

