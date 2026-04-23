/**
 * @file    startup_stm32g474.S
 * @brief   Startup + vector table for STM32G474RE on NUCLEO-G474RE.
 *
 * Cortex-M4 + FPv4-SP-D16. HARD-FLOAT build. This startup enables CPACR
 * CP10/CP11 in Reset_Handler before any C code, so VFP instructions
 * succeed.
 *
 * Reset sequence:
 *   1. MSP from the first vector word.
 *   2. Enable FPU (CPACR |= 0xF << 20).
 *   3. Copy .data from flash to RAM, zero .bss.
 *   4. SystemInit() — 170 MHz PLL, VOS1 boost, 4 WS flash.
 *   5. __libc_init_array() — C++ static ctors.
 *   6. main().
 *
 * Vector table covers the STM32G474 IRQ set (102 peripheral IRQs per
 * CMSIS stm32g474xx). Unused IRQs default to Default_Handler via weak
 * aliases.
 */

    .syntax unified
    .cpu    cortex-m4
    .fpu    fpv4-sp-d16
    .thumb

    .extern _sidata
    .extern _sdata
    .extern _edata
    .extern _sbss
    .extern _ebss
    .extern _estack
    .extern SystemInit
    .extern main
    .extern __libc_init_array

    .section .isr_vector,"a",%progbits
    .type   g_pfnVectors, %object

