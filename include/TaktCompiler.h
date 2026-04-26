/**---------------------------------------------------------------------------
@file   TaktCompiler.h

@brief  Compiler portability macros for TaktOS

Supports three toolchains:
  GCC   (arm-none-eabi-gcc, riscv32-unknown-elf-gcc)   __GNUC__ defined
  Clang (armclang, clang --target=...)                 __clang__ defined
  IAR   (iccarm)                                       __ICCARM__ defined

Detection order: __clang__ is checked before __GNUC__ because Clang defines
both macros.  Add new compiler guards after the IAR block.

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
#ifndef __TAKTCOMPILER_H__
#define __TAKTCOMPILER_H__

#ifdef __cplusplus
  #define TAKT_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
  #define TAKT_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

//  Force-inline 
// Marks a function for mandatory inlining.  All three toolchains support
// __attribute__((always_inline)); IAR also accepts it as a compatibility
// extension from v8.x onward.  The static qualifier is required to avoid
// multiple-definition errors when the header is included in multiple TUs.

#if defined(__clang__) || defined(__GNUC__) || defined(__ICCARM__)
#  define TAKT_ALWAYS_INLINE  __attribute__((always_inline)) inline
#else
#  define TAKT_ALWAYS_INLINE  inline
#endif

//  No-inline 
// Prevents the compiler from inlining a slow-path function into a hot caller.
// IAR: __attribute__((noinline)) is accepted as a GCC compatibility extension.
// Fallback: empty  the compiler may inline, but correctness is unaffected.

#if defined(__clang__) || defined(__GNUC__)
#  define TAKT_NOINLINE  __attribute__((noinline))
#elif defined(__ICCARM__)
#  define TAKT_NOINLINE  __attribute__((noinline))
#else
#  define TAKT_NOINLINE
#endif

//  Cold (slow-path hint) 
// Moves the function to a cold section and optimises for size.
// Clang supports this since clang 3.4.  IAR has no equivalent  maps to empty.

// TAKT_COLD: move slow paths to .text.cold so they cannot shift hot code.
// On nRF52832/STM32 the linker script wildcard *(.text*) places .text.cold
// after .text, quarantining slow-path growth from ThreadMetric hot loops.
// optimize("Os") keeps cold functions byte-minimal.
#if defined(__clang__) || defined(__GNUC__)
// 'cold' suppresses __builtin_expect on GCC ARM  removed.
// section(".text.cold") preserves the linker layout quarantine.
#  define TAKT_COLD  __attribute__((noinline, section(".text.cold")))
#else
#  define TAKT_COLD  __attribute__((noinline))
#endif

//  Weak symbol 
// IAR uses __weak; GCC/Clang use __attribute__((weak)).

#if defined(__ICCARM__)
#  define TAKT_WEAK  __weak
#elif defined(__clang__) || defined(__GNUC__)
#  define TAKT_WEAK  __attribute__((weak))
#else
#  define TAKT_WEAK
#endif

//  Alignment 
// __attribute__((aligned(N))) is supported by GCC, Clang, and IAR (as a
// compatibility extension).  Use TAKT_ALIGNED(N) on declarations.

#if defined(__clang__) || defined(__GNUC__) || defined(__ICCARM__)
#  define TAKT_ALIGNED(n)  __attribute__((aligned(n)))
#else
#  define TAKT_ALIGNED(n)
#endif

//  Branch prediction hints 
// __builtin_expect is GCC/Clang only.  IAR evaluates the expression as-is.

#if defined(__clang__) || defined(__GNUC__)
#  define TAKT_LIKELY(x)    (__builtin_expect(!!(x), 1))
#  define TAKT_UNLIKELY(x)  (__builtin_expect(!!(x), 0))
#else
#  define TAKT_LIKELY(x)    (x)
#  define TAKT_UNLIKELY(x)  (x)
#endif

//  Count leading zeros 
// __builtin_clz is GCC/Clang.  IAR provides __CLZ() via <intrinsics.h>
// (Cortex-M) or the CMSIS core_cm*.h header which defines __CLZ().
// Both expand to the CLZ instruction on ARM Cortex-M.

#if defined(__clang__) || defined(__GNUC__)
#  define TAKT_CLZ(x)  __builtin_clz(x)
#elif defined(__ICCARM__)
#  define TAKT_CLZ(x)  __CLZ(x)
#else
#  error "TAKT_CLZ: unsupported toolchain  provide a CLZ implementation"
#endif

//  Trap / abort 
// Generates an unconditional fault / debug breakpoint.
// GCC/Clang: __builtin_trap()  BKPT #0 on ARM.
// IAR:       __breakpoint(0)   BKPT #0.

#if defined(__clang__) || defined(__GNUC__)
#  define TAKT_TRAP()  __builtin_trap()
#elif defined(__ICCARM__)
#  define TAKT_TRAP()  __breakpoint(0)
#else
#  define TAKT_TRAP()  do { for(;;); } while(0)
#endif

//  Restrict 
// C99 restrict / compiler extension for pointer aliasing hints.
// IAR uses __restrict; GCC/Clang accept both __restrict and __restrict__.

#if defined(__ICCARM__)
#  define TAKT_RESTRICT  __restrict
#elif defined(__clang__) || defined(__GNUC__)
#  define TAKT_RESTRICT  __restrict__
#else
#  define TAKT_RESTRICT
#endif

//  Memcpy hint 
// __builtin_memcpy lets GCC/Clang inline small fixed-size copies as
// load/store sequences.  IAR uses the standard memcpy which the compiler
// may or may not inline; include <string.h> at the call site.

#if defined(__clang__) || defined(__GNUC__)
#  define TAKT_MEMCPY(dst, src, n)  __builtin_memcpy((dst), (src), (n))
#else
#  define TAKT_MEMCPY(dst, src, n)  memcpy((dst), (src), (n))
#endif

#endif // __TAKTCOMPILER_H__
