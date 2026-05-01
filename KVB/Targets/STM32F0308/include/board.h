/**-------------------------------------------------------------------------
 * @file    board.h
 * @brief   STM32F0308-DISCO board definitions for KVB benchmark.
 *
 * UART console: USART1, TXD PA9 / RXD PA10.  The STM32F0308-DISCO
 * routes USART1 to the ST-LINK/V2-1 Virtual COM Port at 115200 8N1.
 * On the host this appears as /dev/ttyACM* (Linux), /dev/cu.usb*
 * (macOS), or COMn (Windows).
 *
 * Core clock: 48 MHz from HSI + PLL (configured by IOsonata SystemInit).
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include "stm32f0xx.h"   /* CMSIS device header — brings core_cm0.h in */

/* --- UART pins (USART1 -> ST-LINK VCOM) --- */
/* IOsonata STM32F0 pinop encoding: low nibble = AF number,
 * bit 4 = alternate-function-mode flag.  USART1 on PA9/PA10 needs
 * (2 | (1 << 4)) = 0x12 — empirically confirmed on hardware. */
#define UART_RX_PORT    0
#define UART_RX_PIN     10
#define UART_RX_PINOP   (2 | (1 << 4))
#define UART_TX_PORT    0
#define UART_TX_PIN     9
#define UART_TX_PINOP   (2 | (1 << 4))

#define UART_DEVNO      0     /* USART1 */

/* --- Core clock --- */
#define KVB_BOARD_CORE_CLOCK_HZ    48000000u

#endif /* __BOARD_H__ */
