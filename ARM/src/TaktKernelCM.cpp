/**---------------------------------------------------------------------------
@file   TaktKernelCM.cpp

@brief  TaktOS ARM Cortex-M port  tick source (SysTick) and stack initialisation

Provides:
  TaktOSTickInit(KernClockHz, TickHz)  weak default using SysTick.
    Any HAL can override with a strong definition to use a different
    peripheral (GP timer, LPTIM, RTC) without touching the kernel.
  TaktKernelStackInit(pStackTop, entry, arg)  builds the initial fake
    Cortex-M exception frame so PendSV can restore it on first switch-in.

Supports Cortex-M0/M0+, M4/M4F, M7/M7F, M33, M55.
FPU context (S0S15 lazy stack, S16S31 explicit) is handled here when
TAKT_FPU_FULL_SAVE is defined.

Safety boundary: IN  branch + line coverage on target.

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
#include <stddef.h>
#include <stdint.h>

#include "TaktKernel.h"
#include "TaktOSThread.h"

#include "systick.h"
#if defined(__ARM_ARCH_8M_MAIN__)
#  include "takt_mpu_v8m.h"
#else
#  include "takt_mpu.h"
#endif

/**
 * @brief	Thread exit trampoline  called when a task entry function returns.
 *
 * LR is initialized to this function in the fake exception frame built by
 * TaktKernelStackInit(), so a task that reaches its closing brace rather than
 * calling pthread_exit() or an explicit destroy will land here.
 * Calls TaktOSThreadDestroy() on the current thread, then loops in WFI
 *  the destroy triggers a context switch, so the loop is never reached.
 */
extern "C" void TaktOSThreadExit(void);

extern TaktKernelCtx_t g_TaktosCtx;

extern "C" void TaktKernelTickHandler(void);

#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
extern "C" void PendSV_Handler_MPU(void);
extern "C" void SVC_Handler_MPU(void);
#endif

#define SCB_VTOR_ADDR                   0xE000ED08UL
#define TAKT_ARM_VECTOR_TABLE_ALIGN     128u

static bool TaktArmVectorTableIsWritable(uintptr_t HandlerBaseAddr)
{
    const uintptr_t region = HandlerBaseAddr & 0xF0000000u;

    if (HandlerBaseAddr == 0u)
    {
        return false;
    }

    if ((HandlerBaseAddr & (TAKT_ARM_VECTOR_TABLE_ALIGN - 1u)) != 0u)
    {
        return false;
    }

    if (HandlerBaseAddr != (uintptr_t)(*((volatile uint32_t*)SCB_VTOR_ADDR)))
    {
        return false;
    }

    return region == 0x10000000u || region == 0x20000000u ||
           region == 0x30000000u || region == 0x60000000u ||
           region == 0x70000000u;
}

extern "C" bool TaktKernelHandlerAssign(uintptr_t HandlerBaseAddr)
{
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
    if (MpuRegionCount() == 0u)
    {
        return false;
    }

    if (!TaktArmVectorTableIsWritable(HandlerBaseAddr))
    {
        return false;
    }

    uint32_t *vt = (uint32_t*)HandlerBaseAddr;

    TaktOSMpuInit();
    TaktOSMemFaultEnable();

    vt[11] = (uint32_t)SVC_Handler_MPU;
    vt[14] = (uint32_t)PendSV_Handler_MPU;
    vt[15] = (uint32_t)TaktKernelTickHandler;

    __DSB();
    __ISB();

    return true;
#else
    (void)HandlerBaseAddr;

    return false;
#endif
}


/**
 * @brief  TaktOS Cortex-M4 initial stack frame builder
 *
 * Builds a fake Cortex-M exception frame so PendSV_Handler restores the first
 * task identically to every subsequent context switch. No special-case code in
 * PendSV.
 *
 * Stack layout after TaktKernelStackInit() (grows down, SP at bottom):
 *
 * stackTop  ...
 *            xPSR  = 0x01000000   Thumb bit, no FPU active
 *            PC    = entry
 *            LR    = TaktOSThreadExit
 *            R12   = 0
 *            R3    = 0            HW frame  auto-popped by BX LR
 *            R2    = 0
 *            R1    = 0
 *            R0    = arg
 *            R11   = 0
 *            R10   = 0
 *            R9    = 0
 *            R8    = 0            SW frame  restored by LDMIA in PendSV
 *            R7    = 0
 *            R6    = 0
 *            R5    = 0
 *            R4    = 0
 * SP        ...
 *
 * @param pStackTop	: Pointer to top of stack
 * @param pThreadFct: Pointer to thread function
 * @param pArg 		: Pointer to arg to pass to thread function
 */