g_pfnVectors:
    .word   _estack
    .word   Reset_Handler
    .word   NMI_Handler
    .word   HardFault_Handler
    .word   MemManage_Handler
    .word   BusFault_Handler
    .word   UsageFault_Handler
    .word   0
    .word   0
    .word   0
    .word   0
    .word   SVC_Handler
    .word   DebugMon_Handler
    .word   0
    .word   PendSV_Handler
    .word   SysTick_Handler

    /* STM32G474 peripheral IRQs — positions per CMSIS stm32g474xx.h */
    .word   WWDG_IRQHandler                    /*  0 */
    .word   PVD_PVM_IRQHandler                 /*  1 */
    .word   RTC_TAMP_LSECSS_IRQHandler         /*  2 */
    .word   RTC_WKUP_IRQHandler                /*  3 */
    .word   FLASH_IRQHandler                   /*  4 */
    .word   RCC_IRQHandler                     /*  5 */
    .word   EXTI0_IRQHandler                   /*  6 */
    .word   EXTI1_IRQHandler                   /*  7 */
    .word   EXTI2_IRQHandler                   /*  8 */
    .word   EXTI3_IRQHandler                   /*  9 */
    .word   EXTI4_IRQHandler                   /* 10 */
    .word   DMA1_Channel1_IRQHandler           /* 11 */
    .word   DMA1_Channel2_IRQHandler           /* 12 */
    .word   DMA1_Channel3_IRQHandler           /* 13 */
    .word   DMA1_Channel4_IRQHandler           /* 14 */
    .word   DMA1_Channel5_IRQHandler           /* 15 */
    .word   DMA1_Channel6_IRQHandler           /* 16 */
    .word   DMA1_Channel7_IRQHandler           /* 17 */
    .word   ADC1_2_IRQHandler                  /* 18 */
    .word   USB_HP_IRQHandler                  /* 19 */
    .word   USB_LP_IRQHandler                  /* 20 */
    .word   FDCAN1_IT0_IRQHandler              /* 21 */
    .word   FDCAN1_IT1_IRQHandler              /* 22 */
    .word   EXTI9_5_IRQHandler                 /* 23 */
    .word   TIM1_BRK_TIM15_IRQHandler          /* 24 */
    .word   TIM1_UP_TIM16_IRQHandler           /* 25 */
    .word   TIM1_TRG_COM_TIM17_IRQHandler      /* 26 */
    .word   TIM1_CC_IRQHandler                 /* 27 */
    .word   TIM2_IRQHandler                    /* 28 */
    .word   TIM3_IRQHandler                    /* 29 */
    .word   TIM4_IRQHandler                    /* 30 */
    .word   I2C1_EV_IRQHandler                 /* 31 */
    .word   I2C1_ER_IRQHandler                 /* 32 */
    .word   I2C2_EV_IRQHandler                 /* 33 */
    .word   I2C2_ER_IRQHandler                 /* 34 */
    .word   SPI1_IRQHandler                    /* 35 */
    .word   SPI2_IRQHandler                    /* 36 */
    .word   USART1_IRQHandler                  /* 37 */
    .word   USART2_IRQHandler                  /* 38 */
    .word   USART3_IRQHandler                  /* 39 */
    .word   EXTI15_10_IRQHandler               /* 40 */
    .word   RTC_Alarm_IRQHandler               /* 41 */
    .word   USBWakeUp_IRQHandler               /* 42 */
    .word   TIM8_BRK_IRQHandler                /* 43 */
    .word   TIM8_UP_IRQHandler                 /* 44 */
    .word   TIM8_TRG_COM_IRQHandler            /* 45 */
    .word   TIM8_CC_IRQHandler                 /* 46 */
    .word   ADC3_IRQHandler                    /* 47 */
    .word   FMC_IRQHandler                     /* 48 */
    .word   LPTIM1_IRQHandler                  /* 49 */
    .word   TIM5_IRQHandler                    /* 50 */
    .word   SPI3_IRQHandler                    /* 51 */
    .word   UART4_IRQHandler                   /* 52 */
    .word   UART5_IRQHandler                   /* 53 */
    .word   TIM6_DAC_IRQHandler                /* 54 */
    .word   TIM7_DAC_IRQHandler                /* 55  <-- tm_cause_interrupt() */
    .word   DMA2_Channel1_IRQHandler           /* 56 */
    .word   DMA2_Channel2_IRQHandler           /* 57 */
    .word   DMA2_Channel3_IRQHandler           /* 58 */
    .word   DMA2_Channel4_IRQHandler           /* 59 */
    .word   DMA2_Channel5_IRQHandler           /* 60 */
    .word   ADC4_IRQHandler                    /* 61 */
    .word   ADC5_IRQHandler                    /* 62 */
    .word   UCPD1_IRQHandler                   /* 63 */
    .word   COMP1_2_3_IRQHandler               /* 64 */
    .word   COMP4_5_6_IRQHandler               /* 65 */
    .word   COMP7_IRQHandler                   /* 66 */
    .word   HRTIM1_Master_IRQHandler           /* 67 */
    .word   HRTIM1_TIMA_IRQHandler             /* 68 */
    .word   HRTIM1_TIMB_IRQHandler             /* 69 */
    .word   HRTIM1_TIMC_IRQHandler             /* 70 */
    .word   HRTIM1_TIMD_IRQHandler             /* 71 */
    .word   HRTIM1_TIME_IRQHandler             /* 72 */
    .word   HRTIM1_FLT_IRQHandler              /* 73 */
    .word   HRTIM1_TIMF_IRQHandler             /* 74 */
    .word   CRS_IRQHandler                     /* 75 */
    .word   SAI1_IRQHandler                    /* 76 */
    .word   TIM20_BRK_IRQHandler               /* 77 */
    .word   TIM20_UP_IRQHandler                /* 78 */
    .word   TIM20_TRG_COM_IRQHandler           /* 79 */
    .word   TIM20_CC_IRQHandler                /* 80 */
    .word   FPU_IRQHandler                     /* 81 */
    .word   I2C4_EV_IRQHandler                 /* 82 */
    .word   I2C4_ER_IRQHandler                 /* 83 */
    .word   SPI4_IRQHandler                    /* 84 */
    .word   AES_IRQHandler                     /* 85 */
    .word   FDCAN2_IT0_IRQHandler              /* 86 */
    .word   FDCAN2_IT1_IRQHandler              /* 87 */
    .word   FDCAN3_IT0_IRQHandler              /* 88 */
    .word   FDCAN3_IT1_IRQHandler              /* 89 */
    .word   RNG_IRQHandler                     /* 90 */
    .word   LPUART1_IRQHandler                 /* 91 */
    .word   I2C3_EV_IRQHandler                 /* 92 */
    .word   I2C3_ER_IRQHandler                 /* 93 */
    .word   DMAMUX_OVR_IRQHandler              /* 94 */
    .word   QUADSPI_IRQHandler                 /* 95 */
    .word   DMA1_Channel8_IRQHandler           /* 96 */
    .word   DMA2_Channel6_IRQHandler           /* 97 */
    .word   DMA2_Channel7_IRQHandler           /* 98 */
    .word   DMA2_Channel8_IRQHandler           /* 99 */
    .word   CORDIC_IRQHandler                  /*100 */
    .word   FMAC_IRQHandler                    /*101 */

    .size g_pfnVectors, .-g_pfnVectors

    .section .text.Reset_Handler
    .weak   Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:
    ldr     r0, =_estack
    mov     sp, r0

    /* Enable FPU CP10/CP11 full access — before any VFP instruction. */
    ldr     r0, =0xE000ED88
    ldr     r1, [r0]
    orr     r1, r1, #(0xF << 20)
    str     r1, [r0]
    dsb
    isb

    ldr     r1, =_sdata
    ldr     r2, =_edata
    ldr     r3, =_sidata
