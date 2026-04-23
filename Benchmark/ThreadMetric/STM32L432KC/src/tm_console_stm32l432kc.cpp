// =============================================================================
// Thread-Metric console driver - USART2 polling mode on NUCLEO-L432KC.
// =============================================================================
// The NUCLEO-L432KC routes USART2 to the ST-LINK/V2-1 Virtual COM Port:
//     PA2  -> USART2_TX  (AF7)
//     PA15 -> USART2_RX  (AF3)
// Appears on the host as /dev/ttyACM* (Linux) or COMn (Windows). 115200 8N1.
//
// This is per ST UM1956. The PA3 pin is NOT wired to ST-LINK VCP on this
// board, so we deliberately use PA15 for RX (and configure its AF mux to
// AF3, which is asymmetric with TX's AF7 - this is the published wiring).
//
// Registers are accessed directly - no CMSIS or IOsonata dependency, since
// IOsonata does not yet ship a port for STM32L4.
// =============================================================================

#include <stdint.h>
#include <stddef.h>

extern "C" uint32_t SystemCoreClock;

// ---- Peripheral addresses (RM0394 §2.2.2 Memory map) -----------------------
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)

// RCC register block (we only need AHB2ENR and APB1ENR1)
#define RCC_AHB2ENR     (*(volatile uint32_t*)(AHB1PERIPH_BASE + 0x104CUL))
#define RCC_APB1ENR1    (*(volatile uint32_t*)(AHB1PERIPH_BASE + 0x1058UL))

#define RCC_AHB2ENR_GPIOAEN     (1U <<  0)
#define RCC_APB1ENR1_USART2EN   (1U << 17)

// GPIOA = 0x48000000
#define GPIOA_BASE      (AHB2PERIPH_BASE + 0x0000UL)
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00UL))
#define GPIOA_OSPEEDR   (*(volatile uint32_t*)(GPIOA_BASE + 0x08UL))
#define GPIOA_PUPDR     (*(volatile uint32_t*)(GPIOA_BASE + 0x0CUL))
#define GPIOA_AFRL      (*(volatile uint32_t*)(GPIOA_BASE + 0x20UL))
#define GPIOA_AFRH      (*(volatile uint32_t*)(GPIOA_BASE + 0x24UL))

// USART2 = 0x40004400
#define USART2_BASE     (APB1PERIPH_BASE + 0x4400UL)
#define USART2_CR1      (*(volatile uint32_t*)(USART2_BASE + 0x00UL))
#define USART2_BRR      (*(volatile uint32_t*)(USART2_BASE + 0x0CUL))
#define USART2_ISR      (*(volatile uint32_t*)(USART2_BASE + 0x1CUL))
#define USART2_TDR      (*(volatile uint32_t*)(USART2_BASE + 0x28UL))

#define USART_CR1_UE    (1U << 0)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_TE    (1U << 3)
#define USART_ISR_TXE   (1U << 7)    /* TXE/TXFNF - transmit data reg empty */

#define UART_BAUDRATE   115200u

// -----------------------------------------------------------------------------
// Thread-Metric console hooks.
//
// These are strong symbols. tm_port_taktos.cpp declares them weak so this
// file wins at link time. USART2 comes up at 115200 8N1 on ST-LINK VCP.
// -----------------------------------------------------------------------------

extern "C" void tm_hw_console_init(void)
{
    // Clocks: GPIOA on AHB2, USART2 on APB1 bank 1.
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    (void)RCC_APB1ENR1;          // read-back to flush write

    // PA2  (TX) -> AF7  (USART2_TX)
    // PA15 (RX) -> AF3  (USART2_RX)   [AF mux is asymmetric on this board]
    // MODER[2n+1:2n]: 00 input, 01 output, 10 alternate function, 11 analog
    GPIOA_MODER &= ~((3U << ( 2 * 2)) | (3U << (15 * 2)));
    GPIOA_MODER |=  ((2U << ( 2 * 2)) | (2U << (15 * 2)));

    // No pull-up/pull-down on either pin (rely on receiver idle-high)
    GPIOA_PUPDR &= ~((3U << ( 2 * 2)) | (3U << (15 * 2)));

    // High speed is unnecessary for 115200 baud, but harmless. Leave at reset.

    // AFRL covers pins 0..7 (4 bits each). PA2 -> bits [11:8]. AF7 = 0x7.
    GPIOA_AFRL &= ~(0xFU << (2 * 4));
    GPIOA_AFRL |=  (0x7U << (2 * 4));

    // AFRH covers pins 8..15. PA15 -> bits [31:28]. AF3 = 0x3.
    GPIOA_AFRH &= ~(0xFU << ((15 - 8) * 4));
    GPIOA_AFRH |=  (0x3U << ((15 - 8) * 4));

    // USART2 is clocked from PCLK1 by default (RCC_CCIPR.USART2SEL = 00).
    // At 80 MHz SYSCLK and AHB/APB1 prescalers = 1, PCLK1 = 80 MHz.
    // BRR = fCK / baud (OVER8=0, default) = 80_000_000 / 115200 ~= 694.
    USART2_CR1 = 0;
    USART2_BRR = (SystemCoreClock + UART_BAUDRATE / 2u) / UART_BAUDRATE;
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

extern "C" void tm_putchar(int c)
{
    // Safety timeout so a misconfigured USART cannot hang the benchmark
    // indefinitely. At 80 MHz and 115200 baud, one char fits in ~7000 cycles;
    // 1,000,000 is >12 ms of headroom.
    uint32_t timeout = 1000000u;
    while ((USART2_ISR & USART_ISR_TXE) == 0u) {
        if (--timeout == 0u) return;
    }
    USART2_TDR = (uint32_t)(c & 0xFFu);
}
