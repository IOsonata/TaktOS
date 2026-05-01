/**-------------------------------------------------------------------------
 * @file    kvb_platform_iosonata.cpp
 *
 * @brief   KVB platform overrides — IOsonata UART, no retarget.
 *
 * Provides:
 *   - kvb_platform_log_write()    routed to g_Uart.Tx() with retry loop
 *   - kvb_platform_log_flush()    no-op (IOsonata drains the FIFO async)
 *
 * IOsonata UART::Tx contract:
 *   The call accepts as many bytes as currently fit in the TX FIFO and
 *   returns the byte count actually accepted (which can be less than
 *   `len`).  The driver drains the FIFO asynchronously through DMA + the
 *   UART IRQ.  If the producer does not loop on the remainder, the
 *   un-accepted bytes are silently dropped — which on KVB shows up as
 *   log lines getting truncated mid-output.  This implementation loops
 *   until every byte is accepted, yielding to the scheduler each pass
 *   so the IRQ can drain the FIFO between attempts.
 *
 * Identification strings and the no-DWT time fallback are supplied per-MCU
 * by kvb_platform_<mcu>.cpp, since they need board-specific values.
 *
 * The C++ UART object g_Uart is defined in main.cpp.
 * -------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>

#include "coredev/uart.h"

#include "kvb_config.h"
#include "kvb_platform_port.h"
#include "kvb_kernel_port.h"

extern UART g_Uart;

static void kvb_io_delay_ms(uint32_t ms)
{
    /* Best-effort delay only for UART settling/drain.  Not used for benchmark
     * timing, so the approximate loop calibration is sufficient. */
    uint32_t loops_per_ms = KVB_CORE_CLOCK_HZ / 8000u;

    if (loops_per_ms == 0u) {
        loops_per_ms = 1u;
    }

    uint32_t loops = loops_per_ms * ms;
    while (loops-- > 0u) {
#if defined(__GNUC__)
        __asm__ volatile ("nop");
#endif
    }
}

extern "C" void kvb_platform_log_write(const char *data, size_t len)
{
    if (data == nullptr || len == 0u) {
        return;
    }

    const uint8_t *p = (const uint8_t *)data;
    int remaining = (int)len;

    while (remaining > 0) {
        int sent = g_Uart.Tx((uint8_t *)p, remaining);

        if (sent < 0) {
            /* Driver-reported error.  Best effort — abandon this line
             * rather than spin forever. */
            return;
        }

        p         += sent;
        remaining -= sent;

        if (remaining > 0) {
            /* FIFO was full.  Yield so the UART IRQ can drain it before
             * we retry; otherwise the busy loop starves the ISR on M0
             * and the FIFO never empties. */
            (void)kvb_thread_yield();
        }
    }
}

extern "C" void kvb_platform_log_flush(void)
{
    /* log_write guarantees FIFO acceptance, not physical transmission.
     * Give DMA/IRQ-backed UART output time to drain before a reset or before
     * the next parseable KVB block begins. */
    kvb_io_delay_ms(KVB_LOG_DRAIN_MS);
}
