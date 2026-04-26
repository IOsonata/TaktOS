/**-------------------------------------------------------------------------
 * @file    board.h
 * @brief   nRF54L15 board definitions for Thread-Metric benchmark.
 *
 * UART console on UARTE30, TXD P0.04 / RXD P0.05 (nRF54L15-DK VCOM).
 * TM5 SW IRQ on SWI00 (IRQ 28)  unused elsewhere on the test board.
 *
 * Shared ports (tm_port_*.c) call the arch-neutral TmCauseInterrupt()
 * to pend the TM software IRQ. The implementation lives here per-MCU:
 * Cortex-M33 uses NVIC_SetPendingIRQ() from CMSIS; a RISC-V board.h
 * would write to CLINT MSIP instead.
 *
 * NB: UART_DEVNO is the IOsonata enumeration value, NOT the nRF54L15
 * silicon instance number. If your IOsonata build maps UARTE30 to a
 * different DevNo, edit the single line below.
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include "nrf.h"   /* brings in CMSIS core_cm33 (NVIC_SetPendingIRQ) */

/* --- UART pins (nRF54L15-DK VCOM via UARTE30) --- */
#define UART_RX_PORT    0
#define UART_RX_PIN     1
#define UART_RX_PINOP   0
#define UART_TX_PORT    0
#define UART_TX_PIN     0
#define UART_TX_PINOP   0

#define UART_DEVNO      0   /* UARTE30  IOsonata DevNo for the nRF54L15 console UART */

/* --- Core clock --- */
#define TM_CORE_CLOCK_HZ    128000000u

/* --- TM5 software interrupt --- */
#define TM_SW_IRQn            28u                /* SWI00 */
#define TM_SW_IRQ_VECTOR      SWI00_IRQHandler

/* --- Arch-neutral SW IRQ pend (called from shared tm_port_*.c) --- */
#ifdef __cplusplus
extern "C" {
#endif

static inline void TmCauseInterrupt(void)
{
    NVIC_SetPendingIRQ((IRQn_Type)TM_SW_IRQn);
}

/* Lower PendSV/SysTick to the minimum priority so they cannot preempt
 * anything. Required before vTaskStartScheduler() on Cortex-M; a RISC-V
 * board.h would be a no-op or CLINT priority setup. */
static inline void TmSetKernelPriorities(void)
{
    NVIC_SetPriority(PendSV_IRQn,  (1u << __NVIC_PRIO_BITS) - 1u);
    NVIC_SetPriority(SysTick_IRQn, (1u << __NVIC_PRIO_BITS) - 1u);
}

/* Configure and enable the TM5 software-triggered interrupt.
 * Priority must be >= configMAX_SYSCALL_INTERRUPT_PRIORITY so the handler
 * can call FreeRTOS FromISR APIs (numerically >= for Cortex-M). */
static inline void TmEnableSoftwareInterrupt(void)
{
    NVIC_SetPriority((IRQn_Type)TM_SW_IRQn,
                     (1u << __NVIC_PRIO_BITS) - 1u);
    NVIC_EnableIRQ((IRQn_Type)TM_SW_IRQn);
}

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
