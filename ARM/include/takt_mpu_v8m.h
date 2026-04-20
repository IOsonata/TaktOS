/**---------------------------------------------------------------------------
@file   takt_mpu_v8m.h

@brief  TaktOS ARM Cortex-M33 / M55 MPU Land layer  ARMv8-M primitives

Pure static-inline MMIO helpers for the ARMv8-M Memory Protection Unit.
Companion to takt_mpu.h (which targets ARMv6-M: M0+ and ARMv7-M: M4, M7).
No state, no function pointers, no objects  IOsonata Land idiom:
'Don't objectify the dirt.'

Supported cores:
  Cortex-M33  (ARMv8-M Mainline, up to 16 regions)
  Cortex-M55  (ARMv8.1-M Mainline, up to 16 regions)

Why a separate header from takt_mpu.h:
  ARMv8-M MPU is a different machine from ARMv7-M MPU.  Register layout,
  permission encoding, and memory-attribute model all changed:

    Item              ARMv7-M (takt_mpu.h)       ARMv8-M (this file)
    ----------------- -------------------------- -----------------------------
    Region descr.     RBAR (base) + RASR (size   RBAR (base + SH + AP + XN) +
                      encoded + TEX/C/B + AP)    RLAR (limit + AttrIndx + EN)
    Size encoding     log2-1 field in RASR       implicit: LIMIT - BASE + 1
                                                  (both fields 32-byte aligned)
    'No access' perm  AP = 000                   NOT AVAILABLE  v8-M AP is
                      (denies priv + unpriv)      2 bits only; minimum is
                                                  AP = 10 (RO-priv, no unpriv)
    Memory attrs      TEX/C/B inline in RASR     AttrIndx (3 bits in RLAR) 
                                                  MAIR0 / MAIR1 (indirect,
                                                  8 attribute slots)

TaktOS MPU stack-guard strategy on ARMv8-M:
  REGION 7 is reserved as a per-task stack guard.  The region covers
  32 bytes starting at the 32-byte-aligned floor of the task stack.

  Encoding (RBAR | RLAR):
    RBAR = base | 0x05        SH = 00, AP = 10 (RO-priv), XN = 1
    RLAR = base | 0x01        AttrIndx = 0 (MAIR0 Normal WB), EN = 1

  AP = 10 gives 'read-only by privileged code only'.  TaktOS tasks run
  privileged (CONTROL.nPRIV = 0), so the effective permissions are:
    priv write   MemManage fault     (catches stack overflow writes)
    priv read    allowed              (benign; does not occur in practice)
    unpriv any   MemManage fault     (defense in depth)
    any execute  MemManage fault     (XN = 1)

  This is strictly weaker than the v7-M 'no access' encoding on reads, but
  the stack-overflow case we want to catch is a WRITE through a descended
  SP or an indexed store  both fault.

  The MPU-aware handler variant ALSO writes PSPLIM on every switch, so the
  two mechanisms complement each other:
    PSPLIM  : fires on SP descent below pStackBottom (frame prologue
              allocating too large a local region).
    MPU #7  : fires on any write into [pStackBottom, pStackBottom + 31]
              without moving SP (frame[-n] indexed write, stale pointer
              landing in guard, alloca that exceeds budget without
              descending SP that far).

MAIR:
  This module configures MAIR0[7:0] = 0xFF (Normal memory, Inner / Outer
  Write-Back non-transient, Read- + Write-allocate).  This is the
  attribute used by RLAR.AttrIndx = 0 for the guard region.  Higher
  AttrIndx slots are left at reset value (0 = Device-nGnRnE) and are
  not used by TaktOS.  The attribute does not affect fault behaviour
  on a write into the guard  the permission check (AP = 10) fires first.

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
#ifndef __TAKT_MPU_V8M_H__
#define __TAKT_MPU_V8M_H__

#include <stdint.h>
#include "cmsis_compiler.h"

//  MPU register base (identical to ARMv7-M) 
#define MPU_BASE            0xE000ED90u

//  MPU registers (ARMv8-M layout) 
#define MPU_TYPE    (*((volatile uint32_t*)(MPU_BASE + 0x00u)))  ///< Type register
#define MPU_CTRL    (*((volatile uint32_t*)(MPU_BASE + 0x04u)))  ///< Control register
#define MPU_RNR     (*((volatile uint32_t*)(MPU_BASE + 0x08u)))  ///< Region number register
#define MPU_RBAR    (*((volatile uint32_t*)(MPU_BASE + 0x0Cu)))  ///< Region base (+ SH + AP + XN)
#define MPU_RLAR    (*((volatile uint32_t*)(MPU_BASE + 0x10u)))  ///< Region limit (+ AttrIndx + EN)
#define MPU_MAIR0   (*((volatile uint32_t*)(MPU_BASE + 0x30u)))  ///< Attribute indirection 0 (AttrIndx 0..3)
#define MPU_MAIR1   (*((volatile uint32_t*)(MPU_BASE + 0x34u)))  ///< Attribute indirection 1 (AttrIndx 4..7)

//  MPU_CTRL bits (identical to ARMv7-M) 
#define MPU_CTRL_ENABLE      (1u << 0)   ///< MPU enable
#define MPU_CTRL_HFNMIENA    (1u << 1)   ///< Enable MPU during HardFault and NMI
#define MPU_CTRL_PRIVDEFENA  (1u << 2)   ///< Privileged default memory map enable

//  MPU_TYPE fields (identical layout across v7-M / v8-M) 
#define MPU_TYPE_DREGION_MASK   0x0000FF00u  ///< Number of MPU regions
#define MPU_TYPE_DREGION_POS    8u

//  System handler control/state register (MemManage enable) 
#define SCB_SHCSR   (*((volatile uint32_t*)0xE000ED24u))
#define SCB_SHCSR_MEMFAULTENA   (1u << 16)

//  Stack guard region constants 
// Region 7 is reserved for the per-task stack guard.
// ARMv8-M minimum region size is 32 bytes  same granularity as ARMv7-M.
//
// The assembly handlers (PendSV_Handler_MPU / SVC_Handler_MPU in
// cm33/PendSV_M33.S and cm55/PendSV_M55.S) encode the region number, the
// RNR address, and the RBAR / RLAR constants literally.  Keep those in
// sync with the macros below.

#define MPU_GUARD_REGION        7u
#define MPU_GUARD_SIZE_BYTES    32u

// RBAR encoding for a 32-byte stack guard at 32-byte-aligned 'base':
//   [31:5] BASE        = base[31:5]
//   [4:3]  SH          = 00   (Non-shareable)
//   [2:1]  AP          = 10   (Read-only, privileged; unprivileged = no access)
//   [0]    XN          = 1    (No execute)
//  Low 3 bits combined = 0b101 = 0x05.  (Low 5 bits of base are zero by
//  alignment, so ORR with 0x05 sets exactly SH / AP / XN with no masking.)
#define MPU_V8M_RBAR_GUARD(base)    ((uint32_t)(base) | 0x05u)

// RLAR encoding for a 32-byte stack guard at 32-byte-aligned 'base':
//   [31:5] LIMIT       = last-byte-address[31:5].  For a 32-byte region at
//                        aligned base, last_byte = base + 31; low 5 bits
//                        of the stored LIMIT are implied 0b11111 on read.
//                        For a 32-byte region the stored LIMIT field
//                        happens to equal the stored BASE field (both
//                        are base[31:5]).
//   [4:1]  AttrIndx    = 0000  (index 0 in MAIR0  Normal WB, set by
//                               TaktOSMpuInit below)
//   [0]    EN          = 1
//  Low 5 bits combined = 0b00001 = 0x01.
#define MPU_V8M_RLAR_GUARD(base)    ((uint32_t)(base) | 0x01u)

// MAIR attribute 0: Normal memory, Inner/Outer Write-Back non-transient,
// Read- + Write-allocate  attribute byte value 0xFF.  Stored in MAIR0[7:0].
// Higher MAIR slots are unused by TaktOS and remain at reset (Device-nGnRnE).
#define MPU_V8M_MAIR0_ATTR0_NORMAL_WB   0x000000FFu


//  MPU region count 
// Returns the number of MPU regions implemented on this part.  M33 and M55
// are specified to include an MPU, but MPU_TYPE is the authoritative source
// and guards against silicon variants that might omit it.
__STATIC_FORCEINLINE uint32_t MpuRegionCount(void) {
    return (MPU_TYPE & MPU_TYPE_DREGION_MASK) >> MPU_TYPE_DREGION_POS;
}

//  MPU initialise  call once before TaktOSStart() 
// Enables the MPU with the privileged default background map and configures
// MAIR0[7:0] as Normal WB cacheable memory for AttrIndx = 0.
// Regions 06 are left unconfigured (disabled).  Region 7 is pre-disabled
// here and then written per context switch by PendSV_Handler_MPU, so the
// guard is inert until the first switch writes a valid base.
//
// Privileged code (kernel, ISRs) retains full access via PRIVDEFENA = 1 to
// any address not covered by an enabled region.  A privileged write into
// an enabled region is checked against that region's AP bits and will
// fault if AP denies the access  this is what catches stack overflow.
__STATIC_FORCEINLINE void TaktOSMpuInit(void) {
    if (MpuRegionCount() == 0u)
    {
        return;
    }

    // Disable the MPU while configuring.
    MPU_CTRL = 0u;
    __DSB();

    // Configure AttrIndx = 0 as Normal WB cacheable memory.
    MPU_MAIR0 = MPU_V8M_MAIR0_ATTR0_NORMAL_WB;

    // Pre-disable region 7 so the guard is inert until the first context
    // switch writes a valid base.  Writing EN = 0 in RLAR is the disable.
    MPU_RNR  = MPU_GUARD_REGION;
    MPU_RBAR = 0u;
    MPU_RLAR = 0u;

    // Enable MPU with privileged background map.
    MPU_CTRL = MPU_CTRL_PRIVDEFENA | MPU_CTRL_ENABLE;
    __DSB();
    __ISB();
}

//  Enable MemManage fault (identical to v7-M) 
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
// the ARMv8-M MPU 32-byte region alignment.
//
// Store the result in TaktOSThread_t.pStackBottom at thread creation time.
__STATIC_FORCEINLINE void *TaktMpuGuardBase(void *pMem, uint32_t tcbPlusGuard) {
    uint32_t raw  = (uint32_t)((uint8_t*)pMem + tcbPlusGuard);
    uint32_t mask = MPU_GUARD_SIZE_BYTES - 1u;
    return (void*)((raw + mask) & ~mask);
}

#endif // __TAKT_MPU_V8M_H__
