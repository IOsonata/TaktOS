/**---------------------------------------------------------------------------
@file   TaktOSThread.h

@brief  TaktOS thread API  C and C++

Thread memory is user-provided (IOsonata / static-allocation style):

  C:
    static uint8_t g_MyMem[TAKTOS_THREAD_MEM_SIZE(512)] TAKT_ALIGNED(4);
    TaktOSThreadCreate(g_MyMem, sizeof(g_MyMem), Entry, NULL, priority);

  C++:
    static uint8_t g_MyMem[TAKTOS_THREAD_MEM_SIZE(512)] TAKT_ALIGNED(4);
    TaktOSThread myThread;
    myThread.Create(g_MyMem, sizeof(g_MyMem), Entry, NULL, priority);

Memory block layout:
  [ TaktOSThread_t | guard (0xDEADBEEF) | arch reserve | stack ... top ]
  Stack grows DOWN from top toward guard word.
  arch reserve covers any guard-region bytes / alignment slack the active
  port needs before the first usable stack byte.
  Guard word checked on context switch to detect stack overflow.

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
#ifndef __TAKTOSTHREAD_H__
#define __TAKTOSTHREAD_H__

#include <string.h>

#include "TaktOS.h"
#include "TaktCompiler.h"
#include "TaktKernel.h"

#define TAKTOS_THREAD_MIN_STACK_SIZE			16u

#define TAKTOS_THREAD_MEM_SIZE(StackSize)		(TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD + (StackSize))

#ifdef __cplusplus
extern "C" {
#endif

//--- C API ------------------------------------------------------

/**
 * @brief Create a thread using user-provided memory.
 *
 * Memory layout must follow:
 * [TaktOSThread_t | guard word | arch reserve | stack ... top]
 *
 * @param pStackMem		: Pointer to the thread memory block.
 * @param StackMemSize	: Size of the memory block in bytes.
 * @param ThreadFct		: Thread entry function.
 * @param pArg			: Opaque pointer passed to @p entry.
 * @param Priority		: Thread priority (1..TAKTOS_MAX_PRI-1). Priority 0 is reserved for idle.
 *
 * @return Thread handle on success, nullptr on failure.
 */
hTaktOSThread_t TaktOSThreadCreate(void *pStackMem, uint32_t StackMemSize,
                                   void (*ThreadFct)(void*), void *pArg,
                                   uint8_t Priority);

/**
 * @brief	Suspend a thread.
 *
 * Self-suspend (@p hThread == TaktOSCurrentThread()) blocks the caller and
 * may not be invoked from ISR context  the kernel rejects it with
 * TAKTOS_ERR_INVALID rather than corrupting the run queue.
 *
 * @param	hThread : Thread handle.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p hThread is invalid
 *          or self-suspend is invoked from ISR context.
 */
TaktOSErr_t TaktOSThreadSuspend(hTaktOSThread_t hThread);

/**
 * @brief	Put a thread to sleep for the given number of kernel ticks.
 *
 * The thread becomes ready when the global tick reaches now + @p Ticks.
 * This keeps the tick API aligned with timeout semantics: SleepTicks(1)
 * waits for the next tick, SleepTicks(2) waits for two tick advances, etc.
 *
 * Use this form when the caller naturally thinks in ticks (timeout
 * passthrough, tick-driven state machines, micro-benchmarks).  When the
 * caller has milliseconds, use TaktOSThreadSleep() instead.
 *
 * Sleep blocks the target thread (or the calling thread if @p hThread is
 * the current thread) and therefore may not be invoked from ISR context.
 * The kernel rejects the call with TAKTOS_ERR_INVALID before touching any
 * scheduler state.
 *
 * @param	hThread : Thread handle.
 * @param	Ticks   : Minimum ticks to sleep.  Zero returns immediately.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID on invalid handle/state
 *          or when invoked from ISR context.
 */
TaktOSErr_t TaktOSThreadSleepTicks(hTaktOSThread_t hThread, uint32_t Ticks);

/**
 * @brief	Put a thread to sleep for at least the given number of milliseconds.
 *
 * Convenience wrapper over TaktOSThreadSleepTicks() that converts @p Ms
 * to the number of kernel ticks needed to cover at least that duration:
 *
 *     ticks = ceil(Ms * TickHz / 1000)
 *
 * The conversion rounds UP — so a 1 ms request at TickHz = 100 (10 ms tick
 * period) produces a 1-tick wait, which then sleeps for at least one full
 * tick-period (10 ms).  This preserves the "at least Ms" semantic at the
 * cost of overshoot when the request is shorter than one tick period.
 *
 * Ms = 0 returns immediately without entering the kernel.  Conversion uses
 * a 64-bit intermediate to avoid overflow at large Ms values.
 *
 * @param	hThread : Thread handle.
 * @param	Ms      : Minimum milliseconds to sleep.  Zero returns immediately.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID on invalid handle/state.
 */
TaktOSErr_t TaktOSThreadSleep(hTaktOSThread_t hThread, uint32_t Ms);

/**
 * @brief	Destroy a thread and remove it from all scheduler/wait structures.
 *
 * After this call the handle is a tombstone (State == TAKTOS_DEAD)
 * and must not be used again.
 *
 * @param	hThread : Thread handle.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p hThread is invalid.
 */
TaktOSErr_t TaktOSThreadDestroy(hTaktOSThread_t hThread);

