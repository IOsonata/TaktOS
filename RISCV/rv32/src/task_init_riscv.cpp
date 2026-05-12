/**---------------------------------------------------------------------------
@file   task_init_riscv.cpp

@brief  TaktOS RV32 task stack initialisation

Builds the fake RV32 trap frame consumed by ctx_switch.S so that the first
context switch into a new task is structurally identical to any subsequent
context switch — no special-case code in the switch handler.

Counterpart to ARM/src/TaktKernelCM.cpp::TaktKernelStackInit.

@author Nguyen Hoan Hoang
@date   May 2026

@license

MIT License

Copyright (c) 2026 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

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

#include "TaktOSThread.h"
#include "port_rv32.h"

/* Compile-time assertion: TCB.pSp must be the first word so ctx_switch.S
 * can use TCB_SP=0 for the load/store. Mirrors the static_asserts in the
 * ARM port. */
static_assert(offsetof(TaktOSThread_t, pSp) == 0u,
              "TCB_SP must be at TaktOSThread_t offset 0 — see ctx_switch.S");

/* Thread exit trampoline — entered if a task function returns rather than
 * calling TaktOSThreadDestroy() / pthread_exit().  Defined in taktos.cpp. */
extern "C" void TaktOSThreadExit(void);

/**
 * @brief   Build the initial RV32 trap frame for a new task.
 *
 * Places a 128-byte frame on the task stack matching the save layout of
 * TaktArchTrapEntry, with mepc = task entry and mstatus.MPIE = 1 so that
 * mret enables interrupts as it enters the task.
 *
 * @param   pStackTop   Top of the allocated stack region.  Aligned down
 *                      to 16 bytes (RV ABI requirement) before the frame
 *                      is laid out.
 * @param   pThreadFct  Task entry function.
 * @param   pArg        Argument passed in a0 (x10) to the task function.
 *
 * @return  Stack pointer at the bottom of the initial frame, to be stored
 *          in TCB.pSp.
 */
extern "C" void *TaktKernelStackInit(void *pStackTop,
                                     void (*pThreadFct)(void *),
                                     void *pArg)
{
    /* RISC-V ABI: 16-byte stack alignment. */
    uintptr_t sp = ((uintptr_t)pStackTop) & ~(uintptr_t)15u;

    /* Allocate the 128-byte frame. */
    sp -= FRAME_SIZE;
    uint32_t *frame = reinterpret_cast<uint32_t *>(sp);

    /* Zero the whole frame so unset regs (s0–s11, t1–t6, a1–a7, t0)       *
     * come up at 0 on first entry — matches the ARM port's behavior.       *
     * Inline loop avoids pulling in <string.h> / libc memset on freestanding.*/
    for (uint32_t i = 0u; i < (FRAME_SIZE / 4u); ++i) {
        frame[i] = 0u;
    }

    /* mepc = task entry point; mret jumps here. */
    frame[FRAME_MEPC    / 4u] = reinterpret_cast<uint32_t>(pThreadFct);

    /* mstatus: MPP = M-mode, MPIE = 1 → mret enables MIE. */
    frame[FRAME_MSTATUS / 4u] = MSTATUS_TASK_INIT;

    /* ra = trampoline — invoked if the task function returns. */
    frame[FRAME_RA      / 4u] = reinterpret_cast<uint32_t>(TaktOSThreadExit);

    /* a0 = task argument (RISC-V ABI: first arg in x10). */
    frame[FRAME_A0      / 4u] = reinterpret_cast<uint32_t>(pArg);

    /* gp and tp: capture the values set by startup so the first mret into  *
     * this task gets the correct globals-pointer.  Bare-metal single-      *
     * address-space builds keep these constant for the life of the system, *
     * so capturing once at task creation is sufficient.  Leaving them zero *
     * would crash any code generated with -mrelax (GCC default) that uses  *
     * GP-relative addressing for global access.                            */
    uint32_t cur_gp;
    uint32_t cur_tp;
    __asm__ volatile ("mv %0, gp" : "=r"(cur_gp));
    __asm__ volatile ("mv %0, tp" : "=r"(cur_tp));
    frame[FRAME_GP      / 4u] = cur_gp;
    frame[FRAME_TP      / 4u] = cur_tp;

    /* All other registers (t0–t6, a1–a7, s0–s11) remain zero from memset. */

    return reinterpret_cast<void *>(sp);
}
