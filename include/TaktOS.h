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
    TAKTOS_ERR_ALIGN,        // pointer or size violates required alignment
                             //   (e.g. queue pData / pStorage / ItemSize not 4-byte aligned)
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
// PROCESSOR is the default (value 0), so a zero-initialised cfg field
// selects it automatically without explicit assignment.
typedef enum {
    TAKTOS_TICK_CLOCK_PROCESSOR = 0,
    TAKTOS_TICK_CLOCK_REFERENCE = 1,
} TaktOSTickClockSrc_t;

//--- Portable defaults ---------------------------------
// Fall-back value used by TaktOSInit() when TaktOSCfg_t.TickHz is 0.
// Port-specific defaults (e.g. SoftIntAddr on RV32) live in the active
// port's TaktKernelCore.h.
#define TAKTOS_DEFAULT_TICK_HZ          1000u

//--- Priority level defines -------------------------------------
// TaktOS standard priority scale.  Single scale, used for thread priority,
// mutex ceiling, and kernel tick interrupt priority.  Higher value = more
// urgent.
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

//--- Tick interrupt priority ---------------------------
// Value written to TaktOSCfg_t.TickPriority and passed to TaktOSTickInit().
//
// Uses the TaktOS standard priority scale above, not a hardware interrupt
// priority encoding.  Range TAKTOS_PRIORITY_LOWEST (1) to
// TAKTOS_PRIORITY_CRITICAL (31), higher value = more urgent, same scale and
// direction as thread priority.  The arch port converts the value to its
// interrupt controller encoding and clamps input above the top of the scale.
// Each port documents its conversion in its TaktKernelCore.h.
//
// TAKTOS_TICK_PRIORITY_DEFAULT (0) selects the port default.  0 is the
// reserved idle level and is never a valid interrupt priority, so a
// zero-initialised cfg field keeps the previous behaviour.  A port whose
// tick source has no configurable priority ignores the field.
//
// A tick priority above an application ISR lets the tick pre-empt that ISR.
// Scheduler state stays consistent because kernel critical sections mask all
// interrupts.  The cost is that the ISR worst-case latency increases by the
// tick handler execution time.
#define TAKTOS_TICK_PRIORITY_DEFAULT    0u

//--- Kernel configuration --------------------------------
// Passed to TaktOSInit() at kernel startup.  Any field left zero is
// filled in by TaktOSInit() from the documented per-field default, so the
// minimum-effort init is:
//     TaktOSCfg_t cfg = { .KernClockHz = <chip clock> };
//     TaktOSInit(&cfg);
// Set only the fields that deviate from the default for your chip.
typedef struct __TaktOSCfg {
    uint32_t              KernClockHz;     // Tick peripheral input clock (Hz).  REQUIRED - 0 -> error.
    uint32_t              TickHz;          // Kernel tick rate (Hz).  0 -> TAKTOS_DEFAULT_TICK_HZ (1000).
    TaktOSTickClockSrc_t  TickClockSrc;    // Tick clock domain.  0 -> PROCESSOR.
    uint32_t              TickPriority;    // Tick interrupt priority on the TaktOS standard scale
                                           //   (TAKTOS_PRIORITY_LOWEST..TAKTOS_PRIORITY_CRITICAL,
                                           //   higher = more urgent).  The port maps it to the
                                           //   hardware encoding.  0 -> port default.
    uintptr_t             HandlerBaseAddr; // Arch exception/trap base address.  0 -> port's statically linked handlers.
    uintptr_t             SoftIntAddr;     // Software-interrupt trigger MMIO.  0 -> port's TAKT_DEFAULT_SOFT_INT_ADDR.
} TaktOSCfg_t;

//--- Priority Ceiling Protocol -----------------------------------
// IPCP (Immediate Priority Ceiling Protocol) per-thread state limit.
// A thread may hold at most TAKTOS_MAX_HELD_PCP_MUTEXES priority-protect
// mutexes simultaneously.  Sized small on purpose: realistic safety-critical
// designs hold one or two PCP mutexes at a time; deeper nesting is a design
// smell.  Overflow on Lock() returns TAKTOS_ERR_INVALID rather than corrupt
// the held-ceiling stack.
#ifndef TAKTOS_MAX_HELD_PCP_MUTEXES
#define TAKTOS_MAX_HELD_PCP_MUTEXES 4u
#endif

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
 * Sets up scheduler state, the idle thread, the tick source, and the
 * deferred-context-switch trigger.  All configuration is passed in a single
 * TaktOSCfg_t struct; see its field documentation above for per-field
 * meaning.  Any zero / DEFAULT field is replaced with the port's documented
 * default - only KernClockHz is required.
 *
 * Minimum-effort init for a standard CLINT-based RV32 chip:
 * @code
 *     TaktOSCfg_t cfg = { .KernClockHz = 16000000u };
 *     TaktOSInit(&cfg);
 *     TaktOSStart();
 * @endcode
 *
 * Init that overrides one default (ESP32-C3 - no CLINT, so SoftIntAddr
 * must be set explicitly to the SYSTEM_CPU_INTR_FROM_CPU_0 register):
 * @code
 *     TaktOSCfg_t cfg = {
 *         .KernClockHz = 16000000u,
 *         .SoftIntAddr = 0x600C0014u,
 *     };
 *     TaktOSInit(&cfg);
 *     TaktOSStart();
 * @endcode
 *
 * Init that raises the tick above the lowest priority level.  TickPriority
 * uses the standard priority scale, so the same source builds for every
 * port; each port converts the level to its interrupt controller encoding:
 * @code
 *     TaktOSCfg_t cfg = {
 *         .KernClockHz  = 64000000u,
 *         .TickPriority = TAKTOS_PRIORITY_HIGH,
 *     };
 *     TaktOSInit(&cfg);
 *     TaktOSStart();
 * @endcode
 *
 * @param  Cfg : Pointer to a filled-in TaktOSCfg_t.  Must remain valid
 *               for the duration of the call; TaktOS copies what it needs.
 *
 * @return	TAKTOS_OK on success, TAKTOS_ERR_INVALID if Cfg is null or
 *          KernClockHz is 0.
 */
TaktOSErr_t TaktOSInit(const TaktOSCfg_t *Cfg);

/**
 * @brief	Start the kernel
 *
 * This function never returns
 *
 * @return	None
 */
void TaktOSStart(void);          // never returns

/**
 * @brief	Return the configured kernel tick rate in Hz.
 *
 * Returns the TickHz value passed to TaktOSInit().  Useful for converting
 * between time units and ticks at user level - e.g. timeout APIs that
 * accept ticks but the caller has milliseconds.
 *
 * @return	Tick rate in Hz, or 0 if TaktOSInit() has not yet been called.
 */
uint32_t TaktOSGetTickHz(void);

#ifdef __cplusplus
}
#endif

#endif // __TAKTOS_H__

