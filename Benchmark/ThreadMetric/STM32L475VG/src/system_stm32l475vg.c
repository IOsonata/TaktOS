/**
 * @file    system_stm32l475vg.c
 * @brief   Early system init for STM32L475VG on B-L475E-IOT01A2.
 *
 *   - Voltage scaling: Range 1 (required for SYSCLK > 26 MHz per RM0351 §5.1.5)
 *   - PLL source: HSI16 (16 MHz internal RC)
 *   - PLL config: /M=1, *N=10, /R=2  =>  16 MHz / 1 * 10 / 2 = 80 MHz SYSCLK
 *   - Flash:      4 wait states (required for 72 < SYSCLK <= 80 MHz, VCORE=Range 1)
 *   - AHB, APB1, APB2 prescalers = 1
 *   - ICache + prefetch enabled
 *
 * Resulting SystemCoreClock = 80 MHz, HCLK = PCLK1 = PCLK2 = 80 MHz.
 *
 * This file intentionally does not use CMSIS / STM32 HAL — the goal is a
 * completely self-contained startup that can be later folded into
 * IOsonata/ARM/ST/STM32L4xx/STM32L475xx/ without pulling in HAL dependencies.
 */

#include <stdint.h>

/* ------ Peripheral bases (RM0351 §2.2.2 Memory map) ---------------------- */
#define PERIPH_BASE         0x40000000UL
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define FLASH_R_BASE        (AHB1PERIPH_BASE + 0x2000UL)   /* 0x40022000 */
#define RCC_BASE            (AHB1PERIPH_BASE + 0x1000UL)   /* 0x40021000 */
#define PWR_BASE            (PERIPH_BASE + 0x00007000UL)   /* 0x40007000 */

typedef struct {
    volatile uint32_t ACR;           /* 0x00 Flash access control */
    volatile uint32_t PDKEYR;        /* 0x04 */
    volatile uint32_t KEYR;          /* 0x08 */
    volatile uint32_t OPTKEYR;       /* 0x0C */
    volatile uint32_t SR;            /* 0x10 */
    volatile uint32_t CR;            /* 0x14 */
    volatile uint32_t ECCR;          /* 0x18 */
    volatile uint32_t RESERVED1;     /* 0x1C */
    volatile uint32_t OPTR;          /* 0x20 */
} FLASH_TypeDef;

typedef struct {
    volatile uint32_t CR;            /* 0x00 */
    volatile uint32_t ICSCR;         /* 0x04 */
    volatile uint32_t CFGR;          /* 0x08 */
    volatile uint32_t PLLCFGR;       /* 0x0C */
    volatile uint32_t PLLSAI1CFGR;   /* 0x10 */
    volatile uint32_t RESERVED0;     /* 0x14 */
    volatile uint32_t CIER;          /* 0x18 */
    volatile uint32_t CIFR;          /* 0x1C */
    volatile uint32_t CICR;          /* 0x20 */
    volatile uint32_t RESERVED1;     /* 0x24 */
    volatile uint32_t AHB1RSTR;      /* 0x28 */
    volatile uint32_t AHB2RSTR;      /* 0x2C */
    volatile uint32_t AHB3RSTR;      /* 0x30 */
    volatile uint32_t RESERVED2;     /* 0x34 */
    volatile uint32_t APB1RSTR1;     /* 0x38 */
    volatile uint32_t APB1RSTR2;     /* 0x3C */
    volatile uint32_t APB2RSTR;      /* 0x40 */
    volatile uint32_t RESERVED3;     /* 0x44 */
    volatile uint32_t AHB1ENR;       /* 0x48 */
    volatile uint32_t AHB2ENR;       /* 0x4C */
    volatile uint32_t AHB3ENR;       /* 0x50 */
    volatile uint32_t RESERVED4;     /* 0x54 */
    volatile uint32_t APB1ENR1;      /* 0x58 */
    volatile uint32_t APB1ENR2;      /* 0x5C */
    volatile uint32_t APB2ENR;       /* 0x60 */
} RCC_TypeDef;

typedef struct {
    volatile uint32_t CR1;           /* 0x00 */
    volatile uint32_t CR2;           /* 0x04 */
    volatile uint32_t CR3;           /* 0x08 */
    volatile uint32_t CR4;           /* 0x0C */
    volatile uint32_t SR1;           /* 0x10 */
    volatile uint32_t SR2;           /* 0x14 */
    volatile uint32_t SCR;           /* 0x18 */
} PWR_TypeDef;

