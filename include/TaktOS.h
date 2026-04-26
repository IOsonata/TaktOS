/**---------------------------------------------------------------------------
@file   TaktOS.h

@brief  TaktOS  public kernel API (C and C++)

Single top-level include for application and kernel object code.  Pulls in
all kernel object headers (thread, semaphore, mutex, queue) and defines the
TaktOSInit / TaktOSStart / TaktOSTickHandler entry points.

Kernel scope  what TaktOS owns:
  Scheduler, thread lifecycle, semaphore, mutex, message queue, and the ONE
  scheduler tick timer (SysTick / CLINT) that drives TaktKernelTickHandler.
  No other timers.

Outside kernel scope  do NOT add to TaktOS:
  Timers      All application timers belong to the HAL (IOsonata
               DevIntrf_t). Use TaktOSThreadSleep() for periodic actions.
               The kernel does not own, configure, or abstract any timer
               beyond its own scheduler tick.
  Interrupts  Vector table, NVIC/PLIC configuration, and all IRQ handlers
               except the tick and context-switch interrupts.

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
#ifndef __TAKTOS_H__
#define __TAKTOS_H__

#include <stdint.h>
#include <stdbool.h>

#include "TaktCompiler.h"

#define TAKTOS_WAIT_FOREVER     0xFFFFFFFFu
#define TAKTOS_NO_WAIT          0u

//--- Return codes ----------------------------------------

typedef enum {
    TAKTOS_OK = 0,
    TAKTOS_ERR_TIMEOUT,
    TAKTOS_ERR_FULL,
    TAKTOS_ERR_EMPTY,
    TAKTOS_ERR_BUSY,         // resource is held by another (e.g. mutex non-blocking)
    TAKTOS_ERR_INTERRUPTED,  // blocking wait cancelled by external Resume/HandOff;
                             //   no token/lock/item was transferred
    TAKTOS_ERR_NOMEM,
    TAKTOS_ERR_INVALID,
} TaktOSErr_t;

//--- Thread states ---------------------------------------

typedef enum {
    TAKTOS_READY = 0,
    TAKTOS_RUNNING,     // Reserved for future use.
                        // In the current implementation, the active thread's
                        // State field remains TAKTOS_READY during execution.
                        // Context switch does NOT write RUNNING on entry or
                        // READY on exit  the savings (~2 stores per switch,
                        // ~12 cycles) are more valuable than the distinction.
                        // Callers that check "is this thread actively running?"
                        // should compare the pointer to g_TaktosCtx.pCurrent,
                        // not inspect the State field.
    TAKTOS_BLOCKED,
    TAKTOS_SLEEPING,
    TAKTOS_DEAD,    // handle is a tombstone  thread has been destroyed
} TaktOSState_t;

// Lock the byte-size guarantee that the C23 ': uint8_t' annotation gave us.
TAKT_STATIC_ASSERT(TAKTOS_DEAD <= 0xFFu, "TaktOSState_t must fit in uint8_t");

//--- Tick clock source ---------------------------------
// Selects which clock path drives the kernel tick source when the target
// tick implementation supports multiple SysTick clock domains.
// For non-SysTick targets, the port may ignore this value.
typedef enum {
    TAKTOS_TICK_CLOCK_REFERENCE = 0,
    TAKTOS_TICK_CLOCK_PROCESSOR = 1,
} TaktOSTickClockSrc_t;

//--- Priority level defines -------------------------------------
// Priority 0 is reserved for the idle thread.
// User threads: LOWEST(1) through HIGHEST(31).

#define TAKTOS_MAX_PRI    			32u

#define TAKTOS_PRIORITY_IDLE        0u   // reserved  do not use in application code

#define TAKTOS_PRIORITY_LOWEST      1u
#define TAKTOS_PRIORITY_LOW         8u
#define TAKTOS_PRIORITY_NORMAL      16u
#define TAKTOS_PRIORITY_HIGH        24u
#define TAKTOS_PRIORITY_HIGHEST     28u
#define TAKTOS_PRIORITY_CRITICAL    31u

//--- Handles ---------------------------------------------

typedef struct __TaktOSThread_s *   hTaktOSThread_t;
typedef struct __TaktOSSem_s *      hTaktOSSem_t;
typedef struct __TaktOSMutex_s *    hTaktOSMutex_t;
typedef struct __TaktOSQueue_s *    hTaktOSQueue_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Initialize the kernel
 *
 *  This function must be called first to initialize the kernel.
 *
 * @param	KernClockHz : Frequency of the tick counter input in Hz.
 *                        This is the clock that drives the tick peripheral
 *                        (SysTick, GP timer, CLINT, etc.)  NOT necessarily
 *                        the CPU frequency.
 *
 *                        Examples:
 *                          nRF52832  (M4,  64 MHz): SystemCoreClock
 *                          nRF54L15  (M33, 128 MHz): SystemCoreClock
 *                          STM32 HCLK/8 ref path:   SystemCoreClock / 8
 *
 *                        The HAL or application owns this value.
 *                        TaktOS passes it straight to TaktOSTickInit().
 *
 * @param 	TickHz : Desired kernel tick rate in Hz (e.g. 1000)
 *
 * @param  TickClockSrc : Tick clock source selection for targets that support
 *                        multiple SysTick clock paths.
 *                        Use TAKTOS_TICK_CLOCK_PROCESSOR when the tick runs
 *                        from the CPU clock, or TAKTOS_TICK_CLOCK_REFERENCE
 *                        when it runs from a reference / divided clock path.
 *
 * @param  HandlerBaseAddr : Architecture-defined exception / trap handler base address.
 *                          ARM: base address of the application-owned RAM vector table
 *                               to patch. Pass 0 to keep the default statically linked
 *                               handlers (no-MPU fallback).
 *                          RISC-V: trap base address (e.g. mtvec base) when the port
 *                               uses one. Pass 0 to keep the port default.
 *
 * @return	Error code
 */
TaktOSErr_t TaktOSInit(uint32_t KernClockHz, uint32_t TickHz,
                      TaktOSTickClockSrc_t TickClockSrc, uintptr_t HandlerBaseAddr);

/**
 * @brief	Start the kernel
 *
 * This function never returns
 *
 * @return	None
 */
void TaktOSStart(void);          // never returns

#ifdef __cplusplus
}
#endif

#endif // __TAKTOS_H__

