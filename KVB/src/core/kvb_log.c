/**-------------------------------------------------------------------------
 * @file    kvb_log.c
 *
 * @brief   KVB log line emission — kvb_log_line() and kvb_logf().
 *
 * Single-thread design: the KVB runner is the only caller of these
 * functions.  No re-entry from ISR context, no concurrent calls from
 * worker threads.  The static format buffer below relies on that.
 *
 * Why a static buffer instead of a stack-local one:
 *   With KVB_LOG_BUFFER_SIZE = 256, every kvb_logf() call would push a
 *   full 256-byte buffer on the runner stack, plus va_list, plus
 *   vsnprintf's internal locals.  That's ~500 bytes of peak stack
 *   inside what's already a 2 KB runner stack on tight nRF52832 /
 *   STM32F0308 builds — uncomfortable for a function called from
 *   inside a context-switch path (kvb_thread_yield is invoked from
 *   kvb_platform_log_write while the buffer is live).  A single static
 *   buffer in BSS removes that pressure entirely.
 *
 *   The static buffer is also robust to a second hazard: an IOsonata
 *   DMA-mode UART driver that retains a pointer into the user-supplied
 *   buffer for the duration of the transfer.  If kvb_logf() returned
 *   while DMA was still draining the previous content, and the next
 *   kvb_logf() call reused the same stack address with new content,
 *   the in-flight DMA would transmit a mix of old and new data — one
 *   of the failure modes that produced the truncated-then-restart
 *   patterns observed at boot on FreeRTOS / TaktOS / ThreadX.  With
 *   the buffer in BSS, the address is fixed and the next write only
 *   happens after kvb_platform_log_write() has accepted every byte
 *   into the TX FIFO (i.e. the DMA has consumed the previous content).
 * -------------------------------------------------------------------------*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "kvb_config.h"
#include "kvb_platform_port.h"
#include "kvb_runner.h"

/* Single-thread-only format buffer.  The KVB runner is the sole caller
 * of kvb_log_line() / kvb_logf(); no ISR or worker thread ever logs.
 * Sized to KVB_LOG_BUFFER_SIZE so a long PLATFORM/CONFIG/BUILD line
 * is fully captured rather than vsnprintf-truncated.  Kept in BSS
 * (zero-initialised at boot, no constructor) so the symbol is not
 * sensitive to startup ordering. */
static char s_log_buf[KVB_LOG_BUFFER_SIZE];

void kvb_log_line(const char *line)
{
    if (line == 0) {
        return;
    }

    kvb_platform_log_write(line, strlen(line));
    kvb_platform_log_write("\n", 1u);
}

void kvb_logf(const char *fmt, ...)
{
    va_list ap;
    int n;

    if (fmt == 0) {
        return;
    }

    va_start(ap, fmt);
    n = vsnprintf(s_log_buf, sizeof(s_log_buf), fmt, ap);
    va_end(ap);

    if (n <= 0) {
        return;
    }

    if ((size_t)n >= sizeof(s_log_buf)) {
        n = (int)sizeof(s_log_buf) - 1;
        s_log_buf[n] = '\0';
    }

    kvb_platform_log_write(s_log_buf, (size_t)n);
    kvb_platform_log_write("\n", 1u);
}