#define FLASH   ((FLASH_TypeDef*)FLASH_R_BASE)
#define RCC     ((RCC_TypeDef*)RCC_BASE)
#define PWR     ((PWR_TypeDef*)PWR_BASE)

/* ------ RCC bits --------------------------------------------------------- */
#define RCC_CR_HSION            (1U << 8)
#define RCC_CR_HSIRDY           (1U << 10)
#define RCC_CR_PLLON            (1U << 24)
#define RCC_CR_PLLRDY           (1U << 25)

#define RCC_CFGR_SW_MASK        (3U << 0)
#define RCC_CFGR_SW_HSI         (1U << 0)
#define RCC_CFGR_SW_PLL         (3U << 0)
#define RCC_CFGR_SWS_MASK       (3U << 2)
#define RCC_CFGR_SWS_HSI        (1U << 2)
#define RCC_CFGR_SWS_PLL        (3U << 2)

#define RCC_PLLCFGR_PLLSRC_HSI16   (2U << 0)
#define RCC_PLLCFGR_PLLM_POS       4              /* bits 6:4  (M = value+1) */
#define RCC_PLLCFGR_PLLN_POS       8              /* bits 14:8 */
#define RCC_PLLCFGR_PLLR_POS       25             /* bits 26:25 (R: 00=2, 01=4, 10=6, 11=8) */
#define RCC_PLLCFGR_PLLREN         (1U << 24)

#define RCC_APB1ENR1_PWREN         (1U << 28)

/* ------ PWR bits --------------------------------------------------------- */
#define PWR_CR1_VOS_MASK           (3U << 9)
#define PWR_CR1_VOS_RANGE1         (1U << 9)      /* 01: Range 1, 10: Range 2 */
#define PWR_SR2_VOSF               (1U << 10)

/* ------ FLASH ACR bits --------------------------------------------------- */
#define FLASH_ACR_LATENCY_MASK     (7U << 0)
#define FLASH_ACR_LATENCY_4WS      (4U << 0)
#define FLASH_ACR_PRFTEN           (1U << 8)
#define FLASH_ACR_ICEN             (1U << 9)
#define FLASH_ACR_DCEN             (1U << 10)

/* ------ Exported --------------------------------------------------------- */
uint32_t SystemCoreClock = 80000000UL;

/* ========================================================================= */
void SystemInit(void)
{
    /* ---------- 1. Enable HSI16 and make sure it's the clock source ----- */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) { }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSI) { }

    /* ---------- 2. Enable PWR clock; set VCORE = Range 1 ---------------- */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS_MASK) | PWR_CR1_VOS_RANGE1;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0U) { }

    /* ---------- 3. Flash: 4 WS, prefetch + I-cache ---------------------- */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_MASK)
               | FLASH_ACR_LATENCY_4WS
               | FLASH_ACR_PRFTEN
               | FLASH_ACR_ICEN
               | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_MASK) != FLASH_ACR_LATENCY_4WS) { }

    /* ---------- 4. Configure PLL ---------------------------------------- *
     * HSI16 (16 MHz) -> /M=1 -> 16 MHz -> *N=10 -> 160 MHz VCO -> /R=2 -> 80 MHz.
     * PLLM field encoding: value = M-1, so M=1 => 0.
     * PLLR field encoding: 00 = /2.
     * --------------------------------------------------------------------- */
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U) { }

    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI16
                 | (0U  << RCC_PLLCFGR_PLLM_POS)   /* M = 1  */
                 | (10U << RCC_PLLCFGR_PLLN_POS)   /* N = 10 */
                 | (0U  << RCC_PLLCFGR_PLLR_POS)   /* R = 2  */
                 | RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) { }

    /* ---------- 5. Switch SYSCLK to PLL --------------------------------- */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = 80000000UL;

    /* ---------- 6. Enable FPU (CP10 and CP11 full access) --------------- *
     * Required whenever code is built with -mfloat-abi=hard; without it,
     * the first FPU instruction faults. CPACR is at 0xE000ED88.
     * --------------------------------------------------------------------- */
    volatile uint32_t *cpacr = (volatile uint32_t*)0xE000ED88UL;
    *cpacr |= (0xFU << 20);
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");
}
