/**-------------------------------------------------------------------------
 * @file	main.cpp
 *
 * @brief	Thread-Metric benchmark entry point  shared across all MCUs.
 *
 * IOsonata provides startup, vector table, and UART driver through
 * libIOsonata_<MCU>.a (linked by each per-MCU Eclipse project).
 *
 * The per-MCU board.h supplies UART pin definitions and the TM5
 * software-interrupt configuration (TM_SW_IRQn, TM_SW_IRQ_VECTOR).
 * Vendor IRQ handlers are weak symbols in libIOsonata; our strong
 * TM_SW_IRQ_VECTOR definition overrides the one we picked for TM5.
 *
 * The RTOS port (tm_port_taktos.cpp, tm_port_freertos.c, etc.) is
 * selected by the Eclipse project  one of them is compiled into the
 * binary. main() does not know which RTOS it is running over.
 * -------------------------------------------------------------------------*/
#include <stdint.h>

#include "coredev/uart.h"
#include "coredev/iopincfg.h"

#include "tm_api.h"

#include "board.h"            /* per-MCU: UART_TX_PORT/..., UART_DEVNO, TM_SW_IRQn */

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

/* Global UART C++ instance. The operator UARTDev_t*() on line 249 of
 * IOsonata's uart.h lets us pass g_Uart to C-API functions (like
 * UARTvprintf in tm_report.cpp) without exposing vDevData. */
UART g_Uart;


int main(void)
{
    g_Uart.Init(s_UartCfg);

    tm_main();

    return 0;
}
