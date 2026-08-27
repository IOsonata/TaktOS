/**
 * @file    system_ra4m1.c  (DEBUG-INSTRUMENTED VERSION)
 * @brief   Early system init for Renesas RA4M1 — Arduino Nano R4.
 *
 * This version adds visual debugging via the on-board orange LED
 * (LED_BUILTIN = D22 = RA4M1 P204). Each milestone emits ONE slow pulse
 * (~600 ms on, ~2 s off). Count the number of pulses before the LED
 * freezes (either solid-on = fault, or dark = hang).
 *
 *   Pulses seen before freeze / heartbeat:
 *     0 = hung before Reset_Handler reached SystemInit (startup issue)
 *     1 = hung in PRCR unlock / OPCCR write
 *     2 = hung in OPCMTSF spin
 *     3 = hung in HOCO stability wait
 *     4 = hung in SCKSCR switch (unlikely)
 *     5 = SystemInit finished — then milestones 6..9 from startup/main
 *
 * You just count long pulses. No rapid sequences, no "is that 4 or 5?" —
 * each pulse is clearly separated by 2 seconds of darkness.
 *
 * Remove this instrumentation for benchmark runs; it alters timing.
 */

#include <stdint.h>

/* ========================================================================= */
/*  System / clock register map (SYSTEM peripheral base 0x4001E000)          */
/* ========================================================================= */
#define SYSTEM_BASE         0x4001E000UL

#define SCKSCR_ADDR         (SYSTEM_BASE + 0x026UL)
#define SCKSCR              (*(volatile uint8_t*)SCKSCR_ADDR)

#define SCKDIVCR_ADDR       (SYSTEM_BASE + 0x020UL)
#define SCKDIVCR            (*(volatile uint32_t*)SCKDIVCR_ADDR)

#define HOCOCR_ADDR         (SYSTEM_BASE + 0x036UL)
#define HOCOCR              (*(volatile uint8_t*)HOCOCR_ADDR)

#define OSCSF_ADDR          (SYSTEM_BASE + 0x03CUL)
#define OSCSF               (*(volatile uint8_t*)OSCSF_ADDR)

#define OPCCR_ADDR          (SYSTEM_BASE + 0x0A0UL)
#define OPCCR               (*(volatile uint8_t*)OPCCR_ADDR)

#define MEMWAIT_ADDR        (SYSTEM_BASE + 0x031UL)
#define MEMWAIT             (*(volatile uint8_t*)MEMWAIT_ADDR)

#define PRCR_ADDR           (SYSTEM_BASE + 0x3FEUL)
#define PRCR                (*(volatile uint16_t*)PRCR_ADDR)

#define PRCR_KEY            (0xA500U)
#define PRCR_PRC0           (1U << 0)   /* clock-gen regs (SCKDIVCR/SCKSCR/HOCOCR/...) */
#define PRCR_PRC1           (1U << 1)   /* low-power regs (MSTPCRA/B/C + OPCCR) */

/* ========================================================================= */
/*  LED debug — P204 (D22, LED_BUILTIN)                                       */
/*                                                                            */
/*  PORT0..9 registers are memory-mapped starting at 0x40040000 with a       */
/*  0x20-byte stride. PORT2 base = 0x40040040.                               */
/*                                                                            */
/*    PDR  (offset 0x00, 16-bit): 0 = input, 1 = output                      */
/*    PODR (offset 0x02, 16-bit): output data                                */
/*    PIDR (offset 0x04, 16-bit): input data                                 */
/*                                                                            */
/*  P204 = PORT2 bit 4 -> mask = (1 << 4) = 0x0010.                          */
/* ========================================================================= */
#define PORT2_BASE          0x40040040UL
#define PORT2_PDR           (*(volatile uint16_t*)(PORT2_BASE + 0x00U))
#define PORT2_PODR          (*(volatile uint16_t*)(PORT2_BASE + 0x02U))
#define LED_PIN_MASK        (1U << 4)

/* PFS register for P204:
 *   0x40040800 + port_num*0x40 + pin_num*0x04
 *   = 0x40040800 + 2*0x40 + 4*0x04 = 0x40040890 */
#define P204PFS_ADDR        (0x40040800UL + 0x40U*2U + 0x04U*4U)
#define P204PFS             (*(volatile uint32_t*)P204PFS_ADDR)

/* PFS write-protect register. */
#define PWPR_ADDR           0x40040D03UL
#define PWPR                (*(volatile uint8_t*)PWPR_ADDR)
#define PWPR_B0WI           (1U << 7)
#define PWPR_PFSWE          (1U << 6)

static inline void debug_busywait(uint32_t cycles)
{
    for (volatile uint32_t i = 0; i < cycles; ++i) { __asm__ volatile ("nop"); }
}

static void debug_led_init(void)
{
    /* Unlock PFS, reset P204 to GPIO output mode. */
    PWPR = 0x00U;
    PWPR = PWPR_PFSWE;
    P204PFS = 0x00000000U;           /* PMR=0 (GPIO), all other fields 0 */
    PWPR = PWPR_B0WI;

    PORT2_PDR  |= LED_PIN_MASK;      /* P204 as output */
    PORT2_PODR &= ~LED_PIN_MASK;     /* start LOW (LED off) */
}

/* ========================================================================= */
/*  Exported globals                                                          */
/* ========================================================================= */
uint32_t SystemCoreClock = 48000000UL;

/* ========================================================================= */
/*  SystemInit — called from Reset_Handler before __libc_init_array.         */
/* ========================================================================= */
void SystemInit(void)
{
    /* LED init runs first so we can see anything at all. */
    debug_led_init();

    /* Unlock clock (PRC0) AND low-power (PRC1) registers. */
    PRCR = PRCR_KEY | PRCR_PRC0 | PRCR_PRC1;

    /* Step 1: high-speed operating power mode. */
    OPCCR = 0x00u;
    while ((OPCCR & (1u << 4)) != 0u) { /* spin until OPCMTSF = 0 */ }

    /* Step 2: one flash wait state (required above 32 MHz). */
    MEMWAIT = 0x01u;

    /* Step 3: turn on HOCO if not already running, wait for stability. */
    HOCOCR = 0x00u;
    while ((OSCSF & 0x01u) == 0u) { /* wait for HOCOSF = 1 */ }

    /* Step 4: all peripheral dividers /1 → 48 MHz everywhere. */
    SCKDIVCR = 0x00000000UL;

    /* Step 5: switch clock source to HOCO. */
    SCKSCR = 0x00u;

    /* Relock clock registers. */
    PRCR = PRCR_KEY;

    SystemCoreClock = 48000000UL;
}
