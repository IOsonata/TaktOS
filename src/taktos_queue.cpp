/**---------------------------------------------------------------------------
@file   taktos_queue.cpp

@brief  TaktOS message queue  slow paths, init, and count helpers

Fast paths (Send / Receive, space / message available) are inlined in
TaktOSQueue.h.  Cold slow paths (blocking, timeout) live here.

Timeout semantics:
  TAKTOS_NO_WAIT         handled in the inline fast path (returns ERR_FULL
                          or ERR_EMPTY immediately).
  TAKTOS_WAIT_FOREVER    blocks indefinitely; no sleep-list insertion.
  finite ticks           blocks up to N ticks, then returns ERR_TIMEOUT.
                          Same dual-list pattern as semaphore and mutex:
                          thread inserted in both the queue wait list and
                          g_sleepList.  Whichever fires first wins:
                            * Wake path: removes from sleep list, returns OK.
                            * Tick ISR:  removes from queue list, sets
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
#include <stdint.h>

#include "TaktOSQueue.h"
#include "TaktKernel.h"
#include "TaktCompiler.h"

/**
 * @brief	Initialize a message queue  implementation.  @see TaktOSQueue.h.
 */
TaktOSErr_t TaktOSQueueInit(TaktOSQueue_t *pQue, void *pStorage,
                             uint32_t ItemSize, uint32_t Capacity)
{
    if (pQue == nullptr || pStorage == nullptr)
    {
    	return TAKTOS_ERR_INVALID;
    }

    if (ItemSize == 0u || Capacity == 0u)
    {
    	return TAKTOS_ERR_INVALID;
    }

    if (ItemSize & 3u)
    {
    	TAKT_TRAP();
    }

    if (reinterpret_cast<uintptr_t>(pStorage) & 3u)
    {
    	TAKT_TRAP();
    }

    uint8_t* buf     = static_cast<uint8_t*>(pStorage);

    pQue->Avail     = Capacity;
    pQue->pWrite      = buf;
    pQue->pRead       = buf;
    pQue->Capacity      = Capacity;
    pQue->pBuffer        = buf;
    pQue->pBufEnd        = buf + Capacity * ItemSize;
    pQue->ItemSize      = ItemSize;
    pQue->SendList.pHead = nullptr;
    pQue->RecvList.pHead = nullptr;

    return TAKTOS_OK;
}

#ifndef TAKT_INLINE_OPTIMIZATION

/**
 * @brief	Word-aligned fast memory copy for fixed small sizes.
 *
 * Uses struct-assignment to let the compiler emit LDM/STM for sizes 4, 8,
 * 12, 16, and 32 bytes (~8 cycles regardless of word count).  Sizes that do
 * not match a known case fall back to TAKT_MEMCPY.
 *
 * @param	pDst  : Destination pointer (must be 4-byte aligned).
 * @param	pSrc  : Source pointer (must be 4-byte aligned).
 * @param	Size  : Number of bytes to copy (must be a multiple of 4).
 */
static TAKT_ALWAYS_INLINE void TaktQueueFastCopy(void *pDst, const void *pSrc, uint32_t Size) {
    if (TAKT_LIKELY(Size == 16u))
    {
        typedef struct { uint32_t w[4]; } b16;
        *(b16*)pDst = *(const b16*)pSrc;
    }
    else if (Size == 32u)
    {
        typedef struct { uint32_t w[8]; } b32;
        *(b32*)pDst = *(const b32*)pSrc;
    }
    else if (Size == 8u)
    {
        typedef struct { uint32_t w[2]; } b8;
        *(b8*)pDst = *(const b8*)pSrc;
    }
    else if (Size == 4u)
    {
        *(uint32_t*)pDst = *(const uint32_t*)pSrc;
    }
    else if (Size == 12u)
    {
        typedef struct { uint32_t w[3]; } b12;
        *(b12*)pDst = *(const b12*)pSrc;
    }
    else
    {
        TAKT_MEMCPY(pDst, pSrc, Size);
    }
}


/**
 * @brief	Advance the write pointer and claim one ring-buffer slot.
 *
 * Must be called inside a critical section.  Returns a pointer to the slot
 * where the caller must copy the item data.  Returns NULL if the queue is full
 * (Avail == 0)  the caller is responsible for the blocking slow path.
 *
 * @param	pQue : Pointer to queue.
 * @return	Pointer to the claimed write slot, or NULL if the queue is full.
 */
static TAKT_ALWAYS_INLINE uint8_t *TaktQueueRingPut(TaktOSQueue_t *pQue) {
    if (!pQue->Avail)
    {
    	return NULL;
    }

    uint8_t* slot = pQue->pWrite;
    uint8_t* next = slot + pQue->ItemSize;

    if (next >= pQue->pBufEnd)
    {
    	next = pQue->pBuffer;
    }
    pQue->pWrite = next;
    pQue->Avail--;

    return slot;
}

