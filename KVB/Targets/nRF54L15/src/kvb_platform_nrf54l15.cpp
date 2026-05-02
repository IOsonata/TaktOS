/**-------------------------------------------------------------------------
 * @file    kvb_platform_nrf54l15.cpp
 *
 * @brief   KVB platform overrides for nRF54L15-DK / nRF54L15.
 *
 * This file provides ONLY the board/CPU identification strings used in
 * KVB run reports.
 *
 * Microsecond timing on this target uses the DWT cycle counter via the
 * cortex_m default platform port (KVB/ports/platforms/cortex_m/
 * kvb_platform_cortex_m.c).  Cortex-M33 has DWT/CYCCNT in the base
 * implementation  no fallback time source is required and no strong
 * override of kvb_platform_cortex_m_fallback_time_us() is provided here.
 *
 * Compare with the STM32F0308 platform file, which DOES override the
 * fallback because Cortex-M0 has no DWT  that target derives
 * microseconds from (kernel tick count, SysTick CVR).  The nRF54L15
 * doesn't need any of that machinery; the DWT path runs out of the
 * cortex_m default and gives one-cycle resolution at 128 MHz (~7.8 ns).
 *
 * Log output is shared across all targets via
 * KVB/Targets/src/kvb_platform_iosonata.cpp  that file routes
 * kvb_platform_log_write() to the IOsonata UART driver g_Uart, which
 * is configured in main.cpp from the pin definitions in board.h.
 * -------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#include "kvb_platform_port.h"
#include "kvb_config.h"

/* ----- Identification strings ----------------------------------------- */

extern "C" const char *kvb_platform_board_name(void)
{
    return "nRF54L15-DK";
}

extern "C" const char *kvb_platform_cpu_name(void)
{
    return "Cortex-M33 @ 128 MHz (nRF54L15)";
}
