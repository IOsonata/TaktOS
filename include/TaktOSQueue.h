/**---------------------------------------------------------------------------
@file   TaktOSQueue.h

@brief  TaktOS message queue  fixed-capacity ring buffer, C and C++

Storage model:
  - User provides the backing byte buffer (no hidden allocation).
  - Queue object stores only metadata: indices, capacity, wait lists.

Alignment requirements:
  - storage must be 4-byte aligned.
  - itemSize must be a non-zero multiple of 4.
  - data pointer passed to Send / Receive must be 4-byte aligned.
  fast_copy() uses word-sized LDM/STM which require 4-byte alignment on all
  supported targets.  Alignment cannot be silently relaxed.  Use
  __attribute__((aligned(4))) on all queue backing buffers.

Buffer sizing macro:
    static uint8_t qbuf[CFIFO_TOTAL_MEMSIZE(8, sizeof(MyMsg_t))]
                        __attribute__((aligned(4)));

Send / Receive hot paths are inlined; slow paths (blocking, timeout) are in
taktos_queue.cpp.

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
#ifndef __TAKTOSQUEUE_H__
#define __TAKTOSQUEUE_H__

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"   // TAKTOS_WAIT_FOREVER / TAKTOS_NO_WAIT
#include "TaktKernel.h"

#pragma pack(push, 4)

typedef struct __TaktOSQueue_s {
    //  Ring-buffer hot fields 
    uint32_t Avail;			//!< free slot count: 0=full, capacity=empty
    uint8_t *pWrite;		//!< next write address (direct pointer)
    uint8_t *pRead;       	//!< next read address  (direct pointer)
    //  Sizing / bounds 
    uint32_t Capacity;      // total slot count (used for empty check)
    uint8_t *pBuffer;        // ring buffer base address
    uint8_t *pBufEnd;        // buffer + capacity*itemSize (wrap sentinel)
    uint32_t ItemSize;      // bytes per item (must be multiple of 4)
    //  Wait lists 
    TaktKernelWaitList_t  SendList;     // blocked senders
    TaktKernelWaitList_t  RecvList;     // blocked receivers
} TaktOSQueue_t;
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Initialize a queue object.
 *
 * The backing buffer must be 4-byte aligned and sized with the helper macro:\n
 *   uint8_t buf[CFIFO_TOTAL_MEMSIZE(capacity, itemSize)] __attribute__((aligned(4)));
 *
 * @param	pQue      : Pointer to caller-allocated queue object.
 * @param	pStorage  : Pointer to the 4-byte-aligned backing byte buffer.
 * @param	ItemSize  : Size of one item in bytes  must be a non-zero multiple of 4.
 * @param	Capacity  : Maximum number of items the queue can hold.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID on null / mis-aligned / bad size.
 */
TaktOSErr_t TaktOSQueueInit(TaktOSQueue_t *pQue, void *pStorage, uint32_t ItemSize, uint32_t Capacity);

/**
 * @brief	Direct-handoff a sent item to the highest-priority blocked receiver.
 *
 * Called (COLD NOINLINE) by @ref TaktOSQueueSend when RecvList is non-empty.
 * Copies @p pData directly into the receiver's @c pMsg buffer and wakes the
 * receiver.  Must be called inside a critical section.
 *
 * @param	pQue   : Pointer to queue.
 * @param	pData  : Pointer to the item to deliver (4-byte aligned, ItemSize bytes).
 * @return	true if a context switch is required after exiting the critical section.
 */
bool TaktQueueHandoffToReceiver(TaktOSQueue_t *pQue, const void *pData);

/**
 * @brief	Direct-handoff a pending item from the highest-priority blocked sender.
 *
 * Called (COLD NOINLINE) by @ref TaktOSQueueReceive when SendList is non-empty
 * and a slot has just been freed by the receiver.  Copies the sender's item into
 * the ring buffer and wakes the sender.  Must be called inside a critical section.
 *
 * @param	pQue : Pointer to queue.
 * @return	true if a context switch is required after exiting the critical section.
 */
bool TaktQueueHandoffFromSender(TaktOSQueue_t *pQue);

#ifndef TAKT_INLINE_OPTIMIZATION
/**
 * @brief	Send an item to the queue.
 *
 * Non-inline variant active when TAKT_INLINE_OPTIMIZATION is not defined.
 * See the inline version above for full semantics.
 *
 * @param	pQue          : Pointer to queue.
 * @param	pData         : Item to send (4-byte aligned, ItemSize bytes).
 * @param	bBlocking     : true to block when the queue is full.
 * @param	TimeoutTicks  : Maximum ticks to block.
 * @return	TAKTOS_OK, TAKTOS_ERR_FULL, TAKTOS_ERR_TIMEOUT, TAKTOS_ERR_INTERRUPTED,
 *          or TAKTOS_ERR_INVALID.
 */
