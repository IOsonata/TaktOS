// =============================================================================
// Thread-Metric console — LPUART1 on NUCLEO-G474RE.
// =============================================================================
// NUCLEO-G474RE routes LPUART1 (not USART2) to the ST-LINK/V3E VCP.
// Wiring: PA2 = LPUART1_TX, PA3 = LPUART1_RX, alternate function AF12.
// Default solder-bridge config leaves this connected to the VCP.
//
// LPUART1 kernel clock defaults to PCLK1 via RCC_CCIPR.LPUART1SEL=00 (reset).
// At SYSCLK=170 MHz and PPRE1=/1, PCLK1=170 MHz.
//
// LPUART baud rate formula (RM0440 §34.5.4):
//     BRR = (256 * f_CK) / baudrate
// with BRR required in [0x300, 0xFFFFF]. At 170 MHz / 115200:
//     BRR = 256 * 170e6 / 115200 = 377778 ≈ 0x5C372  (within range)
//
// This file provides the two weak hooks that tm_port_taktos.cpp calls:
//   tm_hw_console_init()
//   tm_putchar(int)
// =============================================================================

#include <stdint.h>

extern "C" uint32_t SystemCoreClock;

// ---------------- RCC (same base layout as system_stm32g474.c) --------------
#define RCC_BASE            0x40021000UL
#define RCC_AHB2ENR         (*(volatile uint32_t*)(RCC_BASE + 0x4C))
#define RCC_APB1ENR2        (*(volatile uint32_t*)(RCC_BASE + 0x5C))
#define RCC_CCIPR           (*(volatile uint32_t*)(RCC_BASE + 0x88))

#define RCC_AHB2ENR_GPIOAEN (1U << 0)
#define RCC_APB1ENR2_LPUART1EN (1U << 0)

// ---------------- GPIOA -----------------------------------------------------
#define GPIOA_BASE          0x48000000UL
#define GPIOA_MODER         (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_OSPEEDR       (*(volatile uint32_t*)(GPIOA_BASE + 0x08))
#define GPIOA_AFRL          (*(volatile uint32_t*)(GPIOA_BASE + 0x20))

// ---------------- LPUART1 ---------------------------------------------------
#define LPUART1_BASE        0x40008000UL
#define LPUART1_CR1         (*(volatile uint32_t*)(LPUART1_BASE + 0x00))
#define LPUART1_BRR         (*(volatile uint32_t*)(LPUART1_BASE + 0x0C))
#define LPUART1_ISR         (*(volatile uint32_t*)(LPUART1_BASE + 0x1C))
#define LPUART1_TDR         (*(volatile uint32_t*)(LPUART1_BASE + 0x28))

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_FIFOEN    (1U << 29)
#define USART_ISR_TXE       (1U << 7)    // TXE / TXFNF (TX ready)

#define UART_BAUDRATE       115200u

extern "C" void tm_hw_console_init(void)
{
    // Clock GPIOA (AHB2) and LPUART1 (APB1-2).
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    (void)RCC_APB1ENR2;

    // PA2 = AF12 (LPUART1_TX), PA3 = AF12 (LPUART1_RX).
    // MODER bits [2N+1 : 2N] — 0b10 = alternate function.
    GPIOA_MODER   &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA_MODER   |=  ((2U << (2*2)) | (2U << (3*2)));

    // Very-high-speed drive.
    GPIOA_OSPEEDR |=  ((3U << (2*2)) | (3U << (3*2)));

    // AFRL covers pins 0..7; each pin = 4 bits. Pin 2 → bits [11:8], pin 3 → bits [15:12].
    // AF12 = 0xC.
    GPIOA_AFRL    &= ~((0xFU << (2*4)) | (0xFU << (3*4)));
    GPIOA_AFRL    |=  ((0xCU << (2*4)) | (0xCU << (3*4)));

    // Configure LPUART1 async 8N1. Default kernel clock = PCLK1 (=SYSCLK here).
    LPUART1_CR1 = 0;
    // BRR = 256 * fclk / baud. Round to nearest.
    LPUART1_BRR = (uint32_t)((256ULL * (uint64_t)SystemCoreClock
                              + (uint64_t)(UART_BAUDRATE / 2))
                             / (uint64_t)UART_BAUDRATE);
    LPUART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

extern "C" void tm_putchar(int c)
{
    // Timeout = ~100 ms at 170 MHz, ample for 115200 baud.
    uint32_t timeout = 20000000u;
    while ((LPUART1_ISR & USART_ISR_TXE) == 0u) {
        if (--timeout == 0u) return;
    }
    LPUART1_TDR = (uint32_t)(c & 0xFFu);
}
