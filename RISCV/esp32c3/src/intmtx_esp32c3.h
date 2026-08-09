/**---------------------------------------------------------------------------
@file   intmtx_esp32c3.h

@brief  ESP32-C3 / ESP32-C6 Interrupt Matrix (INTERRUPT_CORE0) utilities

Land-layer primitives for the ESP32-C3 interrupt matrix.

The interrupt matrix maps each of 62 peripheral interrupt sources to one of
32 CPU interrupt lines (CPU INT 0–31).  Each CPU INT line has its own:
  - Type:      level or edge
  - Priority:  0–7 (0 = disabled, higher = higher priority)

TaktOS CPU INT allocation (set up by TaktOSTickInit):

  CPU INT 2  — SYSTIMER_TARGET0 tick interrupt
                level-triggered, priority 2
  CPU INT 29 — SYSTEM_CPU_INTR_FROM_CPU_0 (deferred context switch)
                edge-triggered, priority 1

  Interrupt threshold = 0 → both CPU INTs fire.

Priority ordering ensures tick fires before context switch when both pend
simultaneously, matching the ARM SysTick → PendSV ordering exactly.

Peripheral source numbers (ESP32-C3 TRM Table 4-3):
  ETS_FROM_CPU_INTR0_SOURCE         = 0   (CPU_INTR_FROM_CPU_0)
  ETS_SYSTIMER_TARGET0_INTR_SOURCE  = 37  (SYSTIMER comparator 0)

Register base: 0x600C2000 (INTERRUPT_CORE0_BASE)

All functions are pure static-inline — no state, no allocations.

ESP32-C6 note:
  The interrupt matrix base address and register layout are identical on
  ESP32-C6.  Peripheral source numbers differ for some peripherals but the
  two sources used here (0 and 37) are the same on both chips.

@author  Hoang Nguyen Hoan
@date    Apr. 2026

@license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
----------------------------------------------------------------------------*/
#ifndef __INTMTX_ESP32C3_H__
#define __INTMTX_ESP32C3_H__

#include <stdint.h>

// ── Interrupt matrix base ─────────────────────────────────────────────────────
#define INTMTX_BASE                 0x600C2000u

// ── Peripheral source → CPU INT mapping registers ─────────────────────────────
// Each register maps one peripheral source to CPU INT [4:0].
// Register for source N = INTMTX_BASE + N * 4.
#define INTMTX_MAP_REG(src)         (INTMTX_BASE + (uint32_t)(src) * 4u)

// Peripheral source numbers used by TaktOS
#define INTMTX_SRC_CPU_INTR_FROM0  0u   // SYSTEM_CPU_INTR_FROM_CPU_0 (ctx switch)
#define INTMTX_SRC_SYSTIMER_T0     37u  // SYSTIMER TARGET0 (tick)

// ── CPU INT control registers ─────────────────────────────────────────────────
#define INTMTX_CPU_INT_ENABLE_REG   (INTMTX_BASE + 0x104u)  // enable bits [31:0]
#define INTMTX_CPU_INT_TYPE_REG     (INTMTX_BASE + 0x108u)  // 0=level, 1=edge
#define INTMTX_CPU_INT_EIP_REG      (INTMTX_BASE + 0x10Cu)  // pending status (read-only)
#define INTMTX_CPU_INT_PRI_BASE     (INTMTX_BASE + 0x114u)  // PRI_0..PRI_31 @ +0..+31*4
#define INTMTX_CPU_INT_THRESH_REG   (INTMTX_BASE + 0x154u)  // threshold [3:0]

// Priority register for CPU INT N = INTMTX_CPU_INT_PRI_BASE + N*4
#define INTMTX_CPU_INT_PRI_REG(n)   (INTMTX_CPU_INT_PRI_BASE + (uint32_t)(n) * 4u)

// ── TaktOS CPU INT assignments ────────────────────────────────────────────────
#define TAKT_TICK_CPU_INT           2u    // SYSTIMER_TARGET0 → CPU INT 2
#define TAKT_CTX_CPU_INT            29u   // CPU_INTR_FROM_CPU_0 → CPU INT 29
#define TAKT_TICK_CPU_INT_PRIORITY  2u
#define TAKT_CTX_CPU_INT_PRIORITY   1u

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Map a peripheral interrupt source to a CPU interrupt line.
 *
 * @param  src    Peripheral source number (0–61, see INTMTX_SRC_* defines).
 * @param  cpuInt Target CPU interrupt number (0–31).
 */