/**
 * @brief	Advance the read pointer and return the next ring-buffer slot.
 *
 * Must be called inside a critical section.  Returns a pointer to the slot
 * containing the next item to be consumed.  Returns NULL if the queue is empty
 * (Avail == Capacity).
 *
 * @param	pQue : Pointer to queue.
 * @return	Pointer to the next read slot, or NULL if the queue is empty.
 */
static TAKT_ALWAYS_INLINE uint8_t *TaktQueueRingGet(TaktOSQueue_t *pQue) {
    if (pQue->Avail == pQue->Capacity)
    {
    	return NULL;
    }

    uint8_t* slot = pQue->pRead;
    uint8_t* next = slot + pQue->ItemSize;

    if (next >= pQue->pBufEnd)
    {
    	next = pQue->pBuffer;
    }
    pQue->pRead = next;
    pQue->Avail++;

    return slot;
}

#endif

//--- Send slow path ---------------------------------------------------------
// Entered when the queue is full and blocking == true.
/**
 * @brief	Slow path for TaktOSQueueSend  implementation.  @see TaktOSQueue.h.
 *
 * Stores pData in the calling thread's pMsg field, inserts it into the send
 * wait list, and blocks until a receiver performs a direct handoff or the
 * deadline expires.
 */
TaktOSErr_t TaktOSQueueSendSlowPath(TaktOSQueue_t *pQue, const void *pData,
                                     uint32_t TimeoutTicks)
{
    uint32_t saved = TaktOSEnterCritical();

    // Re-check under lock  a receiver may have freed a slot since the fast
    // path gave up.  Two exclusive cases:
    //
    //   A) A blocked receiver is already waiting.  Hand the item directly to
    //      that receiver WITHOUT enqueuing it first.  Enqueuing then handing
    //      off would duplicate the item: the ring buffer copy would be
    //      delivered once now (to the waiter) and again when the next receiver
    //      drains the ring.
    //
    //   B) A free slot exists but no blocked receiver.  Enqueue normally.
    //
    // The two cases are mutually exclusive by invariant: receivers only block
    // when the queue is empty (available == capacity), so recvList.pHead != NULL
    // implies available == capacity, which means TaktQueueRingPut would return
    // NULL.  Checking recvList first avoids a redundant ring probe in case A.

    if (pQue->RecvList.pHead != nullptr)
    {
        // Case A: direct handoff  do NOT touch the ring buffer.
        bool needSwitch = TaktQueueHandoffToReceiver(pQue, pData);
        TaktOSExitCritical(saved);

        if (needSwitch)
        {
            TaktOSCtxSwitch();
        }

        return TAKTOS_OK;
    }

    uint8_t* slot = TaktQueueRingPut(pQue);

    if (slot != nullptr)
    {
        // Case B: free slot, no blocked receiver  enqueue normally.
        TaktQueueFastCopy(slot, pData, pQue->ItemSize);
        TaktOSExitCritical(saved);

        return TAKTOS_OK;
    }

    // Still full  block.  Store data pointer for direct handoff:
    // TaktQueueHandoffFromSender will copy our item into the ring buffer
    // at the moment a receiver frees a slot, eliminating the re-compete race.
    TaktOSThread_t* cur = TaktOSCurrentThread();
    cur->State      = TAKTOS_BLOCKED;
    cur->WakeReason = TAKT_WOKEN_BY_EVENT;
    cur->WakeTick   = 0u;
    cur->pWaitNext  = nullptr;
    cur->pMsg       = (void*)pData;   // receiver will copy from here

    TaktBlockTask(cur);
    TaktWaitListInsert(&pQue->SendList, cur);

    cur->pWaitList = &pQue->SendList;

    if (TimeoutTicks != TAKTOS_WAIT_FOREVER)
    {
        cur->WakeTick = TaktOSTickCount() + TimeoutTicks;
        TaktSleepListAdd(cur);
    }
    else
    {
        cur->WakeTick = 0u;
    }

    TaktOSExitCritical(saved);
    TaktOSCtxSwitch();

    // After wakeup: item was placed in ring buffer by TaktQueueHandoffFromSender.
    // No re-compete needed  either it succeeded (EVENT) or timed out.
    cur->pMsg     = nullptr;
    cur->WakeTick = 0u;
    const uint8_t wr_s = cur->WakeReason;

    if (__builtin_expect((int)wr_s, 0))
    {
        if (wr_s == TAKT_WOKEN_BY_TIMEOUT)
        {
        	return TAKTOS_ERR_TIMEOUT;
        }

        return TAKTOS_ERR_INTERRUPTED;  // TAKT_WOKEN_BY_RESUME  item NOT placed
    }

    return TAKTOS_OK;
}