/**
 * @brief	Hand off execution from the current thread to a specific thread.
 *
 * Makes @p hNext READY (cancelling any blocked wait) and blocks the caller.
 * @p hNext is guaranteed to run next unless a thread at strictly higher
 * priority is already in the ready queue, in which case that higher-priority
 * thread runs first (correct priority scheduling, not a violation).
 *
 * Blocks the caller and therefore may not be invoked from ISR context;
 * the kernel rejects an ISR caller with TAKTOS_ERR_INVALID before any
 * scheduler state is touched.
 *
 * @param	hNext : Target thread handle.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID on invalid handle/state
 *          or when invoked from ISR context.
 */
TaktOSErr_t TaktOSThreadHandOff(hTaktOSThread_t hNext);

/**
 * @brief	Stack guard word value.
 *
 * Written at thread creation by TaktOSThreadCreate(); checked every tick by
 * TaktKernelTickHandler() on cores without a hardware stack limit (M0/M4/M7
 * with no MPU bound).  Public so the tick-path inline below can reference it.
 */
#define TAKTOS_GUARD_WORD     0xDEADBEEFu

/**
 * @brief	Offset from the TCB base to the stack guard word.
 *
 * The guard sits immediately after the TCB structure; see
 * TaktOSThreadCreate() in src/taktos_thread.cpp for the layout.
 */
#define TAKTOS_GUARD_OFFSET   sizeof(TaktOSThread_t)

/**
 * @brief	Check stack guard word for stack overflow / memory corruption detection.
 *
 * Inlined: this runs every tick in TaktKernelTickHandler() on M4/M7 without an
 * MPU-bound kernel handler, and the body is a single load + compare.  Inlining
 * eliminates the BL/BX call frame (~5 cycles) and removes the cold-fetch risk
 * for a function called once per millisecond — measurable in RT_TICK_JITTER
 * even at 64 MHz with ICACHE on.
 *
 * @param	hThread : Thread handle.
 * @return	true if the guard word is intact, false if overflow or corruption is detected.
 */
TAKT_ALWAYS_INLINE bool TaktOSThreadGuardCheck(hTaktOSThread_t hThread)
{
    const uint32_t *guard =
        (const uint32_t*)((const uint8_t*)hThread + TAKTOS_GUARD_OFFSET);
    return (*guard == TAKTOS_GUARD_WORD);
}

/**
 * @brief	Stack overflow hook  weak symbol, application may override.
 *
 * Called from the tick handler with interrupts disabled when the guard word
 * of @p hThread is found corrupt.  The default implementation traps.
 *
 * @param	hThread : Thread whose stack overflowed.
 */
TAKT_WEAK void TaktOSStackOverflowHandler(hTaktOSThread_t hThread);

/**
 * @brief	Wake threads whose sleep / timed-wait deadline has expired.
 *
 * Called by the kernel tick handler on every tick.
 *
 * @param	TickCount : Current global tick count.
 */
void TaktThreadWakeTick(uint32_t TickCount);

/**
 * @brief	Insert a thread into the READY run queue.
 *
 * Internal scheduler helper  not for direct application use.
 *
 * @param	hThread : Thread handle to make ready.
 */
void TaktReadyTask(hTaktOSThread_t hThread);

/**
 * @brief	Yield the CPU to another READY thread of equal or higher priority.
 *
 * If no other thread of the same or higher priority is ready, this is a no-op.
 */
void TaktOSThreadYield(void);


/**
 * @brief	Resume a suspended thread.
 *
 * If the thread is BLOCKED or SLEEPING it is moved to the READY queue and a
 * context switch is triggered if the resumed thread has higher priority than
 * the caller.  If the thread is already READY or RUNNING this is a no-op.
 *
 * @param	hThread : Thread handle.
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if @p hThread is invalid.
 */
TaktOSErr_t TaktOSThreadResume(hTaktOSThread_t hThread);

/**
 * @brief	Return the handle of the currently executing thread.
 *
 * Useful for generic libraries, ownership assertions, and self-registration.
 * Equivalent to pthread_self().
 *
 * @return	Handle of the current thread.
 */
TAKT_ALWAYS_INLINE hTaktOSThread_t TaktOSCurrentThread(void) {
	extern TaktKernelCtx_t g_TaktosCtx;

	return g_TaktosCtx.pCurrent;
}


#ifdef __cplusplus
}   // extern "C"

//--- C++ wrapper  inline, zero overhead ------------------------

class TaktOSThread {
public:
    hTaktOSThread_t Create(void *pStackMem, uint32_t StackMemSize,
                            void (*ThreadFct)(void*), void *pArg,
                            uint8_t Priority) {
    	vhThread = TaktOSThreadCreate(pStackMem, StackMemSize, ThreadFct, pArg, Priority);

        return vhThread;
    }

    TaktOSErr_t Suspend() { return TaktOSThreadSuspend(vhThread); }
    TaktOSErr_t Resume() { return TaktOSThreadResume(vhThread); }
    TaktOSErr_t Sleep(uint32_t Ms) { return TaktOSThreadSleep(vhThread, Ms); }
    TaktOSErr_t SleepTicks(uint32_t Ticks) { return TaktOSThreadSleepTicks(vhThread, Ticks); }
    TaktOSErr_t Destroy() { return TaktOSThreadDestroy(vhThread); }
    TaktOSErr_t HandOff(hTaktOSThread_t hNext) { return TaktOSThreadHandOff(hNext); }

private:
    hTaktOSThread_t vhThread = nullptr;
};

#endif  // __cplusplus

#endif // __TAKTOSTHREAD_H__

