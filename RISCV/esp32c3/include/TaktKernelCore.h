#ifndef __TAKTKERNELCORE_ESP32C3_WRAPPER_H__
#define __TAKTKERNELCORE_ESP32C3_WRAPPER_H__
#include <stdint.h>

#include "TaktOS.h"
#include "takt_riscv_port.h"
#include "../../include/TaktKernelCore.h"

/**
 * @brief  Convert a TaktOS standard priority to the ESP32-C3 INTMTX encoding.
 *
 * TaktOS scale: TAKTOS_PRIORITY_LOWEST(1) .. TAKTOS_PRIORITY_CRITICAL(31),
 * higher value = more urgent.  The ESP32-C3 interrupt matrix uses 1..7 in the
 * same direction (higher = more urgent, 0 = line disabled), so the conversion
 * is a rescale with no inversion:
 *
 *     TAKTOS_PRIORITY_LOWEST   (1)  -> 1
 *     TAKTOS_PRIORITY_NORMAL   (16) -> 4
 *     TAKTOS_PRIORITY_CRITICAL (31) -> 7
 *
 * TAKTOS_TICK_PRIORITY_DEFAULT (0) returns TAKT_TICK_CPU_INT_PRIORITY, the
 * port default, so the default path needs no special case.  Input above the
 * scale is clamped to 7.
 *
 * @param  Prio : TaktOS standard priority, 1..31, or 0 for the port default.
 * @return INTMTX CPU-interrupt priority, 1..7.
 */
static inline uint32_t TaktArchTickPrioMap(uint32_t Prio)
{
    if (Prio == TAKTOS_TICK_PRIORITY_DEFAULT)
    {
        // Must track TAKT_TICK_CPU_INT_PRIORITY in src/intmtx_esp32c3.h,
        // which is a src-side register header and not includable from here.
        return 2u;
    }

    if (Prio >= TAKTOS_PRIORITY_CRITICAL)
    {
        return 7u;                          // clamp - most urgent INTMTX level
    }

    return 1u + (((Prio - TAKTOS_PRIORITY_LOWEST) * 6u)
                 / (TAKTOS_PRIORITY_CRITICAL - TAKTOS_PRIORITY_LOWEST));
}

#endif
