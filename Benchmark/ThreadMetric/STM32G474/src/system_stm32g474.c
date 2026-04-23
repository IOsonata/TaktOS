/**
 * @file    system_stm32g474.c
 * @brief   Clock and flash bring-up for STM32G474RE on NUCLEO-G474RE.
 *
 * Target:  SYSCLK = HCLK = PCLK1 = PCLK2 = 170 MHz.
 * Source:  HSI16 (16 MHz internal RC) -> PLLM=4 -> 4 MHz -> PLLN=85 ->
 *          340 MHz VCO -> PLLR=2 -> 170 MHz.
 * Power:   VOS1 Boost (required for SYSCLK > 150 MHz).
 * Flash:   4 WS (RM0440 Rev >=3 Table 9, VOS1 Boost, 150 < f <= 170 MHz).
 *
 * CRITICAL — STM32G4 170 MHz requires a specific boost-enable sequence
 * (RM0440 §6.1.5 "Range 1 boost mode"). If you switch SYSCLK straight to a
 * 170 MHz PLL with Boost bit just toggled, the switch silently fails and
 * SWS never reaches PLL (infinite spin). Steps that must be followed:
 *
 *   1. Enable PWR clock, set VOS=1.
 *   2. Wait for VOSF=0 in PWR_SR2.
 *   3. Enable Boost (R1MODE=0) while still on HSI (SYSCLK=16 MHz).
 *   4. Set flash latency to final value BEFORE SYSCLK increase.
 *   5. Configure + start PLL, wait for PLLRDY.
 *   6. Set HPRE=/2 first (so HCLK is temporarily ≤85 MHz across the switch).
 *   7. Switch SYSCLK to PLL, wait for SWS=PLL.
 *   8. Wait ≥1 μs at /2 HCLK (per RM0440 boost sequence).
 *   9. Restore HPRE=/1 — now HCLK = 170 MHz.
 *
 * The HPRE dance around the switch is what CubeMX's
 * HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST)
 * does behind the scenes.
 *
 * Beningo-parity (80 MHz): drop PLLN to 40, flash latency to 2 WS, skip
 * Boost entirely. One place to change if you ever need it.
 */

#include <stdint.h>

/* ---------------------------- RCC --------------------------------- */
#define RCC_BASE           0x40021000UL
#define RCC_CR             (*(volatile uint32_t*)(RCC_BASE + 0x00))
#define RCC_ICSCR          (*(volatile uint32_t*)(RCC_BASE + 0x04))
#define RCC_CFGR           (*(volatile uint32_t*)(RCC_BASE + 0x08))
#define RCC_PLLCFGR        (*(volatile uint32_t*)(RCC_BASE + 0x0C))
#define RCC_APB1ENR1       (*(volatile uint32_t*)(RCC_BASE + 0x58))

#define RCC_CR_HSION       (1U << 8)
#define RCC_CR_HSIRDY      (1U << 10)
#define RCC_CR_PLLON       (1U << 24)
#define RCC_CR_PLLRDY      (1U << 25)

/* RCC_CFGR SW/SWS encoding — STM32G4 (RM0440 §7.4.3):
 *   SW  [1:0]   : 00 HSI, 01 HSE, 11 PLL, 10 reserved  *no* — actually
 *                 00 HSI, 01 HSE, 11 PLL on G4 (same encoding as L4+/L5/U5).
 *   SWS [3:2]   : mirrors SW. */
#define RCC_CFGR_SW_MSK    (3U << 0)
#define RCC_CFGR_SW_PLL    (3U << 0)
#define RCC_CFGR_SWS_MSK   (3U << 2)
#define RCC_CFGR_SWS_PLL   (3U << 2)

/* HPRE encoding (4 bits [7:4]):
 *   0xxx = /1 (divider not applied), 1000 = /2, 1001 = /4, ..., 1111 = /512 */
#define RCC_CFGR_HPRE_MSK  (0xFU << 4)
#define RCC_CFGR_HPRE_DIV1 (0x0U << 4)
#define RCC_CFGR_HPRE_DIV2 (0x8U << 4)

/* RCC_PLLCFGR layout (RM0440 §7.4.4 — STM32G4):
 *   [1:0]   PLLSRC  (00 none, 10 HSI16, 11 HSE)
 *   [7:4]   PLLM    (register value = divider-1, so 0011 = /4)
 *   [14:8]  PLLN    (multiplier, 8..127)
 *   [16]    PLLPEN
 *   [17]    PLLP    (on G4 1 bit; /7 or /17)
 *   [20]    PLLQEN
 *   [22:21] PLLQ    (00=/2, 01=/4, 10=/6, 11=/8)
 *   [24]    PLLREN  <-- bit 24, NOT bit 28 (L4 muscle memory trap)
 *   [26:25] PLLR    (00=/2, 01=/4, 10=/6, 11=/8)
 *   [31:27] PLLPDIV (alternate P divider, 0 = use PLLP field) */
