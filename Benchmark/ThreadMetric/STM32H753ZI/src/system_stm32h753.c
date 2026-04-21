/**
 * @file    system_stm32h753.c
 * @brief   Early system init for STM32H753ZI on NUCLEO-H753ZI.
 *
 * Clock strategy: leave the reset defaults alone — HSI @ 64 MHz, all
 * peripheral prescalers at /1, SYSCLK = HCLK = PCLK1 = PCLK2 = 64 MHz.
 * No PLL bring-up, no HSE switch. This is the simplest deterministic
 * configuration and is plenty fast for the benchmark.
 *
 * What we DO configure:
 *   1. Flash ACR: LATENCY=1, WRHIGHFREQ=0 — correct for VOS3 @ up to 70 MHz.
 *      (Reset default programs LATENCY=7 which works but is unnecessarily
 *      slow.)
 *   2. I-cache enable — one bit + barriers, no invalidation needed (the
 *      cache contents are undefined but the tag RAM is cleared at reset so
 *      all lines are invalid).
 *   3. D-cache enable — same reasoning. For a cold reset no invalidation
 *      is strictly required; doing it anyway via SCB_DCISW keeps the code
 *      safe against warm resets or debug-session restarts where lines may
 *      have been marked valid.
 *
 * What we do NOT configure:
 *   - Voltage scaling (VOS). Reset default VOS3 is fine for ≤ 70 MHz.
 *   - FPU (CPACR CP10/CP11). This is a soft-float build — enabling the
 *     FPU would allow the compiler to emit VFP instructions that the
 *     library was not built with.
 *   - MPU. TaktOS's optional per-task MPU guard regions are a library
 *     build option, not an application init responsibility.
 *
 * All peripheral registers accessed directly — no Cube HAL dependency.
 */

#include <stdint.h>

/* ========================================================================= */
/*  Register definitions (addresses from RM0433)                              */
/* ========================================================================= */
#define FLASH_ACR_ADDR        0x52002000UL   /* embedded FLASH base + 0 */
#define FLASH_ACR             (*(volatile uint32_t*)FLASH_ACR_ADDR)
#define FLASH_ACR_LATENCY_Msk (0x0FU << 0)
#define FLASH_ACR_WRHIGHFREQ_Msk (0x03U << 4)
#define FLASH_ACR_LATENCY_1WS (1U << 0)

#define SCB_CCR_ADDR          0xE000ED14UL
#define SCB_CCR               (*(volatile uint32_t*)SCB_CCR_ADDR)
#define SCB_CCR_IC_Msk        (1U << 17)
#define SCB_CCR_DC_Msk        (1U << 16)

#define SCB_ICIALLU_ADDR      0xE000EF50UL   /* I-cache invalidate all to PoU */
#define SCB_ICIALLU           (*(volatile uint32_t*)SCB_ICIALLU_ADDR)

#define SCB_CCSIDR_ADDR       0xE000ED80UL
#define SCB_CCSIDR            (*(volatile uint32_t*)SCB_CCSIDR_ADDR)
#define SCB_CSSELR_ADDR       0xE000ED84UL
#define SCB_CSSELR            (*(volatile uint32_t*)SCB_CSSELR_ADDR)
#define SCB_DCISW_ADDR        0xE000EF60UL   /* D-cache invalidate by set/way */
#define SCB_DCISW             (*(volatile uint32_t*)SCB_DCISW_ADDR)

/* ========================================================================= */
/*  Globals                                                                   */
/* ========================================================================= */
uint32_t SystemCoreClock = 64000000UL;    /* HSI default on STM32H7 */

/* ========================================================================= */
/*  Helpers                                                                   */
/* ========================================================================= */
static inline void dsb_isb(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}

static void enable_icache(void)
{
    dsb_isb();
    SCB_ICIALLU = 0;            /* invalidate I-cache to Point of Unification */
    dsb_isb();
    SCB_CCR |= SCB_CCR_IC_Msk;  /* enable */
    dsb_isb();
}

/* Cortex-M7 D-cache invalidate by set/way.
 *
 * Read CCSIDR for the cache geometry:
 *   NumSets = CCSIDR[27:13] + 1
 *   Assoc   = CCSIDR[12:3]  + 1       (number of ways)
 *   LineSize = 2^(CCSIDR[2:0] + 4)    bytes
 *
 * DCISW register layout: [31..Way_Pos] = way index, [Set_Pos..Line_Pos] = set,
 * where Way_Pos = 32 - log2(Assoc) and Set_Pos = log2(LineSize).
 *
 * On STM32H7 (M7): LineSize = 32 B (log2=5), Sets = 128, Ways = 2
 * → Way_Pos = 31, Set_Pos = 5.
 */
static void invalidate_dcache(void)
{
    uint32_t ccsidr, sets, ways;
    int log2_line, log2_ways_pos;

    /* Select L1 data cache (CSSELR: Level=0, InD=0). */
    SCB_CSSELR = 0;
    dsb_isb();

    ccsidr    = SCB_CCSIDR;
    sets      = ((ccsidr >> 13) & 0x7FFFU) + 1U;
    ways      = ((ccsidr >> 3)  & 0x3FFU)  + 1U;
    log2_line = (int)(ccsidr & 0x7U) + 4;

    /* log2(ways) rounded up via count-leading-zeros */
    log2_ways_pos = 32 - __builtin_clz(ways - 1U);
    if (log2_ways_pos < 0) log2_ways_pos = 0;
    log2_ways_pos = 32 - log2_ways_pos;

    for (uint32_t s = 0; s < sets; ++s) {
        for (uint32_t w = 0; w < ways; ++w) {
            SCB_DCISW = (w << log2_ways_pos) | (s << log2_line);
        }
    }
    dsb_isb();
}

static void enable_dcache(void)
{
    invalidate_dcache();
    SCB_CCR |= SCB_CCR_DC_Msk;
    dsb_isb();
}

/* ========================================================================= */
/*  SystemInit — called from Reset_Handler before __libc_init_array.         */
/* ========================================================================= */
void SystemInit(void)
{
    /* Flash latency: 1 WS is correct for VOS3 at up to 70 MHz. The reset
     * default is LATENCY=7 (safe-but-slow); reducing it gives a real
     * performance lift. WRHIGHFREQ stays at 0 for this frequency band. */
    uint32_t acr = FLASH_ACR;
    acr &= ~(FLASH_ACR_LATENCY_Msk | FLASH_ACR_WRHIGHFREQ_Msk);
    acr |=  FLASH_ACR_LATENCY_1WS;
    FLASH_ACR = acr;
    /* Read back to confirm the write landed before proceeding. */
    while ((FLASH_ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_1WS) { }

    enable_icache();
    enable_dcache();

    SystemCoreClock = 64000000UL;
}
