/**---------------------------------------------------------------------------
@file   systimer_esp32c3.h

@brief  ESP32-C3 / ESP32-C6 SYSTIMER utility functions

Land-layer primitives for the ESP32-C3 SYSTIMER peripheral.
SYSTIMER provides three 52-bit counters and three alarm comparators.
TaktOS uses:  Unit 0 for the free-running time reference.
              Comparator 0 (TARGET0) in PERIOD mode for the tick interrupt.

SYSTIMER clock:
  Fixed at 80 MHz (12.5 ns resolution) regardless of CPU clock.
  PERIOD value = 80_000_000 / TAKT_TICK_HZ.
  For TAKT_TICK_HZ = 1000: period = 80 000 (fits in 28 bits, well under 30-bit limit).

Register base: 0x60023000 (ESP32-C3 TRM §10 / ESP32-C6 TRM §10)

SYSTIMER_TARGET0_CONF_REG [PERIOD_MODE = bit 31]:
  0 = target mode   (fires once when UNIT0 value reaches TARGET0)
  1 = period mode   (fires every PERIOD ticks; TaktOS always uses period mode)

All functions are pure static-inline — no state, no side effects beyond MMIO.

ESP32-C6 note:
  Register layout and base address are identical.  The SYSTIMER clock is
  also 80 MHz on ESP32-C6.  No code change is required for C6 support.

@author  Hoang Nguyen Hoan
@date    Apr. 2026

@license  MIT License — Copyright (c) 2026 I-SYST inc.  All rights reserved.
----------------------------------------------------------------------------*/
#ifndef __SYSTIMER_ESP32C3_H__
#define __SYSTIMER_ESP32C3_H__

#include <stdint.h>
#include <stdbool.h>

// ── Register base ─────────────────────────────────────────────────────────────
#define SYSTIMER_BASE               0x60023000u

// ── SYSTIMER registers ────────────────────────────────────────────────────────
// Offsets from SYSTIMER_BASE (ESP32-C3 TRM Table 10-1)
#define SYSTIMER_CONF_REG           (SYSTIMER_BASE + 0x000u)  // clk_en, stall enables
#define SYSTIMER_UNIT0_OP_REG       (SYSTIMER_BASE + 0x004u)  // [30]=force update timer value
#define SYSTIMER_TARGET0_HI_REG     (SYSTIMER_BASE + 0x014u)  // comparator 0 high [19:0]
#define SYSTIMER_TARGET0_LO_REG     (SYSTIMER_BASE + 0x018u)  // comparator 0 low  [31:0]
#define SYSTIMER_TARGET0_CONF_REG   (SYSTIMER_BASE + 0x02Cu)  // [31]=period_mode, [28:0]=period
#define SYSTIMER_COMP0_LOAD_REG     (SYSTIMER_BASE + 0x038u)  // [0]=load comparator 0
#define SYSTIMER_INT_ENA_REG        (SYSTIMER_BASE + 0x04Cu)  // [0]=TARGET0 enable
#define SYSTIMER_INT_RAW_REG        (SYSTIMER_BASE + 0x050u)  // raw interrupt status
#define SYSTIMER_INT_CLR_REG        (SYSTIMER_BASE + 0x054u)  // [0]=TARGET0 clear
#define SYSTIMER_INT_ST_REG         (SYSTIMER_BASE + 0x058u)  // masked interrupt status
#define SYSTIMER_UNIT0_VALUE_HI_REG (SYSTIMER_BASE + 0x05Cu)  // UNIT0 current value hi (latched)
#define SYSTIMER_UNIT0_VALUE_LO_REG (SYSTIMER_BASE + 0x060u)  // UNIT0 current value lo (latched)

// ── Bit masks ─────────────────────────────────────────────────────────────────
#define SYSTIMER_CONF_CLK_EN        (1u << 31)  // enable SYSTIMER clock
#define SYSTIMER_UNIT0_UPDATE       (1u << 30)  // latch UNIT0 current value
#define SYSTIMER_TARGET0_PERIOD_EN  (1u << 31)  // period mode (vs target-match)
#define SYSTIMER_TARGET0_PERIOD_MSK 0x1FFFFFFFu  // period value [28:0]
#define SYSTIMER_INT_TARGET0        (1u <<  0)  // TARGET0 interrupt bit

// ── Clock frequency ───────────────────────────────────────────────────────────
// Fixed 80 MHz on both ESP32-C3 and ESP32-C6.
#define SYSTIMER_CLK_HZ             80000000u

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Enable the SYSTIMER clock gate.
 *
 * Must be called before any other SYSTIMER operation.
 */
