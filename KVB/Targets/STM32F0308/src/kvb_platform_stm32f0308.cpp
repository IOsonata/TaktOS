/**-------------------------------------------------------------------------
 * @file    kvb_platform_stm32f0308.cpp
 *
 * @brief   KVB platform overrides for STM32F0308-DISCO.
 *
 * Provides:
 *
 *   - Microsecond timing without DWT.  STM32F030 (Cortex-M0) has no
 *     DWT/CYCCNT.  This file derives microsecond timestamps from the
 *     kernel's tick counter (already incremented by the kernel's own
 *     SysTick handler) combined with the SysTick CVR register
 *     (24-bit, decrementing at CPU clock).  Resolution is one CPU
 *     cycle (~21 ns at 48 MHz).
 *
 *     This is a strong override of the weak default in
 *     KVB/ports/platforms/cortex_m/kvb_platform_cortex_m.c.
 *
 *     The tick source is dispatched via the KVB_PORT_* macro so the
 *     same per-MCU file works for both TaktOS and FreeRTOS variants
 *     without touching the kernel port API.  Adding ThreadX or Zephyr
 *     later is one extra branch each.
 *
 *   - Board / CPU identification strings used in KVB run reports.
 * -------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#include "kvb_platform_port.h"
#include "kvb_config.h"

/* ----- Kernel-agnostic tick getter ------------------------------------ */

#if defined(KVB_PORT_TAKTOS) && (KVB_PORT_TAKTOS != 0)
#  include "TaktKernel.h"
   static inline uint32_t kvb_stm32f0308_tick_count(void)
   {
       return (uint32_t)TaktOSTickCount();
   }
#elif defined(KVB_PORT_FREERTOS) && (KVB_PORT_FREERTOS != 0)
#  include "FreeRTOS.h"
#  include "task.h"
   static inline uint32_t kvb_stm32f0308_tick_count(void)
   {
       return (uint32_t)xTaskGetTickCount();
   }
#elif defined(KVB_PORT_THREADX) && (KVB_PORT_THREADX != 0)
#  include "tx_api.h"
   static inline uint32_t kvb_stm32f0308_tick_count(void)
   {
       /* tx_time_get() returns ULONG (32-bit on ARM EABI), the live
        * tick counter incremented by ThreadX's _tx_timer_interrupt
        * out of our custom SysTick_Handler in tx_initialize_low_level.S. */
       return (uint32_t)tx_time_get();
   }
#else
#  error "kvb_platform_stm32f0308.cpp: no KVB_PORT_* kernel selector defined"
#endif

/* ----- Microsecond time source (no DWT) ------------------------------- */

/* SysTick register block — same layout on all Cortex-M cores. */
#define KVB_SYST_RVR (*(volatile uint32_t *)0xE000E014u)
#define KVB_SYST_CVR (*(volatile uint32_t *)0xE000E018u)

/* Strong override of the weak default in kvb_platform_cortex_m.c.
 * Returns monotonic microseconds derived from (kernel tick count, SysTick CVR). */
extern "C" uint64_t kvb_platform_cortex_m_fallback_time_us(void)
{
    const uint32_t reload      = KVB_SYST_RVR;
    const uint32_t cpu_hz      = (uint32_t)KVB_CORE_CLOCK_HZ;
    const uint64_t us_per_tick = 1000000ull / (uint64_t)KVB_TICK_HZ;

    /* Read tick / CVR / tick to detect ISR-induced reload between
     * samples.  If the SysTick ISR fired between our two tick reads,
     * CVR has wrapped and we must retry to get a self-consistent
     * (tick, sub-tick) pair. */
    uint32_t tick1;
    uint32_t cvr;
    uint32_t tick2;

    do {
        tick1 = kvb_stm32f0308_tick_count();
        cvr   = KVB_SYST_CVR;
        tick2 = kvb_stm32f0308_tick_count();
    } while (tick1 != tick2);

    uint32_t cycles_in_tick;
    if (cvr > reload) {
        /* Defensive: should never happen in steady state, but guard
         * against early-boot calls before SysTick has been programmed
         * (kernel init configures SysTick). */
        cycles_in_tick = 0u;
    } else {
        cycles_in_tick = reload - cvr;
    }

    if (cpu_hz == 0u) {
        return (uint64_t)tick1 * us_per_tick;
    }

    return (uint64_t)tick1 * us_per_tick
         + ((uint64_t)cycles_in_tick * 1000000ull) / (uint64_t)cpu_hz;
}

/* ----- Identification strings ----------------------------------------- */

extern "C" const char *kvb_platform_board_name(void)
{
    return "STM32F0308-DISCO";
}

extern "C" const char *kvb_platform_cpu_name(void)
{
    return "Cortex-M0 @ 48 MHz (STM32F030R8)";
}
