/**-------------------------------------------------------------------------
 * @file    board.h
 * @brief   nRF54L15-DK board definitions for KVB benchmark.
 *
 * UART console: UARTE30, TXD P0.04 / RXD P0.05.  The DK routes UARTE30
 * through the on-board J-Link OB to a USB CDC Virtual COM Port at
 * 115200 8N1.  On the host this appears as /dev/ttyACM* (Linux),
 * /dev/cu.usb* (macOS), or COMn (Windows).
 *
 * Core clock: 128 MHz from internal HFXO.  IOsonata SystemInit
 * configures the clock source on the nRF54L15  no external crystal
 * selection needed at the board layer.
 *
 * NB: UART_DEVNO is the IOsonata enumeration value, NOT the nRF54L15
 * silicon instance number.  If your IOsonata build maps UARTE30 to a
 * different DevNo, edit the single line below.
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

/* --- UART pins (nRF54L15-DK VCOM via UARTE30) --- */
#define UART_RX_PORT    0
#define UART_RX_PIN     1
#define UART_RX_PINOP   0
#define UART_TX_PORT    0
#define UART_TX_PIN     0
#define UART_TX_PINOP   0

#define UART_DEVNO      0   /* UARTE30  IOsonata DevNo for the nRF54L15 console UART */

/* --- Core clock --- */
#define KVB_BOARD_CORE_CLOCK_HZ    128000000u

#endif /* __BOARD_H__ */
