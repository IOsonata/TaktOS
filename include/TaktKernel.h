/**---------------------------------------------------------------------------
@file   TaktKernel.h

@brief  TaktOS private kernel API  internal header for kernel objects

Included transitively by kernel object headers that need the arch-inline
critical-section and scheduler helpers.  Application code must NOT include
this header directly.

Kernel ownership boundary:
  IN   Scheduler, thread lifecycle, semaphore, mutex, queue
  OUT  Timers, interrupt routing, peripheral drivers, POSIX layer

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
#ifndef __TAKTKERNEL_H__
#define __TAKTKERNEL_H__

#include "TaktOS.h"
#include "TaktOSCriticalSection.h"
#include "TaktKernelCore.h"

// Stack sizing / alignment are architecture-defined in TaktKernelCore.h.
//
// Required macros:
//   TAKTOS_STACK_GUARD_ALIGN
//   TAKTOS_THREAD_MEM_SIZE(StackSize)
//
// The alignment value is the required alignment for pStackBottom when the
// architecture uses it as a hardware stack-limit / no-access guard base.
// The size macro must include the complete memory required for a new thread,
// including any pStackBottom alignment slack, reserved guard-region bytes,
// stack-top alignment slack, and the initial frame built by TaktKernelStackInit().
#ifndef TAKTOS_STACK_GUARD_ALIGN
#  error "TAKTOS_STACK_GUARD_ALIGN must be defined by TaktKernelCore.h"
#endif

#if ((TAKTOS_STACK_GUARD_ALIGN & (TAKTOS_STACK_GUARD_ALIGN - 1u)) != 0u)
#  error "TAKTOS_STACK_GUARD_ALIGN must be a power of two"
#endif

//--- Wakeup reason -------------------------------------------------
// Written before blocking; read after wakeup to distinguish a normal
// event (sem Give / mutex Unlock) from a tick-ISR timeout expiry.
#define TAKT_WOKEN_BY_EVENT    0u
#define TAKT_WOKEN_BY_TIMEOUT  1u
#define TAKT_WOKEN_BY_RESUME   2u   // thread woken by external Resume/HandOff,
                                    //   not by the blocking kernel object

typedef struct __TaktOSThread_s 	TaktOSThread_t;

#pragma pack(push, 4)

//--- Common wait list -----------------------------------------
// Embedded by every kernel object that blocks threads.
// Using an embedded struct (not a raw pointer) lets the TCB store
// TaktKernelWaitList_t *pWaitList  a single typed pointer  instead of
// TaktOSThread_t **ppWaitHead (pointer-to-pointer).  The address-of
// is taken once inside the wait-list helpers, not scattered at every
// call site.
typedef struct __TaktKernelWaitList_s {
	TaktOSThread_t *pHead;  // highest-priority waiter at front
} TaktKernelWaitList_t;



struct __TaktOSThread_s {
	void          *pSp;						//!< offset  0  loaded by context switch handler
	uint8_t        Priority;				//!< offset  4  031
	TaktOSState_t  State;					//!< offset  5  uint8_t enum; no padding before WakeTick
	uint8_t        WakeReason;				//!< offset  6 - TAKT_WOKEN_BY_EVENT / _TIMEOUT
	uint8_t        _pad1;					//!< offset  7 - reserved  alignment pad
	uint32_t       WakeTick;				//!< offset  8
	struct __TaktOSThread_s *pNext;			//!< offset 12  run queue and sleep list
	void          *pStackBottom;			//!< offset 16  aligned stack floor / guard base
	                                        //!<   MPU guard base when the arch binds the MPU-aware handler path
	                                        //!<   PSPLIM value  (TAKTOS_STACK_LIMIT=1, M33+)
	struct __TaktOSThread_s *pWaitNext;		//!< offset 20  semaphore / mutex wait list only
	TaktKernelWaitList_t    *pWaitList;		//!< offset 24  wait list this thread is blocked on (null if none)
	                                		//!<   tick ISR uses this to remove on timeout
	void                    *pMsg;			//!< offset 28  queue direct-handoff: src ptr (sender) / dst ptr (receiver)
	                                		//!<   set before blocking on queue send/recv; cleared by wake path
};                 // 32 bytes


typedef struct {
	TaktOSThread_t *pCurrent;      // offset 0
	TaktOSThread_t *pNextThread;   // offset 4
} TaktKernelCtx_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Return the current kernel tick count.
 *
 * Reads the global tick counter without a critical section.  Safe for
 * single-core read-only access; use a critical section if the result must
 * be consistent with a subsequent scheduler operation.
 *
 * @return	Current tick count (wraps at UINT32_MAX).
 */