TaktOSErr_t TaktOSQueueSend(TaktOSQueue_t *pQue, const void *pData, bool bBlocking, uint32_t TimeoutTicks);

/**
 * @brief	Receive an item from the queue.
 *
 * Non-inline variant active when TAKT_INLINE_OPTIMIZATION is not defined.
 * See the inline version above for full semantics.
 *
 * @param	pQue          : Pointer to queue.
 * @param	pData         : Buffer to receive into (4-byte aligned, ItemSize bytes).
 * @param	bBlocking     : true to block when the queue is empty.
 * @param	TimeoutTicks  : Maximum ticks to block.
 * @return	TAKTOS_OK, TAKTOS_ERR_EMPTY, TAKTOS_ERR_TIMEOUT, TAKTOS_ERR_INTERRUPTED,
 *          or TAKTOS_ERR_INVALID.
 */
TaktOSErr_t TaktOSQueueReceive(TaktOSQueue_t *pQue, void *pData, bool bBlocking, uint32_t TimeoutTicks);

#else
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

/**
 * @brief	Slow path for TaktOSQueueSend  called when the queue is full.
 *
 * Must not be called directly.  Called by @ref TaktOSQueueSend when blocking
 * is permitted and the ring buffer is full.  Suspends the caller for up to
 * @p TimeoutTicks ticks.
 *
 * @param	pQue          : Pointer to queue.
 * @param	pData         : Item to send (4-byte aligned, ItemSize bytes).
 * @param	TimeoutTicks  : Maximum ticks to block.
 * @return	TAKTOS_OK, TAKTOS_ERR_TIMEOUT, or TAKTOS_ERR_INTERRUPTED.
 */
TaktOSErr_t TaktOSQueueSendSlowPath   (TaktOSQueue_t *pQue, const void *pData, uint32_t TimeoutTicks);

/**
 * @brief	Slow path for TaktOSQueueReceive  called when the queue is empty.
 *
 * Must not be called directly.  Called by @ref TaktOSQueueReceive when blocking
 * is permitted and the ring buffer is empty.  Suspends the caller for up to
 * @p TimeoutTicks ticks.
 *
 * @param	pQue          : Pointer to queue.
 * @param	pData         : Buffer to receive into (4-byte aligned, ItemSize bytes).
 * @param	TimeoutTicks  : Maximum ticks to block.
 * @return	TAKTOS_OK, TAKTOS_ERR_TIMEOUT, or TAKTOS_ERR_INTERRUPTED.
 */
TaktOSErr_t TaktOSQueueReceiveSlowPath(TaktOSQueue_t *pQue, void *pData, uint32_t TimeoutTicks);

/**
 * @brief	Send an item to the queue.
 *
 * If a receiver is blocked waiting on an empty queue the item is handed off
 * directly (zero ring-buffer copies).  Otherwise the item is written to the
 * ring buffer on the fast path.  If the queue is full:
 *   - @p bBlocking == false or @p TimeoutTicks == TAKTOS_NO_WAIT returns
 *     TAKTOS_ERR_FULL immediately.
 *   - Otherwise the caller blocks for up to @p TimeoutTicks ticks.
 *
 * @param	pQue          : Pointer to queue (must not be null).
 * @param	pData         : Pointer to the item to send (4-byte aligned, ItemSize bytes).
 * @param	bBlocking     : true to block when the queue is full.
 * @param	TimeoutTicks  : Maximum ticks to block (TAKTOS_WAIT_FOREVER / TAKTOS_NO_WAIT).
 * @return	TAKTOS_OK, TAKTOS_ERR_FULL, TAKTOS_ERR_TIMEOUT, TAKTOS_ERR_INTERRUPTED,
 *          or TAKTOS_ERR_INVALID.
 */
