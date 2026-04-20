/**---------------------------------------------------------------------------
@file   takt_mpu.h

@brief  TaktOS ARM Cortex-M MPU Land layer  static-inline MMIO primitives

Pure static-inline MMIO helpers for the ARM Memory Protection Unit.
No state, no function pointers, no objects  IOsonata Land idiom:
'Don't objectify the dirt.'

Supported cores:
  Cortex-M4, M7  (ARMv7-M, 8 regions)
  Cortex-M33, M55 (ARMv8-M Mainline, 8 or 16 regions)

TaktOS MPU stack-guard strategy:
  REGION 7 is reserved as a per-task stack guard.
  The guard covers 32 bytes starting at the 32-byte-aligned floor of the
  task stack (AP = no-access, XN = 1).  A MemManage fault fires on
  any access into the guard region, catching stack overflow deterministically.

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
#ifndef __TAKT_MPU_H__
#define __TAKT_MPU_H__

#include <stdint.h>
#include "cmsis_compiler.h"

//  MPU register base 
#define MPU_BASE            0xE000ED90u

//  MPU registers (direct access) 
#define MPU_TYPE    (*((volatile uint32_t*)(MPU_BASE + 0x00u)))  ///< Type register
#define MPU_CTRL    (*((volatile uint32_t*)(MPU_BASE + 0x04u)))  ///< Control register
#define MPU_RNR     (*((volatile uint32_t*)(MPU_BASE + 0x08u)))  ///< Region number register
#define MPU_RBAR    (*((volatile uint32_t*)(MPU_BASE + 0x0Cu)))  ///< Region base address register
#define MPU_RASR    (*((volatile uint32_t*)(MPU_BASE + 0x10u)))  ///< Region attribute and size register

//  MPU_CTRL bits 
#define MPU_CTRL_ENABLE      (1u << 0)   ///< MPU enable
#define MPU_CTRL_HFNMIENA    (1u << 1)   ///< Enable MPU during HardFault and NMI
#define MPU_CTRL_PRIVDEFENA  (1u << 2)   ///< Privileged default memory map enable

//  MPU_TYPE fields 
#define MPU_TYPE_DREGION_MASK   0x0000FF00u  ///< Number of MPU regions
#define MPU_TYPE_DREGION_POS    8u

//  System handler control/state register (MemManage enable) 
#define SCB_SHCSR   (*((volatile uint32_t*)0xE000ED24u))
#define SCB_SHCSR_MEMFAULTENA   (1u << 16)

//  Stack guard region constants 
// Region 7 is reserved for the per-task stack guard.
//
// ARMv6-M (Cortex-M0+): minimum MPU region size is 256 bytes.
//   RASR: XN=1, AP=000, SIZE=7 (256 bytes = 2^(7+1)), ENABLE=1
//
// ARMv7-M (Cortex-M4, M7) and ARMv8-M Mainline (Cortex-M33, M55):
//   minimum MPU region size is 32 bytes.
//   RASR: XN=1, AP=000, SIZE=4 (32 bytes = 2^(4+1)), ENABLE=1
//
// RBAR bits [4:0]: VALID (bit 4) = 1, REGION (bits 3:0) = 7  0x17
// Base address must be aligned to the guard size (lower bits = 0), so OR is safe.

#define MPU_GUARD_REGION        7u

#if defined(__ARM_ARCH_6M__)
// Cortex-M0+  256-byte minimum region
#define MPU_GUARD_SIZE_BYTES    256u
// RASR: bit28=XN, bits26:24=AP=000, bits5:1=SIZE=7, bit0=EN=1
#define MPU_RASR_GUARD          0x1000000Fu
#else
// Cortex-M4, M7, M33, M55  32-byte minimum region
#define MPU_GUARD_SIZE_BYTES    32u
// RASR: bit28=XN, bits26:24=AP=000, bits5:1=SIZE=4, bit0=EN=1
#define MPU_RASR_GUARD          0x10000009u
#endif

// RBAR value: aligned base | VALID | REGION 7
#define MPU_RBAR_GUARD(base)    ((uint32_t)(base) | 0x17u)

//  MPU region count 
__STATIC_FORCEINLINE uint32_t MpuRegionCount(void) {
    return (MPU_TYPE & MPU_TYPE_DREGION_MASK) >> MPU_TYPE_DREGION_POS;
}

//  MPU initialise  call once before TaktOSStart() 
// Enables the MPU with the privileged default background map.
// Regions 06 are unconfigured (disabled). Region 7 is written per
// context switch by PendSV. Privileged code (kernel, ISRs) can still
// access any address via the background map.
__STATIC_FORCEINLINE void TaktOSMpuInit(void) {
    // Verify the device actually has an MPU before enabling.
    // Cortex-M4, M7, M33 always have one; guard against silicon variants
    // that omit it (e.g. some Cortex-M0+ configurations).
    if (MpuRegionCount() == 0u)
    {
        return;
    }

    // Disable the MPU while configuring to avoid unintended faults.
    MPU_CTRL = 0u;
    __DSB();

    // Ensure all regions start disabled. Write a dummy region-7 RASR
    // with ENABLE=0 so the guard region is safe until the first switch.
    MPU_RNR  = MPU_GUARD_REGION;
    MPU_RBAR = 0u;
    MPU_RASR = 0u;

    // Enable MPU with privileged background map.
    // PRIVDEFENA: privileged software uses the default memory map for
    // any address not covered by an enabled region  kernel and ISRs
    // retain full access. Tasks that overflow into the guard region
    // trigger a MemManage fault before the corruption propagates.
    MPU_CTRL = MPU_CTRL_PRIVDEFENA | MPU_CTRL_ENABLE;
    __DSB();
    __ISB();
}

__STATIC_FORCEINLINE void TaktOSMemFaultEnable(void) {
    SCB_SHCSR |= SCB_SHCSR_MEMFAULTENA;
    __DSB();
    __ISB();
}

//  Compute the aligned guard base for a given stack memory block 
// pMem         : start of the thread memory block (TCB base)
// tcbPlusGuard : sizeof(TaktOSThread_t) + sizeof(uint32_t)  (TCB + canary word)
//
// Returns the lowest address at or above (pMem + tcbPlusGuard) that satisfies
// the MPU region alignment for this target:
//   Cortex-M0+: 256-byte aligned  (MPU_GUARD_SIZE_BYTES = 256)
//   Cortex-M4/M7/M33/M55: 32-byte aligned  (MPU_GUARD_SIZE_BYTES = 32)
//
// Store the result in TaktOSThread_t.pStackBottom at thread creation time.
__STATIC_FORCEINLINE void *TaktMpuGuardBase(void *pMem, uint32_t tcbPlusGuard) {
    uint32_t raw  = (uint32_t)((uint8_t*)pMem + tcbPlusGuard);
    uint32_t mask = MPU_GUARD_SIZE_BYTES - 1u;
    return (void*)((raw + mask) & ~mask);
}

#endif // __TAKT_MPU_H__
