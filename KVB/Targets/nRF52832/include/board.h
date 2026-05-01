/**-------------------------------------------------------------------------
 * @file    board.h
 * @brief   nRF52-DK (PCA10040) board definitions for KVB benchmark.
 *
 * UART console: UART0, TXD P0.06 / RXD P0.08, RTS P0.05 / CTS P0.07.
 * The PCA10040 routes UART0 through the on-board J-Link OB to a USB CDC
 * Virtual COM Port at 115200 8N1.  On the host this appears as
 * /dev/ttyACM* (Linux), /dev/cu.usb* (macOS), or COMn (Windows).
 *
 * Core clock: 64 MHz from internal HFXO.  IOsonata SystemInit configures
 * the HFCLK source on the nRF52832 — no external crystal selection needed
 * here at the board layer.
 *
 * BLYST Nano variant note: BLYST Nano (nRF52832 module on a custom PCB)
 * uses the same MCU core and same KVB tunings as PCA10040.  Override
 * UART pin defines below in a derivative board.h if pin routing differs.
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include "nrf.h"   /* Nordic CMSIS device header — brings core_cm4.h in */

/* --- UART pins (UART0 -> J-Link OB VCOM on PCA10040) --- */
/* IOsonata nRF52 pin encoding: bits 0..4 = pin number, bit 5 = port.
 * For the nRF52832 (single port P0), port bit is always 0 and the pin
 * field carries the GPIO pin number directly. */
#define UART_RX_PORT    0
#define UART_RX_PIN     8
#define UART_RX_PINOP   0           /* default GPIO function — UART driver assigns */
#define UART_TX_PORT    0
#define UART_TX_PIN     7
#define UART_TX_PINOP   0
#define UART_CTS_PORT   0
#define UART_CTS_PIN    6
#define UART_CTS_PINOP  0
#define UART_RTS_PORT   0
#define UART_RTS_PIN    5
#define UART_RTS_PINOP  0

#define UART_DEVNO      0     /* UART0 */

/* --- Core clock --- */
#define KVB_BOARD_CORE_CLOCK_HZ    64000000u

#endif /* __BOARD_H__ */
