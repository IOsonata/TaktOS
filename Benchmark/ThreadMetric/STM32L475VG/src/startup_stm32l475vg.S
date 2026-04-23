/**
 * @file    startup_stm32l475vg.S
 * @brief   Startup + interrupt vector table for STM32L475VG (Cortex-M4F).
 *
 * Style matches the IOsonata convention so this file can later be folded into
 * IOsonata/ARM/ST/STM32L4xx/STM32L475xx/ without structural changes.
 *
 * Vector table per RM0351 Rev 10 Table 58 (NVIC vector table, STM32L475xx).
 * All implemented IRQs up to #91 are listed.
 */

    .syntax unified
    .cpu    cortex-m4
    .fpu    softvfp
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

/* ========================================================================= */
/*  Vector table                                                              */
/* ========================================================================= */
    .section .isr_vector,"a",%progbits
    .global g_pfnVectors
    .type   g_pfnVectors, %object

g_pfnVectors:
    .word   _estack                                /*  0: Initial MSP           */
    .word   Reset_Handler                          /*  1: Reset                 */
    .word   NMI_Handler                            /*  2: NMI                   */
    .word   HardFault_Handler                      /*  3: HardFault             */
    .word   MemManage_Handler                      /*  4: MemManage             */
    .word   BusFault_Handler                       /*  5: BusFault              */
    .word   UsageFault_Handler                     /*  6: UsageFault            */
    .word   0                                      /*  7 reserved               */
    .word   0                                      /*  8 reserved               */
    .word   0                                      /*  9 reserved               */
    .word   0                                      /* 10 reserved               */
    .word   SVC_Handler                            /* 11: SVCall                */
    .word   DebugMon_Handler                       /* 12: Debug monitor         */
    .word   0                                      /* 13 reserved               */
    .word   PendSV_Handler                         /* 14: PendSV                */
    .word   SysTick_Handler                        /* 15: SysTick               */

    /* External interrupts (STM32L475xx per RM0351 Table 58) */
    .word   WWDG_IRQHandler                        /*  0 */
    .word   PVD_PVM_IRQHandler                     /*  1 */
    .word   TAMP_STAMP_IRQHandler                  /*  2 */
    .word   RTC_WKUP_IRQHandler                    /*  3 */
    .word   FLASH_IRQHandler                       /*  4 */
    .word   RCC_IRQHandler                         /*  5 */
    .word   EXTI0_IRQHandler                       /*  6 */
    .word   EXTI1_IRQHandler                       /*  7 */
    .word   EXTI2_IRQHandler                       /*  8 */
    .word   EXTI3_IRQHandler                       /*  9 */
    .word   EXTI4_IRQHandler                       /* 10 */
    .word   DMA1_Channel1_IRQHandler               /* 11 */
    .word   DMA1_Channel2_IRQHandler               /* 12 */
    .word   DMA1_Channel3_IRQHandler               /* 13 */
    .word   DMA1_Channel4_IRQHandler               /* 14 */
    .word   DMA1_Channel5_IRQHandler               /* 15 */
    .word   DMA1_Channel6_IRQHandler               /* 16 */
    .word   DMA1_Channel7_IRQHandler               /* 17 */
    .word   ADC1_2_IRQHandler                      /* 18 */
    .word   CAN1_TX_IRQHandler                     /* 19 */
    .word   CAN1_RX0_IRQHandler                    /* 20 */
    .word   CAN1_RX1_IRQHandler                    /* 21 */
    .word   CAN1_SCE_IRQHandler                    /* 22 */
    .word   EXTI9_5_IRQHandler                     /* 23 */
    .word   TIM1_BRK_TIM15_IRQHandler              /* 24 */
    .word   TIM1_UP_TIM16_IRQHandler               /* 25 */
    .word   TIM1_TRG_COM_TIM17_IRQHandler          /* 26 */
    .word   TIM1_CC_IRQHandler                     /* 27 */
    .word   TIM2_IRQHandler                        /* 28 */
    .word   TIM3_IRQHandler                        /* 29 */
    .word   TIM4_IRQHandler                        /* 30 */
    .word   I2C1_EV_IRQHandler                     /* 31 */
    .word   I2C1_ER_IRQHandler                     /* 32 */
    .word   I2C2_EV_IRQHandler                     /* 33 */
    .word   I2C2_ER_IRQHandler                     /* 34 */
    .word   SPI1_IRQHandler                        /* 35 */
    .word   SPI2_IRQHandler                        /* 36 */
    .word   USART1_IRQHandler                      /* 37 */
    .word   USART2_IRQHandler                      /* 38 */
    .word   USART3_IRQHandler                      /* 39 */
    .word   EXTI15_10_IRQHandler                   /* 40 */
    .word   RTC_Alarm_IRQHandler                   /* 41 */
    .word   DFSDM1_FLT3_IRQHandler                 /* 42 */
    .word   TIM8_BRK_IRQHandler                    /* 43 */
    .word   TIM8_UP_IRQHandler                     /* 44 */
    .word   TIM8_TRG_COM_IRQHandler                /* 45 */
    .word   TIM8_CC_IRQHandler                     /* 46 */
    .word   ADC3_IRQHandler                        /* 47 */
    .word   FMC_IRQHandler                         /* 48 */
    .word   SDMMC1_IRQHandler                      /* 49 */
    .word   TIM5_IRQHandler                        /* 50 */
    .word   SPI3_IRQHandler                        /* 51 */
    .word   UART4_IRQHandler                       /* 52 */
    .word   UART5_IRQHandler                       /* 53 */
    .word   TIM6_DAC_IRQHandler                    /* 54 */
    .word   TIM7_IRQHandler                        /* 55 TIM7 - used by tm_cause_interrupt() */
    .word   DMA2_Channel1_IRQHandler               /* 56 */
    .word   DMA2_Channel2_IRQHandler               /* 57 */
    .word   DMA2_Channel3_IRQHandler               /* 58 */
    .word   DMA2_Channel4_IRQHandler               /* 59 */
    .word   DMA2_Channel5_IRQHandler               /* 60 */
    .word   DFSDM1_FLT0_IRQHandler                 /* 61 */
    .word   DFSDM1_FLT1_IRQHandler                 /* 62 */
    .word   DFSDM1_FLT2_IRQHandler                 /* 63 */
    .word   COMP_IRQHandler                        /* 64 */
    .word   LPTIM1_IRQHandler                      /* 65 */
    .word   LPTIM2_IRQHandler                      /* 66 */
    .word   OTG_FS_IRQHandler                      /* 67 USB OTG FS */
    .word   DMA2_Channel6_IRQHandler               /* 68 */
    .word   DMA2_Channel7_IRQHandler               /* 69 */
    .word   LPUART1_IRQHandler                     /* 70 */
    .word   QUADSPI_IRQHandler                     /* 71 */
    .word   I2C3_EV_IRQHandler                     /* 72 */
    .word   I2C3_ER_IRQHandler                     /* 73 */
    .word   SAI1_IRQHandler                        /* 74 */
    .word   SAI2_IRQHandler                        /* 75 */
    .word   SWPMI1_IRQHandler                      /* 76 */
    .word   TSC_IRQHandler                         /* 77 */
    .word   0                                      /* 78 reserved (LCD not on L475) */
    .word   AES_IRQHandler                         /* 79 */
    .word   RNG_IRQHandler                         /* 80 */
    .word   FPU_IRQHandler                         /* 81 */

    .size g_pfnVectors, .-g_pfnVectors

