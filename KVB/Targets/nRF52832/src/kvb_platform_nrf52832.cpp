/**-------------------------------------------------------------------------
 * @file    kvb_platform_nrf52832.cpp
 *
 * @brief   KVB platform overrides for nRF52-DK (PCA10040) / nRF52832.
 *
 * This file provides ONLY the board/CPU identification strings used in
 * KVB run reports.
 *
 * Microsecond timing on this target uses the DWT cycle counter via the
 * cortex_m default platform port (KVB/ports/platforms/cortex_m/
 * kvb_platform_cortex_m.c).  Cortex-M4 has DWT/CYCCNT in the base
 * implementation — no fallback time source is required and no strong
 * override of kvb_platform_cortex_m_fallback_time_us() is provided here.
 *
 * Compare with the STM32F0308 platform file, which DOES override the
 * fallback because Cortex-M0 has no DWT — that target derives
 * microseconds from (kernel tick count, SysTick CVR).  The nRF52832
 * doesn't need any of that machinery; the DWT path runs out of the
 * cortex_m default and gives one-cycle resolution at 64 MHz (~15.6 ns).
 *
 * Log output is shared across all targets via
 * KVB/Targets/src/kvb_platform_iosonata.cpp — that file routes
 * kvb_platform_log_write() to the IOsonata UART driver g_Uart, which
 * is configured in main.cpp from the pin definitions in board.h.
 * -------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#include "nrf.h"   /* NRF_NVMC, NVMC_ICACHECNF_* — same header used by board.h */

#include "kvb_platform_port.h"
#include "kvb_config.h"

/* ----- Early NVMC instruction-cache enable ---------------------------
 *
 * The nRF52832 NVMC has an 8-line × 16-byte instruction cache that is
 * DISABLED by default after reset (Nordic nRF52832 PS v1.7, NVMC chapter,
 * ICACHECNF.CACHEEN reset value = Disabled).  At the 64 MHz core clock
 * the flash requires 1 wait state per line miss, which couples directly
 * into ISR-entry timing: SysTick lands at a different I-bus prefetcher
 * state every tick depending on what idle was doing, producing the
 * tick-cadence jitter observed in RT_TICK_JITTER_001.
 *
 * Enabling ICACHE here removes that source of variance for ALL kernel
 * ports under test on this target (TaktOS, FreeRTOS, ThreadX, Zephyr)
 * — it is the same baseline Zephyr's SoC init applies on its own runs,
 * so the cross-kernel comparison stays apples-to-apples.
 *
 * The constructor attribute makes this run before main(), so the
 * benefit is in place before any kernel starts ticking.  Idempotent:
 * the same value is written every boot and CACHEEN ignores re-writes.
 *
 * Cost: 1 register write, ~5 cycles, once at reset.  No effect on
 * deterministic-RT claims because cache misses still bound by the
 * 1-WS flash latency the application already had to tolerate.
 */
static __attribute__((constructor))
void kvb_nrf52_enable_icache(void)
{
    NRF_NVMC->ICACHECNF =
        (NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos);
}

/* ----- Identification strings ----------------------------------------- */

extern "C" const char *kvb_platform_board_name(void)
{
    return "nRF52-DK (PCA10040)";
}

extern "C" const char *kvb_platform_cpu_name(void)
{
    return "Cortex-M4F @ 64 MHz (nRF52832)";
}


/* ----- IRQ probe ------------------------------------------------------- */

extern "C" KvbStatus kvb_platform_cortex_m_irq_probe_init(uint32_t irq_n,
                                                           uint32_t priority,
                                                           KvbPlatformIrqHandler handler,
                                                           void *arg);
extern "C" KvbStatus kvb_platform_cortex_m_irq_probe_trigger(void);
extern "C" KvbStatus kvb_platform_cortex_m_irq_probe_disable(void);
extern "C" void kvb_platform_cortex_m_irq_probe_handler(void);

extern "C" KvbStatus kvb_platform_irq_probe_init(KvbPlatformIrqHandler handler, void *arg)
{
    return kvb_platform_cortex_m_irq_probe_init((uint32_t)SWI3_EGU3_IRQn,
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
    return "NVIC SWI3_EGU3";
}

extern "C" void SWI3_EGU3_IRQHandler(void)
{
    kvb_platform_cortex_m_irq_probe_handler();
}