static inline void IntMtxMapSrc(uint32_t src, uint32_t cpuInt)
{
    *((volatile uint32_t*)INTMTX_MAP_REG(src)) = cpuInt & 0x1Fu;
}

/**
 * @brief  Enable a CPU interrupt line.
 *
 * @param  cpuInt  CPU interrupt number (0–31).
 */
static inline void IntMtxCpuIntEnable(uint32_t cpuInt)
{
    *((volatile uint32_t*)INTMTX_CPU_INT_ENABLE_REG) |= (1u << cpuInt);
}

/**
 * @brief  Set the type of a CPU interrupt line.
 *
 * @param  cpuInt  CPU interrupt number (0–31).
 * @param  edge    true = edge-triggered, false = level-triggered.
 */
static inline void IntMtxCpuIntSetType(uint32_t cpuInt, bool edge)
{
    volatile uint32_t *reg = (volatile uint32_t*)INTMTX_CPU_INT_TYPE_REG;
    if (edge)
    {
        *reg |=  (1u << cpuInt);
    }
    else
    {
        *reg &= ~(1u << cpuInt);
    }
}

/**
 * @brief  Set the priority of a CPU interrupt line.
 *
 * @param  cpuInt    CPU interrupt number (0–31).
 * @param  priority  Priority value 1–7.  0 = disabled.
 *                   Higher value = higher priority.
 */
static inline void IntMtxCpuIntSetPriority(uint32_t cpuInt, uint32_t priority)
{
    *((volatile uint32_t*)INTMTX_CPU_INT_PRI_REG(cpuInt)) = priority & 0x7u;
}

/**
 * @brief  Set the interrupt priority threshold.
 *
 * CPU interrupts with priority ≤ threshold are masked.
 * Set to 0 to allow all active CPU interrupts.
 *
 * @param  threshold  Priority threshold (0–7).
 */
static inline void IntMtxSetThreshold(uint32_t threshold)
{
    *((volatile uint32_t*)INTMTX_CPU_INT_THRESH_REG) = threshold & 0x7u;
}

/**
 * @brief  Read the CPU interrupt pending status register.
 *
 * Each set bit indicates the corresponding CPU interrupt is currently
 * pending and above the priority threshold.
 *
 * @return  Bitmask of pending CPU interrupt lines.
 */
static inline uint32_t IntMtxGetEipStatus(void)
{
    return *((volatile uint32_t*)INTMTX_CPU_INT_EIP_REG);
}

/**
 * @brief  Configure both TaktOS CPU interrupts (tick + ctx switch).
 *
 * Called once from TaktOSTickInit() to set up the full interrupt routing:
 *   SYSTIMER_TARGET0 → CPU INT 2,  level,  priority TickPriority
 *   CPU_INTR_FROM_CPU0 → CPU INT 29, edge, priority 1
 *   Threshold = 0
 * Machine external interrupt enable (mie.MEIE) must be set by the caller.
 *
 * @param  TickPriority  INTMTX priority for the tick CPU INT line, 1..7,
 *                       higher value = higher priority.  This is the native
 *                       hardware value — the caller has already mapped
 *                       TaktOSCfg_t.TickPriority off the TaktOS standard
 *                       scale (see TaktArchTickPrioMap in TaktOSTickInit).
 */
static inline void IntMtxTaktOSInit(uint32_t TickPriority)
{
    uint32_t tickPrio = TickPriority & 0x7u;

    // Map peripheral sources to CPU INT lines
    IntMtxMapSrc(INTMTX_SRC_SYSTIMER_T0,    TAKT_TICK_CPU_INT);
    IntMtxMapSrc(INTMTX_SRC_CPU_INTR_FROM0, TAKT_CTX_CPU_INT);

    // Configure types
    IntMtxCpuIntSetType(TAKT_TICK_CPU_INT, false);  // level
    IntMtxCpuIntSetType(TAKT_CTX_CPU_INT,  true);   // edge

    // Configure priorities
    IntMtxCpuIntSetPriority(TAKT_TICK_CPU_INT, tickPrio);
    IntMtxCpuIntSetPriority(TAKT_CTX_CPU_INT,  TAKT_CTX_CPU_INT_PRIORITY);

    // Enable both CPU INT lines
    IntMtxCpuIntEnable(TAKT_TICK_CPU_INT);
    IntMtxCpuIntEnable(TAKT_CTX_CPU_INT);

    // Allow all priorities ≥ 1
    IntMtxSetThreshold(0u);
}

#ifdef __cplusplus
}
#endif

#endif // __INTMTX_ESP32C3_H__