/* ========================================================================= */
/*  Reset_Handler                                                             */
/* ========================================================================= */
    .section .text.Reset_Handler
    .weak   Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:
    ldr     r0, =_estack
    mov     sp, r0

    /* Copy .data */
    ldr     r1, =_sdata
    ldr     r2, =_edata
    ldr     r3, =_sidata
1:  cmp     r1, r2
    bhs     2f
    ldr     r0, [r3], #4
    str     r0, [r1], #4
    b       1b
2:

    /* Zero .bss */
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

/* ========================================================================= */
/*  Empty _init / _fini                                                       */
/* ========================================================================= */
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

/* ========================================================================= */
/*  Default_Handler                                                           */
/* ========================================================================= */
    .section .text.Default_Handler, "ax", %progbits
    .type   Default_Handler, %function
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, .-Default_Handler

/* ========================================================================= */
/*  Weak aliases                                                              */
/* ========================================================================= */
    .macro  def_weak_handler name
    .weak   \name
    .thumb_set \name, Default_Handler
    .endm

    def_weak_handler    NMI_Handler
    def_weak_handler    HardFault_Handler
    def_weak_handler    MemManage_Handler
    def_weak_handler    BusFault_Handler
    def_weak_handler    UsageFault_Handler
    def_weak_handler    SVC_Handler
    def_weak_handler    DebugMon_Handler
    def_weak_handler    PendSV_Handler
    def_weak_handler    SysTick_Handler

    def_weak_handler    WWDG_IRQHandler
    def_weak_handler    PVD_PVM_IRQHandler
    def_weak_handler    TAMP_STAMP_IRQHandler
    def_weak_handler    RTC_WKUP_IRQHandler
    def_weak_handler    FLASH_IRQHandler
    def_weak_handler    RCC_IRQHandler
    def_weak_handler    EXTI0_IRQHandler
    def_weak_handler    EXTI1_IRQHandler
    def_weak_handler    EXTI2_IRQHandler
    def_weak_handler    EXTI3_IRQHandler
    def_weak_handler    EXTI4_IRQHandler
    def_weak_handler    DMA1_Channel1_IRQHandler
    def_weak_handler    DMA1_Channel2_IRQHandler
    def_weak_handler    DMA1_Channel3_IRQHandler
    def_weak_handler    DMA1_Channel4_IRQHandler
    def_weak_handler    DMA1_Channel5_IRQHandler
    def_weak_handler    DMA1_Channel6_IRQHandler
    def_weak_handler    DMA1_Channel7_IRQHandler
    def_weak_handler    ADC1_2_IRQHandler
    def_weak_handler    CAN1_TX_IRQHandler
    def_weak_handler    CAN1_RX0_IRQHandler
    def_weak_handler    CAN1_RX1_IRQHandler
    def_weak_handler    CAN1_SCE_IRQHandler
    def_weak_handler    EXTI9_5_IRQHandler
    def_weak_handler    TIM1_BRK_TIM15_IRQHandler
    def_weak_handler    TIM1_UP_TIM16_IRQHandler
    def_weak_handler    TIM1_TRG_COM_TIM17_IRQHandler
    def_weak_handler    TIM1_CC_IRQHandler
    def_weak_handler    TIM2_IRQHandler
    def_weak_handler    TIM3_IRQHandler
    def_weak_handler    TIM4_IRQHandler
    def_weak_handler    I2C1_EV_IRQHandler
    def_weak_handler    I2C1_ER_IRQHandler
    def_weak_handler    I2C2_EV_IRQHandler
    def_weak_handler    I2C2_ER_IRQHandler
    def_weak_handler    SPI1_IRQHandler
    def_weak_handler    SPI2_IRQHandler
    def_weak_handler    USART1_IRQHandler
    def_weak_handler    USART2_IRQHandler
    def_weak_handler    USART3_IRQHandler
    def_weak_handler    EXTI15_10_IRQHandler
    def_weak_handler    RTC_Alarm_IRQHandler
    def_weak_handler    DFSDM1_FLT3_IRQHandler
    def_weak_handler    TIM8_BRK_IRQHandler
    def_weak_handler    TIM8_UP_IRQHandler
    def_weak_handler    TIM8_TRG_COM_IRQHandler
    def_weak_handler    TIM8_CC_IRQHandler
    def_weak_handler    ADC3_IRQHandler
    def_weak_handler    FMC_IRQHandler
    def_weak_handler    SDMMC1_IRQHandler
    def_weak_handler    TIM5_IRQHandler
    def_weak_handler    SPI3_IRQHandler
    def_weak_handler    UART4_IRQHandler
    def_weak_handler    UART5_IRQHandler
    def_weak_handler    TIM6_DAC_IRQHandler
    def_weak_handler    TIM7_IRQHandler
    def_weak_handler    DMA2_Channel1_IRQHandler
    def_weak_handler    DMA2_Channel2_IRQHandler
    def_weak_handler    DMA2_Channel3_IRQHandler
    def_weak_handler    DMA2_Channel4_IRQHandler
    def_weak_handler    DMA2_Channel5_IRQHandler
    def_weak_handler    DFSDM1_FLT0_IRQHandler
    def_weak_handler    DFSDM1_FLT1_IRQHandler
    def_weak_handler    DFSDM1_FLT2_IRQHandler
    def_weak_handler    COMP_IRQHandler
    def_weak_handler    LPTIM1_IRQHandler
    def_weak_handler    LPTIM2_IRQHandler
    def_weak_handler    OTG_FS_IRQHandler
    def_weak_handler    DMA2_Channel6_IRQHandler
    def_weak_handler    DMA2_Channel7_IRQHandler
    def_weak_handler    LPUART1_IRQHandler
    def_weak_handler    QUADSPI_IRQHandler
    def_weak_handler    I2C3_EV_IRQHandler
    def_weak_handler    I2C3_ER_IRQHandler
    def_weak_handler    SAI1_IRQHandler
    def_weak_handler    SAI2_IRQHandler
    def_weak_handler    SWPMI1_IRQHandler
    def_weak_handler    TSC_IRQHandler
    def_weak_handler    AES_IRQHandler
    def_weak_handler    RNG_IRQHandler
    def_weak_handler    FPU_IRQHandler

    .end
