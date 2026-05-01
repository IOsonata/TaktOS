/**
 * @file    system_stm32f030.c
 * @brief   Early system init for STM32F030R8: HSI/2 * 12 = 48 MHz SYSCLK,
 *          1 wait state on flash, prefetch enabled. AHB = APB = SYSCLK.
 */

#include <stdint.h>

#define PERIPH_BASE         0x40000000UL
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define RCC_BASE            (AHB1PERIPH_BASE + 0x1000UL)
#define FLASH_R_BASE        (AHB1PERIPH_BASE + 0x2000UL)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    volatile uint32_t AHBRSTR;
    volatile uint32_t CFGR2;
    volatile uint32_t CFGR3;
    volatile uint32_t CR2;
} RCC_TypeDef;

typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t AR;
    uint32_t RESERVED;
    volatile uint32_t OBR;
    volatile uint32_t WRPR;
} FLASH_TypeDef;

#define RCC     ((RCC_TypeDef*)RCC_BASE)
#define FLASH   ((FLASH_TypeDef*)FLASH_R_BASE)

#define RCC_CR_HSION    (1U << 0)
#define RCC_CR_HSIRDY   (1U << 1)
#define RCC_CR_PLLON    (1U << 24)
#define RCC_CR_PLLRDY   (1U << 25)

#define RCC_CFGR_SW_HSI          (0U << 0)
#define RCC_CFGR_SW_PLL          (2U << 0)
#define RCC_CFGR_SWS_Msk         (3U << 2)
#define RCC_CFGR_SWS_PLL         (2U << 2)
#define RCC_CFGR_PLLSRC_HSI_DIV2 (0U << 16)
#define RCC_CFGR_PLLMUL_12       (10U << 18)

#define FLASH_ACR_LATENCY   (1U << 0)
#define FLASH_ACR_PRFTBE    (1U << 4)

uint32_t SystemCoreClock = 48000000UL;

void SystemInit(void)
{
    /* HSI on */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0) { }

    /* Switch to HSI so we can safely reconfigure PLL */
    RCC->CFGR = (RCC->CFGR & ~(3U << 0)) | RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != (RCC_CFGR_SW_HSI << 2)) { }

    /* PLL off */
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0) { }

    /* Flash: 1 WS + prefetch (required above 24 MHz) */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

    /* PLL: HSI/2 (4 MHz) * 12 = 48 MHz. AHB/APB prescalers = 1 (default). */
    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~((0xFU << 18) | (1U << 16));
    cfgr |=  RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLMUL_12;
    RCC->CFGR = cfgr;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0) { }

    /* Switch SYSCLK to PLL */
    RCC->CFGR = (RCC->CFGR & ~(3U << 0)) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) { }

    SystemCoreClock = 48000000UL;
}
