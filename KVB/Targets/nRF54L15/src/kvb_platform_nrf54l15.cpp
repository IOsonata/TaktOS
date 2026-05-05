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

#include "nrf.h"

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


/* ----- IRQ probe ------------------------------------------------------- */

/*
 * Use the same spare software interrupt selected by the nRF54L15
 * Thread-Metric board support: SWI00, IRQ number 28.
 *
 * Do not use EGU10_IRQn here. Some Nordic MDK/header combinations used by
 * the IOsonata nRF54L15 Eclipse projects do not expose EGU10_IRQn, while
 * SWI00_IRQHandler and IRQ 28 are already used successfully by the existing
 * nRF54L15 benchmark target.
 */
#define KVB_NRF54L15_IRQ_PROBE_IRQ_N 28u

extern "C" KvbStatus kvb_platform_cortex_m_irq_probe_init(uint32_t irq_n,
                                                           uint32_t priority,
                                                           KvbPlatformIrqHandler handler,
                                                           void *arg);
extern "C" KvbStatus kvb_platform_cortex_m_irq_probe_trigger(void);
extern "C" KvbStatus kvb_platform_cortex_m_irq_probe_disable(void);
extern "C" void kvb_platform_cortex_m_irq_probe_handler(void);

extern "C" KvbStatus kvb_platform_irq_probe_init(KvbPlatformIrqHandler handler, void *arg)
{
    return kvb_platform_cortex_m_irq_probe_init(KVB_NRF54L15_IRQ_PROBE_IRQ_N,
                                                KVB_IRQ_PROBE_PRIORITY,
                                                handler,
                                                arg);
}

extern "C" KvbStatus kvb_platform_irq_probe_trigger(void)
{
    return kvb_platform_cortex_m_irq_probe_trigger();
}

extern "C" KvbStatus kvb_platform_irq_probe_disable(void)
{
    return kvb_platform_cortex_m_irq_probe_disable();
}

extern "C" const char *kvb_platform_irq_probe_name(void)
{
    return "NVIC SWI00";
}

extern "C" void SWI00_IRQHandler(void)
{
    kvb_platform_cortex_m_irq_probe_handler();
}