void *TaktKernelStackInit(void *pStackTop, void (*pThreadFct)(void*), void *pArg)
{
    uintptr_t spaddr = ((uintptr_t)pStackTop) & ~(uintptr_t)(TAKTOS_THREAD_STACK_TOP_ALIGN - 1u);
    uint32_t *sp = (uint32_t*)spaddr;

    // HW frame  auto-popped by processor on BX LR (EXC_RETURN)
    *(--sp) = 0x01000000u;                  // xPSR  Thumb bit
    *(--sp) = (uint32_t)pThreadFct;              // PC
    *(--sp) = (uint32_t)TaktOSThreadExit;   // LR
    *(--sp) = 0u;                           // R12
    *(--sp) = 0u;                           // R3
    *(--sp) = 0u;                           // R2
    *(--sp) = 0u;                           // R1
    *(--sp) = (uint32_t)pArg;               // R0

    // SW frame  restored by LDMIA r0!, {r4-r11, lr} in PendSV_Handler
    // M4/M7/M33/M55: push EXC_RETURN so PendSV can detect per-thread FPU state.
    // M0/M0+: no FPU, PendSV saves exactly 8 words (r4-r11)  no LR slot needed.
    //         SVC_Handler uses ADDS #16 (not #20) to reach the HW frame.
#if !defined(__ARM_ARCH_6M__)
    *(--sp) = 0xFFFFFFFDu;                  // LR (EXC_RETURN  tells PendSV: no FPU context)
#endif
    *(--sp) = 0u;                           // R11
    *(--sp) = 0u;                           // R10
    *(--sp) = 0u;                           // R9
    *(--sp) = 0u;                           // R8
    *(--sp) = 0u;                           // R7
    *(--sp) = 0u;                           // R6
    *(--sp) = 0u;                           // R5
    *(--sp) = 0u;                           // R4

    return (void*)sp;
}

/**
 * @brief  Set the PendSV exception priority.
 *
 * PendSV priority is in SHPR3 bits [23:16].
 * Must be called during init and set to the SAME value as SysTick (0xFF).
 *
 * Rationale: PendSV resets to priority 0x00 (highest). At priority 0x00,
 * a pended PendSV would immediately preempt any running user ISR instead of
 * tail-chaining after it. PendSV would then execute its BX lr with
 * EXC_RETURN = 0xFFFFFFF1 (handler mode) rather than 0xFFFFFFFD (thread/PSP),
 * corrupting the preempted ISR's return state. Setting PendSV to 0xFF ensures
 * it always tail-chains after all user ISRs have completed.
 *
 * @param  Prio  8-bit priority value. Must be 0xFF for correct RTOS operation.
 */
__STATIC_FORCEINLINE void PendSVSetPriority(uint8_t Prio)
{
    volatile uint32_t *shpr3 = (volatile uint32_t*)SCB_SHPR3_ADDR;
    *shpr3 = (*shpr3 & 0xFF00FFFFul) | ((uint32_t)Prio << 16);
    __DSB();
    __ISB();
}


// Weak default: SysTick tick source, input clock = KernClockHz.
// Override this function in the HAL for any target where:
//   - SysTick is absent (use a GP timer or RTC instead), or
//   - the tick peripheral ISR name differs from SysTick_Handler.
// Signature stays identical regardless of which peripheral is used.
__attribute__((weak))
/**
 * @brief	Weak default tick source initialiser  uses ARM SysTick.
 *
 * Configures SysTick at the requested frequency and sets both PendSV and
 * SysTick to priority 0xFF (lowest) so that all user ISRs pre-empt the
 * tick and PendSV tail-chains correctly after them.
 * Override this symbol with a strong definition to use a different tick
 * peripheral (GP timer, LPTIM, RTC) without changing the kernel.
 *
 * @see TaktOSTickInit in TaktKernel.h for the full API.
 */
void TaktOSTickInit(uint32_t KernClockHz, uint32_t TickHz, TaktOSTickClockSrc_t TickClockSrc)
{
    SysTickStop();
    PendSVSetPriority(0xFFu);               // lowest priority  must tail-chain after ISRs
    SysTickSetPriority(0xFFu);              // lowest priority


    uint32_t clksrc = TickClockSrc == TAKTOS_TICK_CLOCK_PROCESSOR ? SYSTICK_CLOCK_SRC_MCU : SYSTICK_CLOCK_SRC_EXT;
    SysTickSetFrequency(clksrc, KernClockHz, TickHz);

    SysTickEnableInt();
    SysTickStart();
}

