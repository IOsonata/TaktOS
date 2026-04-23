// =============================================================================
// Thread-Metric console driver - USART1 polling mode on B-L475E-IOT01A2.
// =============================================================================
// The B-L475E-IOT01A2 (and A/A1) routes USART1 to the ST-LINK/V2-1 Virtual
// COM Port:
//     PB6  -> USART1_TX  (AF7)
//     PB7  -> USART1_RX  (AF7)
// Per ST UM2153 §6.4 "Virtual COM port" / schematic MB1297 rev C.
// Appears on the host as /dev/ttyACM* (Linux) or COMn (Windows). 115200 8N1.
//
// Registers are accessed directly - no CMSIS or IOsonata dependency, since
// IOsonata does not yet ship a port for STM32L4.
// =============================================================================

#include <stdint.h>
#include <stddef.h>

extern "C" uint32_t SystemCoreClock;

// ---- Peripheral addresses (RM0351 §2.2.2 Memory map) -----------------------
#define PERIPH_BASE         0x40000000UL
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)

// RCC register block (we only need AHB2ENR and APB2ENR)
#define RCC_AHB2ENR     (*(volatile uint32_t*)(AHB1PERIPH_BASE + 0x104CUL))
#define RCC_APB2ENR     (*(volatile uint32_t*)(AHB1PERIPH_BASE + 0x1060UL))

#define RCC_AHB2ENR_GPIOBEN     (1U <<  1)
#define RCC_APB2ENR_USART1EN    (1U << 14)

// GPIOB = 0x48000400
#define GPIOB_BASE      (AHB2PERIPH_BASE + 0x0400UL)
#define GPIOB_MODER     (*(volatile uint32_t*)(GPIOB_BASE + 0x00UL))
#define GPIOB_OSPEEDR   (*(volatile uint32_t*)(GPIOB_BASE + 0x08UL))
#define GPIOB_PUPDR     (*(volatile uint32_t*)(GPIOB_BASE + 0x0CUL))
#define GPIOB_AFRL      (*(volatile uint32_t*)(GPIOB_BASE + 0x20UL))

// USART1 = 0x40013800
#define USART1_BASE     (APB2PERIPH_BASE + 0x3800UL)
#define USART1_CR1      (*(volatile uint32_t*)(USART1_BASE + 0x00UL))
#define USART1_BRR      (*(volatile uint32_t*)(USART1_BASE + 0x0CUL))
#define USART1_ISR      (*(volatile uint32_t*)(USART1_BASE + 0x1CUL))
#define USART1_TDR      (*(volatile uint32_t*)(USART1_BASE + 0x28UL))

#define USART_CR1_UE    (1U << 0)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_TE    (1U << 3)
#define USART_ISR_TXE   (1U << 7)

#define UART_BAUDRATE   115200u

// -----------------------------------------------------------------------------
// Thread-Metric console hooks.
// -----------------------------------------------------------------------------

extern "C" void tm_hw_console_init(void)
{
    // Clocks: GPIOB on AHB2, USART1 on APB2.
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC_APB2ENR;           // read-back to flush write

    // PB6 = AF7 (USART1_TX), PB7 = AF7 (USART1_RX).
    // MODER[2n+1:2n]: 10 = alternate function
    GPIOB_MODER &= ~((3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOB_MODER |=  ((2U << (6 * 2)) | (2U << (7 * 2)));

    GPIOB_PUPDR &= ~((3U << (6 * 2)) | (3U << (7 * 2)));

    // AFRL covers pins 0..7 (4 bits each). PB6 -> bits [27:24], PB7 -> [31:28].
    GPIOB_AFRL &= ~((0xFU << (6 * 4)) | (0xFU << (7 * 4)));
    GPIOB_AFRL |=  ((0x7U << (6 * 4)) | (0x7U << (7 * 4)));

    // USART1 is clocked from PCLK2 by default (RCC_CCIPR.USART1SEL = 00).
    // At 80 MHz SYSCLK and APB2 prescaler = 1, PCLK2 = 80 MHz.
    USART1_CR1 = 0;
    USART1_BRR = (SystemCoreClock + UART_BAUDRATE / 2u) / UART_BAUDRATE;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

extern "C" void tm_putchar(int c)
{
    uint32_t timeout = 1000000u;
    while ((USART1_ISR & USART_ISR_TXE) == 0u) {
        if (--timeout == 0u) return;
    }
    USART1_TDR = (uint32_t)(c & 0xFFu);
}