TAKT_ALWAYS_INLINE uint32_t TaktOSTickCount(void) {
	extern uint32_t g_TickCount;

    return g_TickCount;
}

//--- Scheduler --------------------------------------------------

/**
 * @brief	Move a thread to the READY run queue.
 *
 * Internal scheduler primitive  not for direct application use.
 * Must be called inside a critical section.
 *
 * @param	hThread : Thread to make ready.
 */
void TaktReadyTask(hTaktOSThread_t hThread);

/**
 * @brief	Remove a thread from the READY run queue and mark it BLOCKED.
 *
 * Internal scheduler primitive  not for direct application use.
 * Must be called inside a critical section.
 *
 * @param	hThread : Thread to block.
 */
void TaktBlockTask(hTaktOSThread_t hThread);

/**
 * @brief	Force a specific thread to run next (HandOff helper).
 *
 * Sets pNextThread so that the named thread is selected on the next context
 * switch, provided no thread of strictly higher priority is already waiting.
 * Called exclusively by @ref TaktOSThreadHandOff.
 * Must be called inside a critical section.
 *
 * @param	hThread : Thread to designate as next-to-run.
 */
void TaktForceNextThread(hTaktOSThread_t hThread);

//--- Sleep list -------------------------------------------------

/**
 * @brief	Insert a thread into the ordered sleep list.
 *
 * The sleep list is sorted by WakeTick (ascending).  Called when a thread
 * enters a timed wait.  Must be called inside a critical section.
 *
 * @param	hThread : Thread to insert (WakeTick must already be set).
 */
void TaktSleepListAdd(hTaktOSThread_t hThread);

/**
 * @brief	Remove a thread from the sleep list.
 *
 * Called on timeout expiry or when a blocking kernel object wakes the thread
 * before the deadline.  Must be called inside a critical section.
 *
 * @param	hThread : Thread to remove.
 */
void TaktSleepListRemove(hTaktOSThread_t hThread);


//--- Arch port must implement ----------------------------------------
//
//  TaktOSTickInit    tick source init (weak default = SysTick).
//                     Override in the HAL for any target where the tick
//                     peripheral is not SysTick, or where its input clock
//                     cannot be expressed as a simple ratio of KernClockHz.
//                     Signature is identical regardless of peripheral used.
//  TaktOSCtxSwitch   pend PendSV (ARM) / set CLINT MSIP (RISC-V).
//                     Declared in the arch-specific TaktKernelCore.h /
//                     TaktOSCriticalSection.h header included above.
//  TaktOSStartFirst  launch the first task.  Mechanism is ISA-specific:
//                     ARM raises SVC #0  SVC_Handler switches MSPPSP;
//                     RISC-V loads the first frame and executes mret.
//                     Declared in the arch-specific header (not here) so
//                     the call site in taktos.cpp resolves it at link time
//                     without exposing ARM-only SVC semantics in this file.
//  TaktKernelStackInit   build initial fake exception frame.

/**
 * @brief	Initialize the arch-specific tick source.
 *
 * Weak symbol  the default implementation configures SysTick.  Override in
 * the HAL for targets without SysTick or with a non-standard clock path.
 *
 * @param	KernClockHz  : Input clock of the tick peripheral in Hz (not
 *                        necessarily the CPU frequency).
 * @param	TickHz       : Desired kernel tick rate in Hz.
 * @param	TickClockSrc : Clock path selector (TAKTOS_TICK_CLOCK_PROCESSOR or
 *                        TAKTOS_TICK_CLOCK_REFERENCE).
 */