static inline void SysTimerClkEnable(void)
{
    *((volatile uint32_t*)SYSTIMER_CONF_REG) |= SYSTIMER_CONF_CLK_EN;
}

/**
 * @brief  Enable the TARGET0 alarm interrupt.
 */
static inline void SysTimerEnableInt(void)
{
    *((volatile uint32_t*)SYSTIMER_INT_ENA_REG) |= SYSTIMER_INT_TARGET0;
}

/**
 * @brief  Disable the TARGET0 alarm interrupt.
 */
static inline void SysTimerDisableInt(void)
{
    *((volatile uint32_t*)SYSTIMER_INT_ENA_REG) &= ~SYSTIMER_INT_TARGET0;
}

/**
 * @brief  Clear the TARGET0 interrupt flag.
 *
 * Must be called at the start of the tick ISR to prevent immediate re-entry.
 */
static inline void SysTimerClearInt(void)
{
    *((volatile uint32_t*)SYSTIMER_INT_CLR_REG) = SYSTIMER_INT_TARGET0;
}

/**
 * @brief  Return true if the TARGET0 interrupt is pending (masked status).
 */
static inline bool SysTimerIsPending(void)
{
    return (*((volatile uint32_t*)SYSTIMER_INT_ST_REG) & SYSTIMER_INT_TARGET0) != 0u;
}

/**
 * @brief  Configure TARGET0 in period mode and load the comparator.
 *
 * Sets TARGET0 to fire every @p periodCycles SYSTIMER clock cycles.
 * SYSTIMER runs at 80 MHz; for 1 kHz tick set periodCycles = 80 000.
 *
 * @param  periodCycles  Period in 80-MHz SYSTIMER clocks.  Max 2^29-1.
 *                       Compute: SYSTIMER_CLK_HZ / TAKT_TICK_HZ.
 */
static inline void SysTimerSetPeriod(uint32_t periodCycles)
{
    // Period mode: bit 31 = 1; period value in bits [28:0].
    *((volatile uint32_t*)SYSTIMER_TARGET0_CONF_REG) =
        SYSTIMER_TARGET0_PERIOD_EN | (periodCycles & SYSTIMER_TARGET0_PERIOD_MSK);

    // Latch the new comparator value into the hardware comparator.
    *((volatile uint32_t*)SYSTIMER_COMP0_LOAD_REG) = 1u;
}

/**
 * @brief  Read and latch the current UNIT0 value (low 32 bits only).
 *
 * The caller must write bit 30 of UNIT0_OP_REG to force a latch first.
 * Returns the low 32 bits of the 52-bit counter (sufficient for short
 * delta measurements well under 53 seconds at 80 MHz).
 *
 * @return  Current UNIT0 lo value in 80-MHz ticks.
 */
static inline uint32_t SysTimerGetCount(void)
{
    // Force-latch the current counter value into the read registers.
    *((volatile uint32_t*)SYSTIMER_UNIT0_OP_REG) = SYSTIMER_UNIT0_UPDATE;
    return *((volatile uint32_t*)SYSTIMER_UNIT0_VALUE_LO_REG);
}

/**
 * @brief  Configure SYSTIMER for a given tick frequency and start the alarm.
 *
 * Enables the SYSTIMER clock, sets period mode on TARGET0, and enables
 * the TARGET0 interrupt.  Does NOT route the interrupt through the interrupt
 * matrix — that is done by TaktOSTickInit().
 *
 * @param  TickHz  Desired tick interrupt rate in Hz (e.g. 1000 for 1 ms).
 * @return  Achieved tick frequency in Hz (may differ due to integer division).
 */
static inline uint32_t SysTimerSetFrequency(uint32_t TickHz)
{
    if (TickHz == 0u)
    {
        return 0u;
    }

    SysTimerClkEnable();

    uint32_t period = SYSTIMER_CLK_HZ / TickHz;

    // Safety: period must fit in 29-bit field and be non-zero.
    if (period == 0u || period > SYSTIMER_TARGET0_PERIOD_MSK)
    {
        return 0u;
    }

    SysTimerSetPeriod(period);
    SysTimerClearInt();       // clear any stale pending flag
    SysTimerEnableInt();

    return SYSTIMER_CLK_HZ / period;  // actual achieved frequency
}

#ifdef __cplusplus
}
#endif

#endif // __SYSTIMER_ESP32C3_H__
