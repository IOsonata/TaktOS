/**---------------------------------------------------------------------------
@file   task_init_riscv.cpp

@brief  TaktOS standard RV32 task stack initialisation

Builds the fake RV32 trap frame expected by RISCV/rv32/ctx_switch.S and
TaktOSStartFirst(). This is the standard RV32 counterpart to the stack-frame
builder in ARM/src/TaktKernelCM.cpp.

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
#include <string.h>

#include "TaktOSThread.h"
#include "port_rv32.h"

extern "C" void TaktOSThreadExit(void);

void *TaktKernelStackInit(void *pStackTop, void (*pThreadFct)(void*), void *pArg)
{
    uintptr_t sp = ((uintptr_t)pStackTop) & ~(uintptr_t)15u;
    sp -= CTX_FRAME_BYTES;

    uint32_t *frame = (uint32_t*)sp;
    memset(frame, 0, CTX_FRAME_BYTES);

    frame[CTX_MEPC / 4u]    = (uint32_t)pThreadFct;
    frame[CTX_MSTATUS / 4u] = (3u << 11u) | (1u << 7u);
    frame[CTX_X1 / 4u]      = (uint32_t)TaktOSThreadExit;
    frame[CTX_X10 / 4u]     = (uint32_t)pArg;

    return (void*)sp;
}
