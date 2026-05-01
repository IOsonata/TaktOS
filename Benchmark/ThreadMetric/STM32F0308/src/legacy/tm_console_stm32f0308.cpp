// =============================================================================
// Thread-Metric console driver — USART1 polling mode on STM32F0308-DISCO.
// =============================================================================
// The STM32F0308-DISCO routes USART1 (PA9 TX / PA10 RX) to the ST-LINK/V2-1
// Virtual COM Port. Appears on the host as /dev/ttyACM* (Linux), /dev/cu.usb*
// (macOS), or COMn (Windows). Baud: 115200 8N1.
//
// Registers are accessed directly — no CMSIS or IOsonata dependency, since
// IOsonata does not ship an STM32F0 port.
// =============================================================================

#include <stdint.h>
#include <stddef.h>

extern "C" uint32_t SystemCoreClock;

// ----- Peripheral addresses --------------------------------------------------
#define PERIPH_BASE         0x40000000UL
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)

#define RCC_AHBENR     (*(volatile uint32_t*)(AHB1PERIPH_BASE + 0x1014UL))
#define RCC_APB2ENR    (*(volatile uint32_t*)(AHB1PERIPH_BASE + 0x1018UL))

#define RCC_AHBENR_GPIOAEN      (1U << 17)
#define RCC_APB2ENR_USART1EN    (1U << 14)

// GPIOA = 0x48000000
#define GPIOA_MODER    (*(volatile uint32_t*)(AHB2PERIPH_BASE + 0x0000UL))
#define GPIOA_AFRH     (*(volatile uint32_t*)(AHB2PERIPH_BASE + 0x0024UL))

// USART1 = 0x40013800
#define USART1_BASE    (APB2PERIPH_BASE + 0x3800UL)
#define USART1_CR1     (*(volatile uint32_t*)(USART1_BASE + 0x00UL))
#define USART1_BRR     (*(volatile uint32_t*)(USART1_BASE + 0x0CUL))
#define USART1_ISR     (*(volatile uint32_t*)(USART1_BASE + 0x1CUL))
#define USART1_TDR     (*(volatile uint32_t*)(USART1_BASE + 0x28UL))

#define USART_CR1_UE    (1U << 0)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_TE    (1U << 3)
#define USART_ISR_TXE   (1U << 7)

#define UART_BAUDRATE   115200u

// -----------------------------------------------------------------------------
// Thread-Metric console hooks.
//
// These are strong symbols. tm_port_taktos.cpp declares them weak so this
// file wins at link time. If this file is dropped into a project, the
// console comes to life at 115200 8N1 on USART1.
// -----------------------------------------------------------------------------

extern "C" void tm_hw_console_init(void)
{
    // Clocks: GPIOA on AHB, USART1 on APB2.
    RCC_AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    // PA9 = AF1 (USART1_TX), PA10 = AF1 (USART1_RX).
    // MODER: alternate-function = 0b10 for each pin.
    GPIOA_MODER &= ~((3U << (9 * 2)) | (3U << (10 * 2)));
    GPIOA_MODER |=  ((2U << (9 * 2)) | (2U << (10 * 2)));
    // AFRH covers pins 8..15; AF1 = 0x1. Pin 9 -> bits [7:4], pin 10 -> [11:8].
    GPIOA_AFRH &= ~((0xFU << ((9  - 8) * 4)) | (0xFU << ((10 - 8) * 4)));
    GPIOA_AFRH |=  ((1U   << ((9  - 8) * 4)) | (1U   << ((10 - 8) * 4)));

    // USART1 is clocked from SYSCLK by default (RCC_CFGR3.USART1SW = 00).
    USART1_CR1 = 0;
    USART1_BRR = (SystemCoreClock + UART_BAUDRATE / 2u) / UART_BAUDRATE;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

extern "C" void tm_putchar(int c)
{
    // Safety timeout so a misconfigured USART cannot hang the benchmark
    // indefinitely. At 115200 baud and 48 MHz SYSCLK, one character fits
    // in well under 10000 cycles; 1,000,000 is ~20 ms of headroom.
    uint32_t timeout = 1000000u;
    while ((USART1_ISR & USART_ISR_TXE) == 0u) {
        if (--timeout == 0u) return;
    }
    USART1_TDR = (uint32_t)(c & 0xFFu);
}