1:  cmp     r1, r2
    bhs     2f
    ldr     r0, [r3], #4
    str     r0, [r1], #4
    b       1b
2:
    ldr     r1, =_sbss
    ldr     r2, =_ebss
    movs    r0, #0
3:  cmp     r1, r2
    bhs     4f
    str     r0, [r1], #4
    b       3b
4:
    bl      SystemInit
    bl      __libc_init_array
    bl      main
5:  b       5b
    .size   Reset_Handler, .-Reset_Handler

    .section .text._init, "ax", %progbits
    .weak   _init
    .type   _init, %function
_init:
    bx      lr
    .size   _init, .-_init

    .section .text._fini, "ax", %progbits
    .weak   _fini
    .type   _fini, %function
_fini:
    bx      lr
    .size   _fini, .-_fini

    .section .text.Default_Handler, "ax", %progbits
    .type   Default_Handler, %function
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, .-Default_Handler

    .macro  def_weak_handler name
    .weak   \name
    .thumb_set \name, Default_Handler
    .endm

    def_weak_handler NMI_Handler
    def_weak_handler HardFault_Handler
    def_weak_handler MemManage_Handler
    def_weak_handler BusFault_Handler
    def_weak_handler UsageFault_Handler
    def_weak_handler SVC_Handler
    def_weak_handler DebugMon_Handler
    def_weak_handler PendSV_Handler
    def_weak_handler SysTick_Handler

    def_weak_handler WWDG_IRQHandler
    def_weak_handler PVD_PVM_IRQHandler
    def_weak_handler RTC_TAMP_LSECSS_IRQHandler
    def_weak_handler RTC_WKUP_IRQHandler
    def_weak_handler FLASH_IRQHandler
    def_weak_handler RCC_IRQHandler
    def_weak_handler EXTI0_IRQHandler
    def_weak_handler EXTI1_IRQHandler
    def_weak_handler EXTI2_IRQHandler
    def_weak_handler EXTI3_IRQHandler
    def_weak_handler EXTI4_IRQHandler
    def_weak_handler DMA1_Channel1_IRQHandler
    def_weak_handler DMA1_Channel2_IRQHandler
    def_weak_handler DMA1_Channel3_IRQHandler
    def_weak_handler DMA1_Channel4_IRQHandler
    def_weak_handler DMA1_Channel5_IRQHandler
    def_weak_handler DMA1_Channel6_IRQHandler
    def_weak_handler DMA1_Channel7_IRQHandler
    def_weak_handler ADC1_2_IRQHandler
    def_weak_handler USB_HP_IRQHandler
    def_weak_handler USB_LP_IRQHandler
    def_weak_handler FDCAN1_IT0_IRQHandler
    def_weak_handler FDCAN1_IT1_IRQHandler
    def_weak_handler EXTI9_5_IRQHandler
    def_weak_handler TIM1_BRK_TIM15_IRQHandler
    def_weak_handler TIM1_UP_TIM16_IRQHandler
    def_weak_handler TIM1_TRG_COM_TIM17_IRQHandler
    def_weak_handler TIM1_CC_IRQHandler
    def_weak_handler TIM2_IRQHandler
    def_weak_handler TIM3_IRQHandler
    def_weak_handler TIM4_IRQHandler
    def_weak_handler I2C1_EV_IRQHandler
    def_weak_handler I2C1_ER_IRQHandler
    def_weak_handler I2C2_EV_IRQHandler
    def_weak_handler I2C2_ER_IRQHandler
    def_weak_handler SPI1_IRQHandler
    def_weak_handler SPI2_IRQHandler
    def_weak_handler USART1_IRQHandler
    def_weak_handler USART2_IRQHandler
    def_weak_handler USART3_IRQHandler
    def_weak_handler EXTI15_10_IRQHandler
    def_weak_handler RTC_Alarm_IRQHandler
    def_weak_handler USBWakeUp_IRQHandler
    def_weak_handler TIM8_BRK_IRQHandler
    def_weak_handler TIM8_UP_IRQHandler
    def_weak_handler TIM8_TRG_COM_IRQHandler
    def_weak_handler TIM8_CC_IRQHandler
    def_weak_handler ADC3_IRQHandler
    def_weak_handler FMC_IRQHandler
    def_weak_handler LPTIM1_IRQHandler
    def_weak_handler TIM5_IRQHandler
    def_weak_handler SPI3_IRQHandler
    def_weak_handler UART4_IRQHandler
    def_weak_handler UART5_IRQHandler
    def_weak_handler TIM6_DAC_IRQHandler
    def_weak_handler TIM7_DAC_IRQHandler
    def_weak_handler DMA2_Channel1_IRQHandler
    def_weak_handler DMA2_Channel2_IRQHandler
    def_weak_handler DMA2_Channel3_IRQHandler
    def_weak_handler DMA2_Channel4_IRQHandler
    def_weak_handler DMA2_Channel5_IRQHandler
    def_weak_handler ADC4_IRQHandler
    def_weak_handler ADC5_IRQHandler
    def_weak_handler UCPD1_IRQHandler
    def_weak_handler COMP1_2_3_IRQHandler
    def_weak_handler COMP4_5_6_IRQHandler
    def_weak_handler COMP7_IRQHandler
    def_weak_handler HRTIM1_Master_IRQHandler
    def_weak_handler HRTIM1_TIMA_IRQHandler
    def_weak_handler HRTIM1_TIMB_IRQHandler
    def_weak_handler HRTIM1_TIMC_IRQHandler
    def_weak_handler HRTIM1_TIMD_IRQHandler
    def_weak_handler HRTIM1_TIME_IRQHandler
    def_weak_handler HRTIM1_FLT_IRQHandler
    def_weak_handler HRTIM1_TIMF_IRQHandler
    def_weak_handler CRS_IRQHandler
    def_weak_handler SAI1_IRQHandler
    def_weak_handler TIM20_BRK_IRQHandler
    def_weak_handler TIM20_UP_IRQHandler
    def_weak_handler TIM20_TRG_COM_IRQHandler
    def_weak_handler TIM20_CC_IRQHandler
    def_weak_handler FPU_IRQHandler
    def_weak_handler I2C4_EV_IRQHandler
    def_weak_handler I2C4_ER_IRQHandler
    def_weak_handler SPI4_IRQHandler
    def_weak_handler AES_IRQHandler
    def_weak_handler FDCAN2_IT0_IRQHandler
    def_weak_handler FDCAN2_IT1_IRQHandler
    def_weak_handler FDCAN3_IT0_IRQHandler
    def_weak_handler FDCAN3_IT1_IRQHandler
    def_weak_handler RNG_IRQHandler
    def_weak_handler LPUART1_IRQHandler
    def_weak_handler I2C3_EV_IRQHandler
    def_weak_handler I2C3_ER_IRQHandler
    def_weak_handler DMAMUX_OVR_IRQHandler
    def_weak_handler QUADSPI_IRQHandler
    def_weak_handler DMA1_Channel8_IRQHandler
    def_weak_handler DMA2_Channel6_IRQHandler
    def_weak_handler DMA2_Channel7_IRQHandler
    def_weak_handler DMA2_Channel8_IRQHandler
    def_weak_handler CORDIC_IRQHandler
    def_weak_handler FMAC_IRQHandler

    .end
