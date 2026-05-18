/**---------------------------------------------------------------------------
@file   TaktKernelCore.h

@brief  Generic RISC-V kernel core interface for TaktOS

Computes the common thread-stack sizing macros for RISC-V ports and pulls in
TaktOSCriticalSection.h for the interrupt and deferred context-switch
primitives needed by TaktKernel.h.

Arch-level values (frame size, alignment, guard granularity) come from the
arch lib's port header (port_rv32.h, port_rv32f.h, ...).  Selected at compile
time based on the FP ABI advertised by the toolchain.

The HAL is not involved here.  HAL-side glue is supplied through extern
declarations in TaktOSCriticalSection.h (TaktOSCtxSwitch, TaktOSStartFirst).
----------------------------------------------------------------------------*/
#ifndef __TAKTKERNELCORE_H__
#define __TAKTKERNELCORE_H__

/* Arch-level values (frame size, alignment, guard granularity) come from the
 * arch lib's port header.  For RV32, port_rv32.h handles all three ABIs
 * (ilp32 / ilp32f / ilp32d) internally via __riscv_flen.  A future RV64
 * arch lib would add its own conditional include here. */
#include "port_rv32.h"

/* Port-specific default for TaktOSCfg_t.SoftIntAddr.
 * Used by TaktOSInit() when the application leaves the field 0.
 * RV32 default = standard CLINT MSIP base, per the RISC-V Privileged Spec
 * reference (SiFive E series, Nuclei N100/N200/N300, BL616, Renesas
 * R9A02G021, ESP32-C6/H2, etc.).  Chips with CLINT at a non-standard base
 * (GD32VF103 at 0xD1000000) or no CLINT at all (ESP32-C3 - uses
 * SYSTEM_CPU_INTR_FROM_CPU_0 at 0x600C0014) supply the value explicitly in
 * cfg.SoftIntAddr. */
#define TAKT_DEFAULT_SOFT_INT_ADDR     0x02000000u

#define TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD                        \
    (sizeof(TaktOSThread_t) + sizeof(uint32_t) +                   \
     TAKT_RISCV_THREAD_INIT_FRAME_SIZE +                           \
     TAKT_RISCV_THREAD_INIT_ALIGN_SLACK)

#define TAKTOS_STACK_GUARD_ALIGN       TAKT_RISCV_STACK_GUARD_ALIGN
#define TAKTOS_THREAD_INIT_FRAME_SIZE  TAKT_RISCV_THREAD_INIT_FRAME_SIZE
#define TAKTOS_THREAD_INIT_ALIGN_SLACK TAKT_RISCV_THREAD_INIT_ALIGN_SLACK

/* Idle-thread usable payload above the reserved frame/guard overhead. */
#define TAKTOS_IDLE_THREAD_STACKSIZE   16u

#include "TaktOSCriticalSection.h"

#endif /* __TAKTKERNELCORE_H__ */