//--- Receive slow path (Bug 7 fix) --------------------------------------
// Entered when the queue is empty and blocking == true.
/**
 * @brief	Slow path for TaktOSQueueReceive  implementation.  @see TaktOSQueue.h.
 *
 * Inserts the calling thread into the receive wait list and blocks until a
 * sender performs a direct handoff or the deadline expires.
 */
TaktOSErr_t TaktOSQueueReceiveSlowPath(TaktOSQueue_t *pQue, void *pData,
                                        uint32_t TimeoutTicks)
{
    uint32_t saved = TaktOSEnterCritical();

    // Re-check under lock  a sender may have added an item.
    uint8_t* slot = TaktQueueRingGet(pQue);

    if (slot != nullptr)
    {
        TaktQueueFastCopy(pData, slot, pQue->ItemSize);
        bool needSwitch = false;

        if (pQue->SendList.pHead != nullptr)
        {
        	needSwitch = TaktQueueHandoffFromSender(pQue);
        }

        TaktOSExitCritical(saved);

        if (needSwitch)
        {
        	TaktOSCtxSwitch();
        }

        return TAKTOS_OK;
    }

    // Still empty  block.  Store destination pointer for direct handoff:
    // TaktQueueHandoffToReceiver will copy the sender's item directly here,
    // eliminating the re-compete race with other non-blocked receivers.
    TaktOSThread_t* cur = TaktOSCurrentThread();
    cur->State      = TAKTOS_BLOCKED;
    cur->WakeReason = TAKT_WOKEN_BY_EVENT;
    cur->WakeTick   = 0u;
    cur->pWaitNext  = nullptr;
    cur->pMsg       = pData;   // sender will copy to here

    TaktBlockTask(cur);
    TaktWaitListInsert(&pQue->RecvList, cur);
    cur->pWaitList = &pQue->RecvList;

    if (TimeoutTicks != TAKTOS_WAIT_FOREVER)
    {
        cur->WakeTick = TaktOSTickCount() + TimeoutTicks;
        TaktSleepListAdd(cur);
    }
    else
    {
        cur->WakeTick = 0u;
    }

    TaktOSExitCritical(saved);
    TaktOSCtxSwitch();

    // After wakeup: item was copied to data by TaktQueueHandoffToReceiver.
    cur->pMsg     = nullptr;
    cur->WakeTick = 0u;
    const uint8_t wr_r = cur->WakeReason;

    if (__builtin_expect((int)wr_r, 0))
    {
        if (wr_r == TAKT_WOKEN_BY_TIMEOUT)
        {
        	return TAKTOS_ERR_TIMEOUT;
        }

        return TAKTOS_ERR_INTERRUPTED;  // TAKT_WOKEN_BY_RESUME  data NOT filled
    }

    return TAKTOS_OK;
}

/**
 * @brief	Return the current item count  implementation.  @see TaktOSQueue.h.
 */
uint32_t TaktOSQueueCount(const TaktOSQueue_t *pQue)
{
    return (pQue != nullptr) ? (pQue->Capacity - pQue->Avail) : 0u;
}

//--- Direct-handoff helpers (COLD  only called when a waiter is present) 

TAKT_COLD TAKT_NOINLINE
/**
 * @brief	Direct-handoff to a blocked receiver  implementation.  @see TaktOSQueue.h.
 *
 * Pops the highest-priority receiver, copies @p pData into its pMsg slot, wakes
 * it, and returns true if a context switch is needed.
 * Must be called inside a critical section.
 */
bool TaktQueueHandoffToReceiver(TaktOSQueue_t *pQue, const void *pData)
{
    TaktOSThread_t* waiter = TaktWaitListPop(&pQue->RecvList);

    if (waiter != nullptr)
    {
        TaktQueueFastCopy(waiter->pMsg, pData, pQue->ItemSize);
        waiter->pMsg       = nullptr;
        waiter->pWaitList  = nullptr;

        if (waiter->WakeTick != 0u)
        {
            TaktSleepListRemove(waiter);
            waiter->WakeTick = 0u;
        }

        waiter->WakeReason = TAKT_WOKEN_BY_EVENT;
        waiter->State      = TAKTOS_READY;
        TaktReadyTask(waiter);
        TaktOSThread_t* cur = TaktOSCurrentThread();

        return (waiter->Priority > cur->Priority) ||
               (cur->State != TAKTOS_READY && cur->State != TAKTOS_RUNNING);
    }

    return false;
}

