/**-------------------------------------------------------------------------
 * @file    main.cpp
 *
 * @brief   KVB benchmark entry point — shared across all MCUs.
 *
 * IOsonata provides startup, vector table, and UART driver through
 * libIOsonata_<MCU>.a (linked by each per-MCU Eclipse project).
 *
 * The per-MCU board.h supplies UART pin definitions, core clock, and
 * any arch-specific helpers KVB needs.
 *
 * The kernel port (kvb_port_taktos.c, kvb_port_freertos.c, ...) is
 * selected by the Eclipse project — one of them is compiled into the
 * binary. main() does not know which kernel it is running over; it
 * calls kvb_kernel_start() and the port wires the runner thread.
 * -------------------------------------------------------------------------*/
#include <stdint.h>

#include "coredev/uart.h"
#include "coredev/iopincfg.h"

#include "kvb.h"

#include "board.h"            /* per-MCU: UART_TX_PORT/..., UART_DEVNO, ... */

#ifndef UARTFIFOSIZE
#define UARTFIFOSIZE    CFIFO_MEMSIZE(256)
#endif

static uint8_t s_UartTxFifo[UARTFIFOSIZE];

static IOPinCfg_t s_UartPins[] = {
    { UART_RX_PORT, UART_RX_PIN, UART_RX_PINOP, IOPINDIR_INPUT,  IOPINRES_NONE, IOPINTYPE_NORMAL },
    { UART_TX_PORT, UART_TX_PIN, UART_TX_PINOP, IOPINDIR_OUTPUT, IOPINRES_NONE, IOPINTYPE_NORMAL },
};

static const UARTCfg_t s_UartCfg = {
    .DevNo         = UART_DEVNO,
    .pIOPinMap     = s_UartPins,
    .NbIOPins      = sizeof(s_UartPins) / sizeof(IOPinCfg_t),
    .Rate          = 115200,
    .DataBits      = 8,
    .Parity        = UART_PARITY_NONE,
    .StopBits      = 1,
    .FlowControl   = UART_FLWCTRL_NONE,
    .bIntMode      = true,
    .IntPrio       = 6,
    .EvtCallback   = nullptr,
    .bFifoBlocking = true,
    .RxMemSize     = 0,
    .pRxMem        = nullptr,
    .TxMemSize     = UARTFIFOSIZE,
    .pTxMem        = s_UartTxFifo,
    .bDMAMode      = true,
};

/* Global UART C++ instance.  The operator UARTDev_t*() on UART (uart.h)
 * lets us pass g_Uart to C-API functions like UARTTx without exposing
 * vDevData. */
UART g_Uart;

static void kvb_startup_delay_ms(uint32_t ms)
{
    /* Runs before the RTOS scheduler starts.  Only used to let the UART /
     * virtual COM bridge settle after reset. */
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


int main(void)
{
    g_Uart.Init(s_UartCfg);

    /* Reset can leave partial bytes in the probe / USB-UART bridge.  Delay,
     * emit only line separators, and drain before the first [KVB] record. */
    kvb_startup_delay_ms(KVB_STARTUP_QUIET_MS);
    kvb_platform_log_write("\r\n\r\n", 4u);
    kvb_platform_log_flush();

    /* kvb_kernel_start initialises the kernel under test, creates the
     * runner thread at KVB_PRIORITY_HIGHEST, and starts the scheduler.
     * On TaktOS this never returns; on Zephyr (where the kernel is
     * already running before main()) it returns after the runner has
     * executed the test set. */
    kvb_kernel_start(kvb_runner_main, nullptr);

    return 0;
}
