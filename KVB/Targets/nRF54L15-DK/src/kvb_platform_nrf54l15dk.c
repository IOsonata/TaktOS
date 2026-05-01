/* SPDX-License-Identifier: MIT
 *
 * KVB platform overrides for nRF54L15-DK on Zephyr.
 *
 * Provides:
 *
 *   - Microsecond timing.  nRF54L15 is Cortex-M33 (ARMv8-M Mainline)
 *     and DOES have DWT/CYCCNT, but routing time through Zephyr's
 *     own k_cycle_get_32() / k_cyc_to_us_floor64() pair is the only
 *     way to keep the cycle reference consistent with however Zephyr
 *     has configured its system clock (GRTC or otherwise).  Using the
 *     Zephyr APIs also matches the way an application written to
 *     target Zephyr would naturally read time, so the benchmark
 *     numbers reflect what a real Zephyr app sees.
 *
 *   - Log output routed through Zephyr's printk, which goes to the
 *     console UART (uart30 on the nRF54L15-DK by default).  KVB writes
 *     short, fully-formatted lines; printk handles framing.
 *
 *   - Board / CPU identification strings used in KVB run reports.
 *
 * Strong overrides of the weak defaults declared in
 * KVB/src/core/kvb_platform_default.c.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "kvb_platform_port.h"
#include "kvb_config.h"

/* ----- Cycle counter (Zephyr-managed) -------------------------------- */

uint32_t kvb_platform_cycle_frequency_hz(void)
{
    /* sys_clock_hw_cycles_per_sec() reflects Zephyr's configured cycle
     * source.  On nRF54L15 with the GRTC clocking the system tick, this
     * returns the GRTC frequency; with DWT, it returns SystemCoreClock.
     * KVB consumers only need a stable Hz value to convert cycles to
     * microseconds, so any consistent source is fine. */
    return (uint32_t)sys_clock_hw_cycles_per_sec();
}

uint64_t kvb_platform_cycle_count(void)
{
    /* k_cycle_get_32() returns the live hardware cycle counter.  On
     * targets with a 32-bit underlying source it wraps every 2^32 cycles;
     * KVB throughput tests only ever measure intervals well below that
     * window, so a 32-bit return is sufficient.  Promote to 64-bit for
     * the framework's signature. */
    return (uint64_t)k_cycle_get_32();
}

uint64_t kvb_platform_time_us(void)
{
    /* k_cyc_to_us_floor64() handles cycles-to-microseconds with full
     * 64-bit width and Zephyr-configured frequency.  This is the
     * canonical "what time is it" call inside a Zephyr application. */
    return k_cyc_to_us_floor64(k_cycle_get_32());
}

/* ----- Log output ----------------------------------------------------- */

void kvb_platform_log_write(const char *data, size_t len)
{
    if (data == NULL || len == 0u) {
        return;
    }

    /* Zephyr's printk("%.*s", ...) routes the bytes to the console
     * UART driver registered via CONFIG_UART_CONSOLE=y.  The KVB log
     * lines are already fully formatted with their trailing newline,
     * so no extra framing is added here. */
    printk("%.*s", (int)len, data);
}

void kvb_platform_log_flush(void)
{
    /* printk is line-buffered at the driver level on Zephyr's UART
     * console; nothing further required.  The function is provided so
     * that future ports gating on a buffered backend can override it. */
}

/* ----- Identification strings ---------------------------------------- */

const char *kvb_platform_board_name(void)
{
    return "nRF54L15-DK";
}

const char *kvb_platform_cpu_name(void)
{
    return "Cortex-M33 @ 128 MHz (nRF54L15)";
}