TAKT_COLD TAKT_NOINLINE
/**
 * @brief	Direct-handoff from a blocked sender  implementation.  @see TaktOSQueue.h.
 *
 * Pops the highest-priority sender, copies its pMsg data into the ring buffer
 * slot just freed by the receiver, wakes the sender, and returns true if a
 * context switch is needed.
 * Must be called inside a critical section.
 */
bool TaktQueueHandoffFromSender(TaktOSQueue_t *pQue)
{
    TaktOSThread_t* waiter = TaktWaitListPop(&pQue->SendList);

    if (waiter != nullptr)
    {
        uint8_t* slot = TaktQueueRingPut(pQue);  // guaranteed: caller just freed one
        TaktQueueFastCopy(slot, waiter->pMsg, pQue->ItemSize);
        waiter->pMsg       = nullptr;
        waiter->pWaitList  = nullptr;

        if (waiter->WakeTick != 0u)
        {
            TaktSleepListRemove(waiter);
            waiter->WakeTick = 0u;
        }

        waiter->WakeReason = TAKT_WOKEN_BY_EVENT;
        waiter->State      = TAKTOS_READY;
        TaktReadyTask(waiter);
        TaktOSThread_t* cur = TaktOSCurrentThread();

        return (waiter->Priority > cur->Priority) ||
               (cur->State != TAKTOS_READY && cur->State != TAKTOS_RUNNING);
    }

    return false;
}

#ifndef TAKT_INLINE_OPTIMIZATION


/**
 * @brief	Send an item  non-inline fallback.  @see TaktOSQueue.h.
 */
TaktOSErr_t TaktOSQueueSend(TaktOSQueue_t *pQue, const void *pData, bool bBlocking,
                            uint32_t TimeoutTicks)
{
    if (pQue == nullptr || pData == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }
#ifdef TAKT_DEBUG_CHECKS
    if (reinterpret_cast<uintptr_t>(pData) & 3u)
    {
        return TAKTOS_ERR_INVALID;
    }
#endif

    uint32_t state = TaktOSEnterCritical();

    // Invariant: receivers only block when the queue is empty (available == capacity).
    // Therefore recvList.pHead != nullptr implies available == capacity  the second
    // check is logically redundant but left as a defensive assertion in debug builds.
    if (TAKT_UNLIKELY(pQue->RecvList.pHead != nullptr))
    {
        bool needSwitch = TaktQueueHandoffToReceiver(pQue, pData);
        TaktOSExitCritical(state);

        if (needSwitch)
        {
        	TaktOSCtxSwitch();
        }

        return TAKTOS_OK;
    }

    uint8_t* slot = TaktQueueRingPut(pQue);

    if (TAKT_LIKELY(slot != nullptr))
    {
        TaktQueueFastCopy(slot, pData, pQue->ItemSize);
        TaktOSExitCritical(state);

        return TAKTOS_OK;
    }

    TaktOSExitCritical(state);
    if (!bBlocking || TimeoutTicks == TAKTOS_NO_WAIT)
    {
    	return TAKTOS_ERR_FULL;
    }

    return TaktOSQueueSendSlowPath(pQue, pData, TimeoutTicks);
}

/**
 * @brief	Receive an item  non-inline fallback.  @see TaktOSQueue.h.
 */
TaktOSErr_t TaktOSQueueReceive(TaktOSQueue_t *pQue, void *pData, bool bBlocking, uint32_t TimeoutTicks)
{
    if (pQue == nullptr || pData == nullptr)
    {
        return TAKTOS_ERR_INVALID;
    }
#ifdef TAKT_DEBUG_CHECKS
    if (reinterpret_cast<uintptr_t>(pData) & 3u)
    {
    	return TAKTOS_ERR_INVALID;
    }
#endif

    uint32_t state = TaktOSEnterCritical();
    uint8_t* slot = TaktQueueRingGet(pQue);

    if (TAKT_LIKELY(slot != nullptr))
    {
        TaktQueueFastCopy(pData, slot, pQue->ItemSize);
        bool needSwitch = false;

        if (TAKT_UNLIKELY(pQue->SendList.pHead != nullptr))
        {
        	needSwitch = TaktQueueHandoffFromSender(pQue);
        }
        TaktOSExitCritical(state);

        if (needSwitch)
        {
        	TaktOSCtxSwitch();
        }

        return TAKTOS_OK;
    }

    TaktOSExitCritical(state);

    if (!bBlocking || TimeoutTicks == TAKTOS_NO_WAIT)
    {
    	return TAKTOS_ERR_EMPTY;
    }

    return TaktOSQueueReceiveSlowPath(pQue, pData, TimeoutTicks);
}
#endif

