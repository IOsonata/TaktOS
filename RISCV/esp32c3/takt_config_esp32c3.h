/**---------------------------------------------------------------------------
@file   takt_config_esp32c3.h

@brief  TaktOS compile-time configuration — ESP32-C3 bare-metal

Copy this file to your application project as takt_config.h and adjust
the values for your specific application requirements.

Naming note: this file is named takt_config_esp32c3.h in the repository
to distinguish it from other BSP configs.  Rename to takt_config.h in
your project.

ESP32-C3 hardware facts used here:
  CPU:      RISC-V RV32IMC (ESP32-C3) / RV32IMAFC+Zbb (ESP32-C6)
  Frequency: 160 MHz default (configurable 80/160 MHz via RCC)
  SYSTIMER: Fixed 80 MHz — independent of CPU clock
  IRAM:     0x4037_8000, 400 KB
  DRAM:     0x3FC8_0000, 400 KB

@author  Hoang Nguyen Hoan
@date    Apr. 2026

@license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
----------------------------------------------------------------------------*/
#ifndef __TAKT_CONFIG_H__
#define __TAKT_CONFIG_H__

// ── Architecture selection ────────────────────────────────────────────────────
// Exactly one TAKT_ARCH_* must be defined.  This is normally set as an Eclipse
// Defined Symbol (see .cproject) so this guard is a fallback.
#ifndef TAKT_ARCH_RISCV
#  define TAKT_ARCH_RISCV   1
#endif

// ── CPU clock frequency ───────────────────────────────────────────────────────
// Passed to TaktOSInit(). On ESP32-C3/C6, the SYSTIMER clock is fixed at
// 80 MHz regardless of this value, so it affects only application code that
// queries the CPU frequency.  Set to match your RCC configuration.
#define TAKT_CPU_CLOCK_HZ       160000000u  // 160 MHz default; adjust if 80 MHz

// ── Tick rate ─────────────────────────────────────────────────────────────────
// SYSTIMER period = 80_000_000 / TAKT_TICK_HZ.
// 1000 Hz → period = 80 000 (fits in 29-bit field).
// Maximum tick rate: 80 000 000 / 1 = 80 MHz (not useful; minimum is hardware limit).
#define TAKT_TICK_HZ            1000u       // 1 ms tick — standard embedded default

// ── Thread and object limits ──────────────────────────────────────────────────
// These control static table sizes — no heap involved.
// Reduce for deeply resource-constrained applications.
#define TAKT_MAX_TASKS          16u         // maximum simultaneous threads
#define TAKT_MAX_TIMERS         16u         // maximum simultaneous timers (if used)
#define TAKT_SEM_MAX_WAITERS    8u          // max threads blocked on one semaphore
#define TAKT_TIMER_TASK_PRIO    24u         // timer service task priority (TAKTOS_PRIORITY_HIGH)

// ── FreeRTOS migration shim ───────────────────────────────────────────────────
// Set to 1 to enable the FreeRTOS compatibility shim (outside cert boundary).
#define TAKT_SHIM_FREERTOS      0u

// ── Mutex priority inheritance ────────────────────────────────────────────────
// v1.0 default: always-on priority inheritance.
// Set to 0 only if you have no priority-inversion concerns and need the
// smallest possible mutex fast path.
#define TAKT_MUTEX_PI           1u

// ── Memory protection (PMP) ───────────────────────────────────────────────────
// Set to 1 to enable 32-byte no-access PMP guard at the bottom of each task
// stack.  Catches stack overflows within one tick.  Requires RISC-V PMP
// hardware (present on ESP32-C3/C6).
// Note: PMP entry 0 is used; ensure no other code configures pmpaddr0/pmpcfg0.
#ifndef TAKTOS_PMP_ENABLE
#  define TAKTOS_PMP_ENABLE     0u          // 0 = disabled (paint+check fallback)
#endif

// ── FPU context save (ESP32-C6 only) ─────────────────────────────────────────
// Set to 1 on ESP32-C6 builds to save/restore f0-f31 + fcsr on every context
// switch.  Increases frame size by 132 bytes and adds ~30 cycles/switch.
// Leave at 0 for integer-only workloads (default and typical IoT use case).
#define TAKTOS_FP_SAVE          0u

// ── Stack overflow detection mode ────────────────────────────────────────────
// 0 = paint+check (software, always available): paint guard word at
//     TaktOSThread_t + sizeof(TaktOSThread_t), check in tick ISR.
// 1 = PMP guard (hardware): enable TAKTOS_PMP_ENABLE=1 above.
// Both modes can coexist: paint+check catches overflows that slide past PMP
// granularity.  Recommended: both enabled for maximum coverage.
#define TAKTOS_STACK_OVERFLOW_DETECT  0u   // 0=paint+check, 1=PMP

// ── CPU interrupt numbers (informational — set in intmtx_esp32c3.h) ──────────
// Documenting here for project-level visibility.  Do not change without also
// updating intmtx_esp32c3.h TAKT_TICK_CPU_INT / TAKT_CTX_CPU_INT.
//   CPU INT 2   — SYSTIMER_TARGET0 tick (level, priority 2)
//   CPU INT 29  — CPU_INTR_FROM_CPU_0 ctx (edge, priority 1)

#endif // __TAKT_CONFIG_H__
