/**
 * @file    system_ra4m1.c
 * @brief   Early system init for Renesas RA4M1: select HOCO (48 MHz) as ICLK
 *          and all peripheral clocks, leave MOCO/SOSC/MOSC in reset state.
 *
 * RA4M1 cold reset defaults:
 *   - CKSEL (SCKSCR.CKSEL) = 0b001 (MOCO @ 8 MHz) → ICLK = 8 MHz / 1 = 8 MHz
 *   - SCKDIVCR = 0 (all peripheral dividers /1)
 *   - HOCO is CONTROLLED BY OFS1 (option function select register in flash).
 *     The Arduino factory OFS1 on Nano R4 / UNO R4 Minima sets HOCOEN=1 so
 *     HOCO is already running at reset. If flashed via SWD without preserving
 *     the factory OFS1 region, HOCO may be disabled — we therefore force it
 *     on here regardless.
 *
 * HOCO output frequency is also OFS1-configured (HOCOFRQ1[2:0]). Arduino
 * factory Nano R4 is 48 MHz (HOCOFRQ1 = 000b). If a user has reprogrammed
 * OFS1 to a different HOCO frequency the value reported in SystemCoreClock
 * will be wrong; override TM_TAKTOS_CORE_CLOCK_HZ in the port if needed.
 *
 * Register access: HOCOCR, SCKDIVCR, SCKSCR, OPCCR are all protected by
 * PRCR.PRC0. We unlock, write, relock. All writes are via explicit
 * addresses — no SoC vendor SDK dependency.
 */

#include <stdint.h>

/* ========================================================================= */
/*  System / clock register map (SYSTEM peripheral base 0x4001E000)          */
/* ========================================================================= */
#define SYSTEM_BASE         0x4001E000UL

/* Clock source select register (8-bit). CKSEL[2:0] at bits [2:0].
 *   000: HOCO
 *   001: MOCO (reset default)
 *   010: LOCO
 *   011: MOSC
 *   100: SOSC
 */
#define SCKSCR_ADDR         (SYSTEM_BASE + 0x026UL)
#define SCKSCR              (*(volatile uint8_t*)SCKSCR_ADDR)

/* System clock divider control register (32-bit). Four-bit fields per
 * divider. All zero = /1 for every clock. */
#define SCKDIVCR_ADDR       (SYSTEM_BASE + 0x020UL)
#define SCKDIVCR            (*(volatile uint32_t*)SCKDIVCR_ADDR)

/* HOCO control register (8-bit). Bit 0 (HCSTP): 0 = HOCO operating,
 * 1 = HOCO stopped. */
#define HOCOCR_ADDR         (SYSTEM_BASE + 0x036UL)
#define HOCOCR              (*(volatile uint8_t*)HOCOCR_ADDR)

/* Oscillator stabilization flag register (8-bit). Bit 0 (HOCOSF): 1 = HOCO
 * is stable and safe to switch to. */
#define OSCSF_ADDR          (SYSTEM_BASE + 0x03CUL)
#define OSCSF               (*(volatile uint8_t*)OSCSF_ADDR)

/* Operating power control register (8-bit). OPCM[1:0] at bits [1:0]:
 *   00: High-speed mode (required for ICLK > 24 MHz).
 *   01: Middle-speed mode.
 *   11: Low-speed mode.
 *
 * After a cold reset OPCM = 0b10 on RA4M1 which is invalid — setting to
 * 0b00 (high-speed) is mandatory before selecting HOCO @ 48 MHz.
 */
#define OPCCR_ADDR          (SYSTEM_BASE + 0x0A0UL)
#define OPCCR               (*(volatile uint8_t*)OPCCR_ADDR)

/* Memory wait cycle control register (8-bit). MEMWAIT[0]: 0 = no wait state,
 * 1 = 1 wait state. RA4M1 requires MEMWAIT = 1 when ICLK > 32 MHz. */
#define MEMWAIT_ADDR        (SYSTEM_BASE + 0x031UL)
#define MEMWAIT             (*(volatile uint8_t*)MEMWAIT_ADDR)

/* Register write protection (16-bit).
 *   [15:8] = key — must be 0xA5 to accept the write.
 *   [0]    = PRC0 — protect clocks (SCKDIVCR, SCKSCR, HOCOCR, OPCCR, ...).
 *   [1]    = PRC1 — protect low-power modes.
 *   [3]    = PRC3 — protect LVD.
 */
#define PRCR_ADDR           (SYSTEM_BASE + 0x3FEUL)
#define PRCR                (*(volatile uint16_t*)PRCR_ADDR)

#define PRCR_KEY            (0xA500U)
#define PRCR_PRC0           (1U << 0)

/* ========================================================================= */
/*  Exported globals                                                          */
/* ========================================================================= */
uint32_t SystemCoreClock = 48000000UL;

/* ========================================================================= */
/*  SystemInit — called from Reset_Handler before __libc_init_array.         */
/* ========================================================================= */
void SystemInit(void)
{
    /* Unlock clock + OPCCR registers. */
    PRCR = PRCR_KEY | PRCR_PRC0;

    /* Step 1: high-speed operating power mode.
     * OPCCR write is only accepted while the other OPCCR bits indicate no
     * mode transition is in progress; writing 0x00 is unconditional. */
    OPCCR = 0x00u;
    /* Wait for mode transition bit to clear (OPCMTSF at bit 4). */
    while ((OPCCR & (1u << 4)) != 0u) { /* spin until steady */ }

    /* Step 2: one flash wait state (required above 32 MHz). */
    MEMWAIT = 0x01u;

    /* Step 3: turn on HOCO if not already running, then wait for stability.
     * HOCOCR.HCSTP = 0 means HOCO runs. Factory Arduino firmware has HOCO
     * already enabled via OFS1; writing 0 here is idempotent in that case. */
    HOCOCR = 0x00u;
    while ((OSCSF & 0x01u) == 0u) { /* wait for HOCOSF = 1 */ }

    /* Step 4: all peripheral dividers /1 → ICLK = PCLKA = PCLKB = PCLKC =
     * PCLKD = FCLK = HOCO (48 MHz). */
    SCKDIVCR = 0x00000000UL;

    /* Step 5: switch clock source to HOCO. CKSEL = 0b000. */
    SCKSCR = 0x00u;

    /* Relock clock registers. */
    PRCR = PRCR_KEY;

    SystemCoreClock = 48000000UL;
}