#define PLLCFGR_PLLSRC_HSI16 (2U << 0)
#define PLLCFGR_PLLM(m)      (((m)-1u) << 4)
#define PLLCFGR_PLLN(n)      ((n) << 8)
#define PLLCFGR_PLLREN       (1U << 24)
#define PLLCFGR_PLLR_DIV2    (0U << 25)



/* ---------------------------- PWR --------------------------------- */
#define PWR_BASE           0x40007000UL
#define PWR_CR1            (*(volatile uint32_t*)(PWR_BASE + 0x00))
#define PWR_CR5            (*(volatile uint32_t*)(PWR_BASE + 0x80))
#define PWR_SR2            (*(volatile uint32_t*)(PWR_BASE + 0x18))
#define PWR_CR1_VOS_MSK    (3U << 9)
#define PWR_CR1_VOS_RANGE1 (1U << 9)     /* VOS1 */
#define PWR_CR5_R1MODE     (1U << 0)     /* 0 = Boost, 1 = Normal */
#define PWR_SR2_VOSF       (1U << 10)    /* voltage scaling flag */
#define RCC_APB1ENR1_PWREN (1U << 28)

/* ---------------------------- FLASH ------------------------------- */
#define FLASH_BASE_REG     0x40022000UL
#define FLASH_ACR          (*(volatile uint32_t*)(FLASH_BASE_REG + 0x00))
#define FLASH_ACR_LATENCY  (0xFU << 0)
#define FLASH_ACR_PRFTEN   (1U << 8)
#define FLASH_ACR_ICEN     (1U << 9)
#define FLASH_ACR_DCEN     (1U << 10)
#define FLASH_ACR_4WS      (4U << 0)

/* ---------------------------- Globals ----------------------------- */
uint32_t SystemCoreClock = 170000000UL;

/* Rough busy-wait for the boost-mode settling time.
 * At 16 MHz HSI, one loop iteration ≈ 3 cycles; 10000 iterations ≈ ~2 ms,
 * well above the 1 μs required by RM0440 boost transition. Conservative. */
static void boost_settle_delay(void)
{
    for (volatile uint32_t i = 0; i < 20000u; ++i) { __asm volatile("nop"); }
}

/* ---------------------------- SystemInit -------------------------- */
void SystemInit(void)
{
    /* 1. HSI already on by reset, but make it explicit. */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & RCC_CR_HSIRDY)) { }

    /* 2. Enable PWR clock. */
    RCC_APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC_APB1ENR1;

    /* 3. Set VOS = Range 1. Wait for VOSF to clear. */
    PWR_CR1 = (PWR_CR1 & ~PWR_CR1_VOS_MSK) | PWR_CR1_VOS_RANGE1;
    while (PWR_SR2 & PWR_SR2_VOSF) { }

    /* 4. Enable Boost mode (R1MODE=0) while SYSCLK is still on HSI (16 MHz). */
    PWR_CR5 &= ~PWR_CR5_R1MODE;

    /* 5. Set flash latency to 4 WS + enable prefetch + I-cache + D-cache
     *    BEFORE raising SYSCLK. At HSI=16 MHz, 4 WS is over-spec but safe. */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY)
              | FLASH_ACR_4WS
              | FLASH_ACR_PRFTEN
              | FLASH_ACR_ICEN
              | FLASH_ACR_DCEN;
    while ((FLASH_ACR & FLASH_ACR_LATENCY) != FLASH_ACR_4WS) { }

    /* 6. Configure PLL: HSI16 / PLLM=4 = 4 MHz; ×PLLN=85 = 340 MHz VCO;
     *    /PLLR=2 = 170 MHz SYSCLK. Enable R output. */
    RCC_CR &= ~RCC_CR_PLLON;
    while (RCC_CR & RCC_CR_PLLRDY) { }

    RCC_PLLCFGR = PLLCFGR_PLLSRC_HSI16
                | PLLCFGR_PLLM(4)
                | PLLCFGR_PLLN(85)
                | PLLCFGR_PLLR_DIV2
                | PLLCFGR_PLLREN;

    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY)) { }

    /* 7. Boost transition: set HPRE=/2 BEFORE switching to PLL, so HCLK
     *    will be 170/2 = 85 MHz immediately after the switch — safe at
     *    VOS1 Boost without the final settling delay. */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_HPRE_MSK) | RCC_CFGR_HPRE_DIV2;

    /* 8. Switch SYSCLK source to PLL. Wait for SWS confirmation. */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MSK) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MSK) != RCC_CFGR_SWS_PLL) { }

    /* 9. Wait ≥1 μs at reduced HCLK per RM0440 boost sequence. */
    boost_settle_delay();

    /* 10. Restore HPRE=/1 — HCLK = PCLK1 = PCLK2 = 170 MHz. */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_HPRE_MSK) | RCC_CFGR_HPRE_DIV1;

    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    SystemCoreClock = 170000000UL;
}
