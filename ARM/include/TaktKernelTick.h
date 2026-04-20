/**---------------------------------------------------------------------------
@file   TaktKernelTick.h

@brief  TaktOS ARM Cortex-M SysTick handler alias

Declares the SysTick_Handler alias for TaktKernelTickHandler so that the
tick ISR lands directly in the ARM vector table under the expected name.

INCLUDE RULE  this file must be included ONLY by src/taktos.cpp.
__attribute__((alias(...))) requires the target symbol (TaktKernelTickHandler)
to be defined in the same translation unit.  Including this header in any
other TU will produce a compile-time error on GCC 12+.

TaktKernel.h includes TaktKernelCore.h (not this file) for the other
ARM-specific kernel primitives (TaktOSCtxSwitch, TaktOSStartFirst, etc.).

Safety boundary: IN  alias declaration is in the same TU as its target.

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
#ifndef __TAKTKERNELTICK_H__
#define __TAKTKERNELTICK_H__

// Alias SysTick_Handler  TaktKernelTickHandler so the ARM vector table
// resolves the SysTick exception to the kernel tick function.
// Valid only in src/taktos.cpp  TaktKernelTickHandler is defined there.
// Excluded on host/stub and RISC-V builds (no SysTick on those targets).
#if !defined(TAKT_ARCH_STUB) && !defined(__x86_64__) && !defined(__i386__) \
    && !defined(__riscv)
#ifdef __cplusplus
extern "C"
#endif
void __attribute__((alias("TaktKernelTickHandler"))) SysTick_Handler(void);
#endif

#endif // __TAKTKERNELTICK_H__
