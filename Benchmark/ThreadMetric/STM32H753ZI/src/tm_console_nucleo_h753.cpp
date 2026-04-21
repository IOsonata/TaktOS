// =============================================================================
// Thread-Metric console driver — USART3 polling mode on NUCLEO-H753ZI.
// =============================================================================
// The NUCLEO-H753ZI routes STM32H753 USART3 to the ST-LINK/V3 Virtual COM
// Port via PD8 (TX) and PD9 (RX). Appears on the host as /dev/ttyACM*
// (Linux), /dev/cu.usb* (macOS), or COMn (Windows). Baud: 115200 8N1.
//
// Solder bridges SB12 and SB19 on the board must be intact (factory default).
//
// Kernel clock: we leave everything at reset defaults, so USART3 is clocked
// by PCLK1 = HCLK = SYSCLK = 64 MHz (HSI). The STM32H7 USART uses the
// standard 16x oversampling async formula: BRR = fclk / baud.
// At 64 MHz / 115200 baud → BRR = 556, actual baud ≈ 115107 (-0.08 %).
//
// Registers are accessed directly — no Cube HAL, no CMSIS device header.
// =============================================================================

#include <stdint.h>
#include <stddef.h>

extern "C" uint32_t SystemCoreClock;

// -----------------------------------------------------------------------------
// RCC base = 0x58024400 (STM32H7 D3 domain). Registers used:
//   RCC_AHB4ENR   (+0xE0) — GPIOD clock enable (bit 3)
//   RCC_APB1LENR  (+0xE8) — USART3 clock enable (bit 18)
// -----------------------------------------------------------------------------
#define RCC_BASE             0x58024400UL
#define RCC_AHB4ENR          (*(volatile uint32_t*)(RCC_BASE + 0x0E0UL))
#define RCC_APB1LENR         (*(volatile uint32_t*)(RCC_BASE + 0x0E8UL))

#define RCC_AHB4ENR_GPIODEN  (1U << 3)
#define RCC_APB1LENR_USART3EN (1U << 18)

// -----------------------------------------------------------------------------
// GPIOD = 0x58020C00 (STM32H7 D3 domain).
//   MODER (+0x00): per-pin 2-bit mode. 10 = alternate function.
//   OTYPER (+0x04): push-pull (0, default) / open-drain (1).
//   OSPEEDR (+0x08): per-pin 2-bit speed. 11 = very high speed.
//   AFRL (+0x20) / AFRH (+0x24): per-pin 4-bit alternate-function select.
// USART3 on PD8 / PD9 uses AF7.
// -----------------------------------------------------------------------------
#define GPIOD_BASE           0x58020C00UL
#define GPIOD_MODER          (*(volatile uint32_t*)(GPIOD_BASE + 0x00UL))
#define GPIOD_OSPEEDR        (*(volatile uint32_t*)(GPIOD_BASE + 0x08UL))
#define GPIOD_AFRH           (*(volatile uint32_t*)(GPIOD_BASE + 0x24UL))

// -----------------------------------------------------------------------------
// USART3 = 0x40004800 (on APB1 in D2 domain).
//   CR1 (+0x00): UE [0], TE [3], RE [2], FIFOEN [29]
//   BRR (+0x0C): 16-bit baud divisor
//   ISR (+0x1C): TXE_FNF [7] / TXFNF [7] — TX ready bit
//   TDR (+0x28): transmit data register
// -----------------------------------------------------------------------------
#define USART3_BASE          0x40004800UL
#define USART3_CR1           (*(volatile uint32_t*)(USART3_BASE + 0x00UL))
#define USART3_BRR           (*(volatile uint32_t*)(USART3_BASE + 0x0CUL))
#define USART3_ISR           (*(volatile uint32_t*)(USART3_BASE + 0x1CUL))
#define USART3_TDR           (*(volatile uint32_t*)(USART3_BASE + 0x28UL))

#define USART_CR1_UE         (1U << 0)
#define USART_CR1_RE         (1U << 2)
#define USART_CR1_TE         (1U << 3)
#define USART_ISR_TXE        (1U << 7)    // TXE_FNF (TX register / FIFO not full)

#define UART_BAUDRATE        115200u

// -----------------------------------------------------------------------------
// Thread-Metric console hooks. Strong symbols — tm_port_taktos.cpp declares
// its stubs weak so this file wins at link time.
// -----------------------------------------------------------------------------

extern "C" void tm_hw_console_init(void)
{
    // Clock GPIOD on AHB4 and USART3 on APB1.
    RCC_AHB4ENR  |= RCC_AHB4ENR_GPIODEN;
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN;

    // Read-modify-write barrier — RCC writes on H7 can take a cycle or two
    // to propagate before peripheral registers are safely writable.
    (void)RCC_APB1LENR;

    // PD8 = USART3_TX (AF7), PD9 = USART3_RX (AF7).
    // MODER bits: pin N occupies bits [2N+1 : 2N]. 0b10 = alternate function.
    GPIOD_MODER   &= ~((3U << (8*2)) | (3U << (9*2)));
    GPIOD_MODER   |=  ((2U << (8*2)) | (2U << (9*2)));

    // OSPEEDR bits: pin N occupies bits [2N+1 : 2N]. 0b11 = very high speed.
    GPIOD_OSPEEDR |=  ((3U << (8*2)) | (3U << (9*2)));

    // AFRH covers pins 8..15; each pin occupies 4 bits.
    // Pin 8 -> bits [3:0], pin 9 -> [7:4]. AF7 = 0x7.
    GPIOD_AFRH    &= ~((0xFU << ((8 - 8)*4)) | (0xFU << ((9 - 8)*4)));
    GPIOD_AFRH    |=  ((7U   << ((8 - 8)*4)) | (7U   << ((9 - 8)*4)));

    // Configure USART3 async 8N1, TX-only enabled at the end.
    // USART3 kernel clock defaults to PCLK1 (RCC_D2CCIP2R.USART234578SEL=0b000).
    USART3_CR1 = 0;                                           // disable before reconfig
    USART3_BRR = (SystemCoreClock + UART_BAUDRATE / 2u) / UART_BAUDRATE;
    USART3_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

extern "C" void tm_putchar(int c)
{
    // Safety timeout so a misconfigured USART cannot hang the benchmark
    // indefinitely. At 115200 baud one byte takes ~87 us = ~5600 cycles @
    // 64 MHz; 2,000,000 cycles is >30 ms of headroom.
    uint32_t timeout = 2000000u;
    while ((USART3_ISR & USART_ISR_TXE) == 0u) {
        if (--timeout == 0u) return;
    }
    USART3_TDR = (uint32_t)(c & 0xFFu);
}