static TAKT_ALWAYS_INLINE TaktOSErr_t TaktOSQueueSend(TaktOSQueue_t *pQue, const void *pData,
													  bool bBlocking, uint32_t TimeoutTicks) {
    // Null check unconditional  null handles cause UB in both debug and release.
    if (pQue == NULL || pData == NULL)
    {
    	return TAKTOS_ERR_INVALID;
    }
#ifdef TAKT_DEBUG_CHECKS
    // Alignment check is a developer-error diagnostic only.
    if (reinterpret_cast<uintptr_t>(pData) & 3u)
    {
    	return TAKTOS_ERR_INVALID;
    }
#endif

    uint32_t state = TaktOSEnterCritical();

    // Invariant: receivers only block when the queue is empty (available == capacity).
    // Therefore recvList.pHead != NULL implies available == capacity  the second
    // check is logically redundant but left as a defensive assertion in debug builds.
    if (TAKT_UNLIKELY(pQue->RecvList.pHead != NULL))
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

    if (TAKT_LIKELY(slot != NULL))
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
 * @brief	Receive an item from the queue.
 *
 * Dequeues the oldest item on the fast path.  If a sender is blocked waiting
 * on a full queue, its item is pulled directly into the ring buffer after the
 * slot is freed.  If the queue is empty:
 *   - @p bBlocking == false or @p TimeoutTicks == TAKTOS_NO_WAIT returns
 *     TAKTOS_ERR_EMPTY immediately.
 *   - Otherwise the caller blocks for up to @p TimeoutTicks ticks.
 *
 * @param	pQue          : Pointer to queue (must not be null).
 * @param	pData         : Buffer to receive into (4-byte aligned, ItemSize bytes).
 * @param	bBlocking     : true to block when the queue is empty.
 * @param	TimeoutTicks  : Maximum ticks to block (TAKTOS_WAIT_FOREVER / TAKTOS_NO_WAIT).
 * @return	TAKTOS_OK, TAKTOS_ERR_EMPTY, TAKTOS_ERR_TIMEOUT, TAKTOS_ERR_INTERRUPTED,
 *          or TAKTOS_ERR_INVALID.
 */
static TAKT_ALWAYS_INLINE TaktOSErr_t TaktOSQueueReceive(TaktOSQueue_t *pQue, void *pData,
														 bool bBlocking, uint32_t TimeoutTicks) {
    // Null check unconditional  null handles cause UB in both debug and release.
    if (pQue == NULL || pData == NULL)
    {
    	return TAKTOS_ERR_INVALID;
    }
#ifdef TAKT_DEBUG_CHECKS
    // Alignment check is a developer-error diagnostic only.
    if (reinterpret_cast<uintptr_t>(pData) & 3u)
    {
    	return TAKTOS_ERR_INVALID;
    }
#endif

    uint32_t state = TaktOSEnterCritical();
    uint8_t* slot = TaktQueueRingGet(pQue);

    if (TAKT_LIKELY(slot != NULL))
    {
        TaktQueueFastCopy(pData, slot, pQue->ItemSize);
        bool needSwitch = false;

        if (TAKT_UNLIKELY(pQue->SendList.pHead != NULL))
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

/**
 * @brief	Return the number of items currently held in the queue.
 *
 * Lightweight observability helper  does NOT take a critical section.
 * The returned value may be stale by the time the caller acts on it.
 * Safe for single-core diagnostics and ring-size hints; do NOT use to
 * make scheduling decisions without re-checking under a critical section.
 *
 * @param	pQue : Pointer to queue (const  read-only access).
 * @return	Current item count (0 = empty, Capacity = full).
 */
uint32_t TaktOSQueueCount(const TaktOSQueue_t *pQue);

#define TaktQueueInit     TaktOSQueueInit
#define TaktQueueSend     TaktOSQueueSend
#define TaktQueueReceive  TaktOSQueueReceive
#define TaktQueueCount    TaktOSQueueCount

#ifdef __cplusplus
}

class TaktOSQueue {
public:
    TaktOSErr_t Init(void *pStorage, uint32_t ItemSize, uint32_t Capacity) {
        return TaktOSQueueInit(&vQue, pStorage, ItemSize, Capacity);
    }
    TaktOSErr_t Send(const void *pData, bool bBlocking = false, uint32_t TimeoutTicks = TAKTOS_WAIT_FOREVER) {
        return TaktOSQueueSend(&vQue, pData, bBlocking, TimeoutTicks);
    }
    TaktOSErr_t Receive(void *pData, bool bBlocking = true, uint32_t TimeoutTicks = TAKTOS_WAIT_FOREVER) {
        return TaktOSQueueReceive(&vQue, pData, bBlocking, TimeoutTicks);
    }
    uint32_t Count() const { return TaktOSQueueCount(&vQue); }
    TaktOSQueue_t *Handle() { return &vQue; }
    operator TaktOSQueue_t*() { return &vQue; }
private:
    TaktOSQueue_t vQue{};
};

typedef TaktOSQueue TaktQueue;

#endif // __cplusplus

#endif // __TAKTOSQUEUE_H__

