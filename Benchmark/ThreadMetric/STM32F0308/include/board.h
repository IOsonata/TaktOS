/**-------------------------------------------------------------------------
 * @file    board.h
 * @brief   STM32F030R8 (Cortex-M0) board definitions for Thread-Metric.
 *
 * Cortex-M0 @ 48 MHz, ARMv6-M, no FPU, soft-float ABI.
 *
 * UART console on USART1 via IOsonata (DevNo = 1).
 *   PA9  -> USART1 TXD  (alternate function 1)
 *   PA10 -> USART1 RXD  (alternate function 1)
 *
 * Wiring matches the STM32F0308-DISCO and the NUCLEO-F030R8 ST-LINK/V2-1
 * Virtual COM Port. If your board routes USART1 differently, edit the
 * UART_* macros below.
 *
 * TM5 software interrupt borrows the TIM17 NVIC line (IRQ 22). The TIM17
 * peripheral itself is never clocked or configured by the benchmark;
 * only its NVIC slot is used. Because none of the seven Thread-Metric
 * tests in scope (Basic, Cooperative, Preemptive, Message, Sync, Mutex,
 * MutexBarging) calls tm_cause_interrupt(), the TIM17_IRQHandler vector
 * remains the IOsonata weak default at runtime. The macros are kept
 * here for symmetry with the nRF52832 / nRF54L15 / SAM4LC8C ports.
 *
 * NB: ARMv6-M (Cortex-M0) requires word-aligned access to NVIC_IPR --
 * byte access faults. TmEnableSoftwareInterrupt() therefore does
 * read-modify-write on the 32-bit IPR word, unlike the byte-access
 * version that works on the Cortex-M4 ports (SAM4LC8C, nRF52832).
 *
 * No SAM/STM32 CMSIS umbrella is included; the four NVIC/SCB
 * register addresses below are architectural (defined by the ARMv6-M
 * Architecture Reference Manual) and identical on every Cortex-M0
 * implementation.
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdint.h>

/* --- UART pins (STM32F0308-DISCO / NUCLEO-F030R8 ST-LINK VCOM via USART1) --- */
#define UART_RX_PORT    0       /* PortA: A=0, B=1, C=2, ... */
#define UART_RX_PIN     10      /* PA10 -> USART1 RXD */
#define UART_RX_PINOP   1       /* AF1 = USART1 on PA9/PA10 (STM32F030R8 datasheet) */
#define UART_TX_PORT    0
#define UART_TX_PIN     9       /* PA9  -> USART1 TXD */
#define UART_TX_PINOP   1

#define UART_DEVNO      1       /* USART1 -- IOsonata DevNo for the STM32F0 console UART */

/* --- Core clock (post IOsonata SystemInit on STM32F030R8: HSI -> PLL x 6 -> 48 MHz) --- */
#define TM_CORE_CLOCK_HZ    48000000u

/* --- TM5 software interrupt --- */
/* STM32F030R8 NVIC IRQ numbers (RM0360 Table 32 "Vector table").
 * TIM17 is borrowed because (a) IOsonata's SystemInit leaves its peripheral
 * clock gated off, (b) it cannot raise its line on its own, (c) its NVIC
 * vector name TIM17_IRQHandler is a weak default in the IOsonata STM32F0
 * startup. */
#define TM_SW_IRQn            22u
#define TM_SW_IRQ_VECTOR      TIM17_IRQHandler

/* --- ARMv6-M architectural register addresses (Cortex-M0 baseline) ---
 * These are defined by the ARMv6-M ARM, not by ST, so they are the same
 * on every Cortex-M0 silicon. */
#define BOARD_NVIC_ISER ((volatile uint32_t *) 0xE000E100u)  /* Set-Enable */
#define BOARD_NVIC_ISPR ((volatile uint32_t *) 0xE000E200u)  /* Set-Pending */
#define BOARD_NVIC_IPR  ((volatile uint32_t *) 0xE000E400u)  /* Priority -- WORD access only on M0 */
#define BOARD_SCB_SHPR3 (*(volatile uint32_t *) 0xE000ED20u) /* System Handler Priority 3:
                                                                bits[23:16] = PendSV (#14)
                                                                bits[31:24] = SysTick (#15) */

/* STM32F030R8 Cortex-M0 implements the top 2 priority bits (4 levels).
 * Writing 0xFF picks the lowest implemented prio regardless of how many
 * bits are wired. */
#define BOARD_PRIO_LOWEST   0xFFu

#ifdef __cplusplus
extern "C" {
#endif

/* Pend the borrowed software-interrupt line. STM32F0 has no STIR (Cortex-M0),
 * so we set the bit in NVIC ISPR directly. */
static inline void TmCauseInterrupt(void)
{
    BOARD_NVIC_ISPR[TM_SW_IRQn >> 5] = (1u << (TM_SW_IRQn & 31u));
    __asm volatile ("dsb" ::: "memory");
}

/* Drop PendSV (#14) and SysTick (#15) to the lowest priority so they cannot
 * preempt anything. Required before TaktOSStart() / vTaskStartScheduler().
 * SHPR3 supports word access on Cortex-M0, so the M4 implementation works
 * unchanged. */
static inline void TmSetKernelPriorities(void)
{
    uint32_t v = BOARD_SCB_SHPR3 & 0x0000FFFFu;     /* clear bits[31:16] */
    v |= ((uint32_t)BOARD_PRIO_LOWEST << 16);        /* PendSV  */
    v |= ((uint32_t)BOARD_PRIO_LOWEST << 24);        /* SysTick */
    BOARD_SCB_SHPR3 = v;
}

/* Configure and enable the borrowed TM5 software-triggered interrupt.
 * ARMv6-M requires word-aligned access to NVIC_IPR -- one IPR word
 * holds four 8-bit priority fields, so we read-modify-write the word
 * containing IRQ N's priority byte at IPR[N >> 2]. */
static inline void TmEnableSoftwareInterrupt(void)
{
    const uint32_t word  = TM_SW_IRQn >> 2;
    const uint32_t shift = (TM_SW_IRQn & 3u) * 8u;

    uint32_t v = BOARD_NVIC_IPR[word];
    v &= ~(0xFFu << shift);
    v |=  ((uint32_t)BOARD_PRIO_LOWEST << shift);
    BOARD_NVIC_IPR[word] = v;

    BOARD_NVIC_ISER[TM_SW_IRQn >> 5] = (1u << (TM_SW_IRQn & 31u));
}

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
