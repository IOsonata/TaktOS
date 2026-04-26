/**-------------------------------------------------------------------------
 * @file	board.h
 * @brief	nRF52832 board definitions for Thread-Metric benchmark.
 *
 * UART console on UARTE0, TXD P0.06 / RXD P0.08 (nRF52 DK VCOM).
 * TM5 SW IRQ on SWI1_EGU1 (IRQ 21) — unused elsewhere.
 *
 * Shared ports (tm_port_*.c) call the arch-neutral TmCauseInterrupt()
 * to pend the TM software IRQ. The implementation lives here per-MCU:
 * Cortex-M uses NVIC_SetPendingIRQ() from CMSIS; a RISC-V board.h
 * would write to CLINT MSIP instead.
 * -------------------------------------------------------------------------*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include "nrf.h"   /* brings in CMSIS core_cm4 (NVIC_SetPendingIRQ) */

/* --- UART pins (nRF52 DK stlink VCOM) --- */
#define UART_RX_PORT    0
#define UART_RX_PIN     8
#define UART_RX_PINOP   0
#define UART_TX_PORT    0
#define UART_TX_PIN     7
#define UART_TX_PINOP   0

#define UART_DEVNO      0    /* UARTE0 */

/* --- Core clock --- */
#define TM_CORE_CLOCK_HZ    64000000u

/* --- TM5 software interrupt --- */
#define TM_SW_IRQn            21u
#define TM_SW_IRQ_VECTOR      SWI1_EGU1_IRQHandler

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

/* --- Bare-metal UART console, TX only, DMA + busy-wait, no IRQ.
 * Matches the known-working 117K reference path exactly. IOsonata's UART
 * class was leaving the RX DMA running which spewed UARTE IRQs and stole
 * ~50% of CPU on this board. This direct path has zero background cost. */
#define UART_BAUDRATE_115200    0x01D7E000u

static inline void BoardConsoleInit(void)
{
    /* Configure TX pin as strong-drive GPIO output, idle-high. */
    NRF_P0->DIRSET  = (1u << UART_TX_PIN);
    NRF_P0->OUTSET  = (1u << UART_TX_PIN);

    /* UARTE0 bare-metal setup: baud, config, pin assignment, enable. */
    NRF_UARTE0->PSEL.TXD  = UART_TX_PIN;
    NRF_UARTE0->PSEL.RXD  = 0xFFFFFFFFu;     /* disconnected  no RX activity */
    NRF_UARTE0->PSEL.RTS  = 0xFFFFFFFFu;
    NRF_UARTE0->PSEL.CTS  = 0xFFFFFFFFu;
    NRF_UARTE0->BAUDRATE  = UART_BAUDRATE_115200;
    NRF_UARTE0->CONFIG    = 0u;
    NRF_UARTE0->INTENCLR  = 0xFFFFFFFFu;     /* no UARTE IRQ */
    NRF_UARTE0->ENABLE    = 8u;              /* UARTE enable */
}

static inline void BoardConsoleWrite(const char *buf, uint32_t len)
{
    /* UARTE TXD.PTR requires a RAM buffer. Function-local static so only
     * one copy exists per TU's inlined instance; all TUs calling this
     * write TX-only, no data race because TX is synchronous (spin on
     * EVENTS_ENDTX before return). */
    static char dma_buf[128] __attribute__((aligned(4)));

    while (len > 0u) {
        uint32_t chunk = (len > sizeof(dma_buf))
                       ? (uint32_t)sizeof(dma_buf) : len;
        for (uint32_t i = 0; i < chunk; i++) dma_buf[i] = buf[i];

        NRF_UARTE0->EVENTS_ENDTX   = 0u;
        NRF_UARTE0->TXD.PTR        = (uint32_t)(uintptr_t)dma_buf;
        NRF_UARTE0->TXD.MAXCNT     = chunk;
        NRF_UARTE0->TASKS_STARTTX  = 1u;
        while (!NRF_UARTE0->EVENTS_ENDTX) { /* spin */ }
        NRF_UARTE0->TASKS_STOPTX   = 1u;

        buf += chunk;
        len -= chunk;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H__ */
