/**-------------------------------------------------------------------------
 * @file    board.h
 * @brief   ATSAM4LC8C board definitions for Thread-Metric benchmark.
 *
 * Cortex-M4 @ 48 MHz, no FPU, soft-float ABI (the C variant of SAM4L
 * has no FPU on silicon).
 *
 * UART console on USART1 via IOsonata (DevNo = 1).
 *   PC27 -> USART1 TXD  (function B = PINOP 1)
 *   PC26 -> USART1 RXD  (function B = PINOP 1)
 *
 * Wiring matches SAM4L8 Xplained Pro EDBG Virtual COM Port. If your
 * board uses a different USART/pin/PINOP mapping, edit the UART_*
 * macros below.
 *
 * TM5 software interrupt borrows the TRNG NVIC line (IRQ 73). The TRNG
 * peripheral itself is never clocked or configured by the benchmark;
 * only its NVIC slot is used. Because none of the seven Thread-Metric
 * tests in scope (Basic, Cooperative, Preemptive, Message, Sync, Mutex,
 * MutexBarging) calls tm_cause_interrupt(), the TRNG_Handler vector
 * remains the IOsonata weak default at runtime. The macros are kept
 * here for symmetry with the nRF52832 / nRF54L15 ports.
 *
 * NB: this header intentionally does NOT include a SAM4L CMSIS umbrella
 * (no "sam.h"). The NVIC and System Control Block live at fixed
 * architectural addresses defined by ARM for every Cortex-M4
 * implementation, so the four register accesses below are portable
 * without pulling in any vendor-specific device header. This matches
 * the approach the previous SAM4L Thread-Metric port took (now under
 * src/legacy/) and avoids depending on whatever device-pack layout the
 * user's IOsonata installation happens to use.
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdint.h>

/* --- UART pins (SAM4L8 Xplained Pro EDBG VCOM via USART1) --- */
#define UART_RX_PORT    2       /* PortC: A=0, B=1, C=2 */
#define UART_RX_PIN     26      /* PC26 -> USART1 RXD */
#define UART_RX_PINOP   1       /* function B (start with B = 1; try A = 0 if no output) */
#define UART_TX_PORT    2
#define UART_TX_PIN     27      /* PC27 -> USART1 TXD */
#define UART_TX_PINOP   1

#define UART_DEVNO      1       /* USART1 -- IOsonata DevNo for the SAM4L console UART */

/* --- Core clock (post IOsonata SystemInit) --- */
#define TM_CORE_CLOCK_HZ    48000000u

/* --- TM5 software interrupt --- */
/* SAM4LC8C peripheral interrupt numbers (from device datasheet, Section 9 NVIC).
 * TRNG is borrowed because (a) IOsonata's SystemInit leaves its peripheral
 * clock gated off, (b) it cannot raise its line on its own, (c) its NVIC
 * vector name TRNG_Handler is a weak default in IOsonata's startup. */
#define TM_SW_IRQn            73u
#define TM_SW_IRQ_VECTOR      TRNG_Handler

/* --- ARMv7-M architectural register addresses (Cortex-M4 baseline) ---
 * These are defined by the ARM ARM v7-M, not by Microchip, so they are
 * the same on every Cortex-M4 silicon. */
#define BOARD_NVIC_ISER ((volatile uint32_t *) 0xE000E100u)  /* Set-Enable */
#define BOARD_NVIC_ISPR ((volatile uint32_t *) 0xE000E200u)  /* Set-Pending */
#define BOARD_NVIC_IPR  ((volatile uint8_t  *) 0xE000E400u)  /* Priority (one byte per IRQ) */
#define BOARD_SCB_SHPR3 (*(volatile uint32_t *) 0xE000ED20u) /* System Handler Priority 3:
                                                                bits[23:16] = PendSV (#14)
                                                                bits[31:24] = SysTick (#15) */

/* SAM4L Cortex-M4 implements the top 4 priority bits (16 levels). The lowest
 * priority is therefore 0xF0 in the 8-bit NVIC_IPR/SHPR fields. */
#define BOARD_PRIO_LOWEST   0xFFu   /* writing 0xFF picks the lowest implemented prio
                                       regardless of how many bits are wired */

#ifdef __cplusplus
extern "C" {
#endif

/* Pend the borrowed software-interrupt line. SAM4L does not implement STIR,
 * so we set the bit in NVIC ISPR directly. */
static inline void TmCauseInterrupt(void)
{
    BOARD_NVIC_ISPR[TM_SW_IRQn >> 5] = (1u << (TM_SW_IRQn & 31u));
    __asm volatile ("dsb" ::: "memory");
}

/* Drop PendSV (#14) and SysTick (#15) to the lowest priority so they cannot
 * preempt anything. Required before TaktOSStart() / vTaskStartScheduler(). */
static inline void TmSetKernelPriorities(void)
{
    uint32_t v = BOARD_SCB_SHPR3 & 0x0000FFFFu;     /* clear bits[31:16] */
    v |= ((uint32_t)BOARD_PRIO_LOWEST << 16);        /* PendSV  */
    v |= ((uint32_t)BOARD_PRIO_LOWEST << 24);        /* SysTick */
    BOARD_SCB_SHPR3 = v;
}

/* Configure and enable the borrowed TM5 software-triggered interrupt.
 * Priority must be >= configMAX_SYSCALL_INTERRUPT_PRIORITY so the handler
 * can call FreeRTOS FromISR APIs (numerically >= for Cortex-M). The same
 * numeric priority is used on TaktOS for symmetry across the ports. */
static inline void TmEnableSoftwareInterrupt(void)
{
    BOARD_NVIC_IPR[TM_SW_IRQn]       = BOARD_PRIO_LOWEST;
    BOARD_NVIC_ISER[TM_SW_IRQn >> 5] = (1u << (TM_SW_IRQn & 31u));
}

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
