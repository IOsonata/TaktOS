// =============================================================================
// Thread-Metric console override - IOsonata UART on SAM4LC8C
// =============================================================================
// This file provides working implementations of tm_hw_console_init() and
// tm_putchar() using the IOsonata UART driver. It replaces the weak no-op
// fallbacks in tm_port_taktos.cpp.
//
// HOW TO USE:
//   1. Copy this file into one of the Eclipse projects under its local src/
//      folder (next to main.cpp).
//   2. Edit the UART_CFG_* defines below to match your board wiring - pick
//      any of the four SAM4L USARTs (USART0..3) and the pins your board
//      actually has routed to a serial terminal. SAM4LC-EK typically uses
//      USART2 on the EDBG virtual-COM bridge.
//   3. The strong symbols defined here will override the weak no-op fallbacks
//      in tm_port_taktos.cpp automatically at link time.
//
// HOW IT WORKS:
//   IOsonata's UARTDev_t class handles clock gating, pin-muxing, baud-rate
//   calculation, and TXRDY polling internally - all things the datasheet
//   register-level code was getting wrong. We simply initialize a UART
//   instance once, then use its Tx(const uint8_t*, int) method from
//   tm_putchar.
// =============================================================================

#include <stdint.h>
#include "coredev/uart.h"   // IOsonata UART API - in IOsonata/include/coredev
#include "coredev/iopincfg.h"

// -----------------------------------------------------------------------------
// Board configuration - SAM4L8 Xplained Pro EDBG Virtual COM Port defaults
// -----------------------------------------------------------------------------
// Per Atmel SAM4L8 Xplained Pro User Guide (Atmel-42103B), Section 4.3.2:
//   EDBG VCOM: USART1  (NOT USART2 - that's the SAM4L-EK wiring)
//     PC27 -> USART1 TXD  (SAM4L8 TX line)
//     PC26 -> USART1 RXD  (SAM4L8 RX line)
//
// If you're using a different SAM4LC8C board (custom or SAM4L-EK), update
// these defines to match your schematic.
#define UART_CFG_PORT_NUM       1

// TX / RX pins. SAM4L GPIO uses port A/B/C (letter-coded 0/1/2) and
// peripheral-function letters A..H (0..7). SAM4L8 Xplained Pro mapping:
//   PC27 = port C (2), pin 27, function <A/B> = USART1 TXD
//   PC26 = port C (2), pin 26, function <A/B> = USART1 RXD
//
// IMPORTANT - peripheral function letter:
// The exact function (A/B/C/D/...) for PC26/PC27 -> USART1 must be read
// from Table 3-1 "GPIO Controller Function Multiplexing - 100-pin Package"
// in the SAM4L datasheet, OR from the Atmel ASF board file
// sam/boards/sam4l8_xplained_pro/sam4l8_xplained_pro.h which defines
// COM_PORT_PIN_TX_MUX etc. Function A (0) is the most common primary
// mapping on SAM4L. If UART output still doesn't appear with PINOP=0,
// try 1 (B) next, then 2 (C), etc.
#define UART_CFG_TX_PORT        2       // 0=A, 1=B, 2=C
#define UART_CFG_TX_PIN         27      // PC27 -> USART1 TXD
#define UART_CFG_TX_PINOP       1       // start with A (0); try B (1) if no output
#define UART_CFG_RX_PORT        2
#define UART_CFG_RX_PIN         26      // PC26 -> USART1 RXD
#define UART_CFG_RX_PINOP       1

#define UART_CFG_BAUDRATE       115200
#define UART_CFG_RX_BUF_SIZE    16
#define UART_CFG_TX_BUF_SIZE    256

// -----------------------------------------------------------------------------
// IOsonata UART instance and buffers
// -----------------------------------------------------------------------------
static IOPinCfg_t s_uart_pins[] = {
    { UART_CFG_RX_PORT, UART_CFG_RX_PIN, UART_CFG_RX_PINOP, IOPINDIR_INPUT,
      IOPINRES_NONE, IOPINTYPE_NORMAL },
    { UART_CFG_TX_PORT, UART_CFG_TX_PIN, UART_CFG_TX_PINOP, IOPINDIR_OUTPUT,
      IOPINRES_NONE, IOPINTYPE_NORMAL },
};

static uint8_t s_uart_rx_buf[UART_CFG_RX_BUF_SIZE];
static uint8_t s_uart_tx_buf[UART_CFG_TX_BUF_SIZE];

static UARTCfg_t s_uart_cfg = {
    UART_CFG_PORT_NUM,                           // DevNo
    s_uart_pins,
    sizeof(s_uart_pins) / sizeof(IOPinCfg_t),
    UART_CFG_BAUDRATE,                           // Rate
    8,                                            // DataBits
    UART_PARITY_NONE,
    1,                                            // StopBits
    UART_FLWCTRL_NONE,
    false,                                        // bIntMode (polling)
    0,                                            // IntPrio
    nullptr,                                      // EvtCallback
    true,                                         // bFifoBlocking
    UART_CFG_RX_BUF_SIZE, s_uart_rx_buf,
    UART_CFG_TX_BUF_SIZE, s_uart_tx_buf,
    false,                                        // bDMAMode
};

// Single UART instance, lazily initialized on first tm_hw_console_init()
static UART s_uart;
static bool s_uart_ready = false;

// -----------------------------------------------------------------------------
// Thread-Metric console hooks (override weak fallbacks in tm_port_taktos.cpp)
// -----------------------------------------------------------------------------
extern "C" void tm_hw_console_init(void)
{
    if (!s_uart_ready) {
        s_uart_ready = s_uart.Init(s_uart_cfg);
    }
}

extern "C" void tm_putchar(int c)
{
    if (!s_uart_ready) {
        return;
    }
    uint8_t ch = (uint8_t)(c & 0xFFu);
    (void)s_uart.Tx(&ch, 1);
}