void TaktOSTickInit(uint32_t KernClockHz, uint32_t TickHz, TaktOSTickClockSrc_t TickClockSrc);

/**
 * @brief	Build the initial fake exception frame on a new thread's stack.
 *
 * Places register values on @p pStackTop so that the first context restore
 * will call @p pThreadFct(pArg) with a valid exception-return state.
 *
 * @param	pStackTop    : Pointer to the top (highest address) of the thread stack.
 * @param	pThreadFct   : Thread entry function.
 * @param	pArg         : Argument passed to the entry function.
 * @return	Updated stack pointer after the fake frame has been pushed.
 */
void *TaktKernelStackInit(void *pStackTop, void (*pThreadFct)(void*), void *pArg);

/**
 * @brief	Bind the kernel exception / trap handlers to an architecture-defined base.
 *
 * Called once from TaktOSInit(). The application / system owns the storage and
 * routing of the exception table. The arch port may patch only the reserved
 * kernel handler slots it uses.
 *
 * @param	HandlerBaseAddr : Architecture-defined handler base address.
 * @return	true if an MPU-aware kernel handler path was bound, false if the
 *          default no-MPU handlers remain active.
 */
bool TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr);

//--- Wait list helpers  shared by semaphore, mutex, and queue -------
// All operate on TaktKernelWaitList_t so call sites store a single typed
// pointer (TaktKernelWaitList_t *pWaitList) instead of TaktOSThread_t **.

/**
 * @brief	Insert a thread into a priority-ordered wait list.
 *
 * The list is kept in descending priority order (highest priority at head).
 * Threads of equal priority are inserted after existing threads of the same
 * priority (FIFO within a priority level).
 * Must be called inside a critical section.
 *
 * @param	pList    : Wait list to insert into.
 * @param	pThread  : Thread to insert.
 */
static inline void TaktWaitListInsert(TaktKernelWaitList_t *pList, TaktOSThread_t *pThread) {
    pThread->pWaitNext = 0;
    TaktOSThread_t **p = &pList->pHead;
    while (*p != 0 && (*p)->Priority >= pThread->Priority)
    {
        p = &(*p)->pWaitNext;
    }
    pThread->pWaitNext = *p;
    *p = pThread;
}

/**
 * @brief	Pop the highest-priority thread from a wait list.
 *
 * Removes and returns the head of the list (highest-priority waiter).
 * Returns NULL if the list is empty.
 * Must be called inside a critical section.
 *
 * @param	pList : Wait list to pop from.
 * @return	Pointer to the removed thread, or NULL if the list is empty.
 */
static inline TaktOSThread_t *TaktWaitListPop(TaktKernelWaitList_t *pList) {
    TaktOSThread_t *t = pList->pHead;
    if (t != 0)
    {
        pList->pHead = t->pWaitNext;
        t->pWaitNext = 0;
    }
    return t;
}

/**
 * @brief	Remove an arbitrary thread from a wait list (O(n)).
 *
 * Used by the tick ISR on timeout expiry to remove a thread from an object's
 * wait list, and by Give/Unlock to cancel a concurrent timed-wait entry.
 * No-op if @p pThread is not in @p pList.
 * Must be called inside a critical section.
 *
 * @param	pList    : Wait list to search.
 * @param	pThread  : Thread to remove.
 */
static inline void TaktWaitListRemove(TaktKernelWaitList_t *pList, TaktOSThread_t *pThread) {
    TaktOSThread_t **p = &pList->pHead;
    while (*p != 0 && *p != pThread)
    {
        p = &(*p)->pWaitNext;
    }
    if (*p == pThread)
    {
        *p = pThread->pWaitNext;
        pThread->pWaitNext = 0;
    }
}


#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __TAKTKERNEL_H__

