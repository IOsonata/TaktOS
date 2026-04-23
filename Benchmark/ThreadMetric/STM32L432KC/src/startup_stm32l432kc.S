/**
 * @file    startup_stm32l432kc.S
 * @brief   Startup + interrupt vector table for STM32L432KC (Cortex-M4F).
 *
 * Style matches the IOsonata convention so this file can later be folded into
 * IOsonata/ARM/ST/STM32L4xx/STM32L432KC/ without structural changes:
 *   - Vector table in .isr_vector, symbol `g_pfnVectors`.
 *   - Reset_Handler sets MSP, copies .data, zeroes .bss, calls SystemInit,
 *     __libc_init_array, then main().
 *   - Default_Handler is a forever-loop; all unused IRQs point at it via
 *     weak aliases, so a stray unimplemented IRQ halts predictably.
 *   - TaktOS's PendSV_Handler, SysTick_Handler, SVC_Handler are strong
 *     symbols in libTaktOS_M4.a and override the weak aliases here.
 *
 * Vector table is taken from RM0394 Rev 4 Table 47 (NVIC vector table,
 * STM32L432xx). All implemented IRQs up to #82 are listed.
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

    /* External interrupts (STM32L432xx per RM0394 Table 47) */
    .word   WWDG_IRQHandler                        /*  0 Window watchdog              */
    .word   PVD_PVM_IRQHandler                     /*  1 PVD/PVM through EXTI          */
    .word   TAMP_STAMP_IRQHandler                  /*  2 RTC tamper/timestamp via EXTI */
    .word   RTC_WKUP_IRQHandler                    /*  3 RTC wakeup via EXTI           */
    .word   FLASH_IRQHandler                       /*  4 Flash global                  */
    .word   RCC_IRQHandler                         /*  5 RCC global                    */
    .word   EXTI0_IRQHandler                       /*  6 EXTI line0                    */
    .word   EXTI1_IRQHandler                       /*  7 EXTI line1                    */
    .word   EXTI2_IRQHandler                       /*  8 EXTI line2                    */
    .word   EXTI3_IRQHandler                       /*  9 EXTI line3                    */
    .word   EXTI4_IRQHandler                       /* 10 EXTI line4                    */
    .word   DMA1_Channel1_IRQHandler               /* 11 DMA1 ch1                      */
    .word   DMA1_Channel2_IRQHandler               /* 12 DMA1 ch2                      */
    .word   DMA1_Channel3_IRQHandler               /* 13 DMA1 ch3                      */
    .word   DMA1_Channel4_IRQHandler               /* 14 DMA1 ch4                      */
    .word   DMA1_Channel5_IRQHandler               /* 15 DMA1 ch5                      */
    .word   DMA1_Channel6_IRQHandler               /* 16 DMA1 ch6                      */
    .word   DMA1_Channel7_IRQHandler               /* 17 DMA1 ch7                      */
    .word   ADC1_IRQHandler                        /* 18 ADC1                          */
    .word   CAN1_TX_IRQHandler                     /* 19 CAN1 TX                       */
    .word   CAN1_RX0_IRQHandler                    /* 20 CAN1 RX0                      */
    .word   CAN1_RX1_IRQHandler                    /* 21 CAN1 RX1                      */
    .word   CAN1_SCE_IRQHandler                    /* 22 CAN1 SCE                      */
    .word   EXTI9_5_IRQHandler                     /* 23 EXTI line[9:5]                */
    .word   TIM1_BRK_TIM15_IRQHandler              /* 24 TIM1 Break / TIM15            */
    .word   TIM1_UP_TIM16_IRQHandler               /* 25 TIM1 Update / TIM16           */
    .word   TIM1_TRG_COM_IRQHandler                /* 26 TIM1 Trigger and Comm         */
    .word   TIM1_CC_IRQHandler                     /* 27 TIM1 Capture Compare          */
    .word   TIM2_IRQHandler                        /* 28 TIM2                          */
    .word   0                                      /* 29 reserved (TIM3 not on L432)   */
    .word   0                                      /* 30 reserved (TIM4 not on L432)   */
    .word   I2C1_EV_IRQHandler                     /* 31 I2C1 event                    */
    .word   I2C1_ER_IRQHandler                     /* 32 I2C1 error                    */
    .word   0                                      /* 33 reserved (I2C2 not on L432KC) */
    .word   0                                      /* 34 reserved (I2C2 not on L432KC) */
    .word   SPI1_IRQHandler                        /* 35 SPI1                          */
    .word   0                                      /* 36 reserved (SPI2 not on L432KC) */
    .word   USART1_IRQHandler                      /* 37 USART1                        */
    .word   USART2_IRQHandler                      /* 38 USART2                        */
    .word   USART3_IRQHandler                      /* 39 USART3                        */
    .word   EXTI15_10_IRQHandler                   /* 40 EXTI line[15:10]              */
    .word   RTC_Alarm_IRQHandler                   /* 41 RTC Alarms via EXTI           */
    .word   0                                      /* 42 reserved                      */
    .word   0                                      /* 43 reserved                      */
    .word   0                                      /* 44 reserved                      */
    .word   0                                      /* 45 reserved                      */
    .word   0                                      /* 46 reserved                      */
    .word   0                                      /* 47 reserved                      */
    .word   0                                      /* 48 reserved                      */
    .word   SDMMC1_IRQHandler                      /* 49 SDMMC1 (not present on L432)  */
    .word   0                                      /* 50 reserved                      */
    .word   SPI3_IRQHandler                        /* 51 SPI3                          */
    .word   0                                      /* 52 reserved (UART4 not on L432KC)*/
    .word   0                                      /* 53 reserved                      */
    .word   TIM6_DAC_IRQHandler                    /* 54 TIM6 / DAC1+DAC2 underrun     */
    .word   TIM7_IRQHandler                        /* 55 TIM7 — used by tm_cause_interrupt() */
    .word   DMA2_Channel1_IRQHandler               /* 56 DMA2 ch1                      */
    .word   DMA2_Channel2_IRQHandler               /* 57 DMA2 ch2                      */
    .word   DMA2_Channel3_IRQHandler               /* 58 DMA2 ch3                      */
    .word   DMA2_Channel4_IRQHandler               /* 59 DMA2 ch4                      */
    .word   DMA2_Channel5_IRQHandler               /* 60 DMA2 ch5                      */
    .word   0                                      /* 61 reserved                      */
    .word   0                                      /* 62 reserved                      */
    .word   0                                      /* 63 reserved                      */
    .word   COMP_IRQHandler                        /* 64 COMP1/2                       */
    .word   LPTIM1_IRQHandler                      /* 65 LPTIM1                        */
    .word   LPTIM2_IRQHandler                      /* 66 LPTIM2                        */
    .word   USB_IRQHandler                         /* 67 USB FS                        */
    .word   DMA2_Channel6_IRQHandler               /* 68 DMA2 ch6                      */
    .word   DMA2_Channel7_IRQHandler               /* 69 DMA2 ch7                      */
    .word   LPUART1_IRQHandler                     /* 70 LPUART1                       */
    .word   QUADSPI_IRQHandler                     /* 71 QUADSPI                       */
    .word   I2C3_EV_IRQHandler                     /* 72 I2C3 event                    */
    .word   I2C3_ER_IRQHandler                     /* 73 I2C3 error                    */
    .word   SAI1_IRQHandler                        /* 74 SAI1                          */
    .word   0                                      /* 75 reserved                      */
    .word   SWPMI1_IRQHandler                      /* 76 SWPMI1                        */
    .word   TSC_IRQHandler                         /* 77 TSC                           */
    .word   0                                      /* 78 reserved (LCD not on L432)    */
    .word   0                                      /* 79 reserved (AES not on L432)    */
    .word   RNG_IRQHandler                         /* 80 RNG                           */
    .word   FPU_IRQHandler                         /* 81 FPU                           */
    .word   CRS_IRQHandler                         /* 82 CRS                           */

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

    /* Copy .data from LMA (flash) to VMA (RAM).
     * Canonical form — advance a single destination cursor and a single
     * source cursor, one word at a time. Do not reintroduce a dual-register
     * increment form; see history in startup_stm32f030r8.S. */
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
/*  Empty _init / _fini required by __libc_init_array.                       */
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
/*  Default_Handler — catch-all for unimplemented IRQs.                      */
/* ========================================================================= */
    .section .text.Default_Handler, "ax", %progbits
    .type   Default_Handler, %function
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, .-Default_Handler

/* ========================================================================= */
/*  Weak aliases — overridden by strong symbols in libTaktOS_M4.a            */
/*  (PendSV_Handler, SysTick_Handler, SVC_Handler) and by the tm_port        */
/*  layer (TIM7_IRQHandler for tm_cause_interrupt()).                        */
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
    def_weak_handler    ADC1_IRQHandler
    def_weak_handler    CAN1_TX_IRQHandler
    def_weak_handler    CAN1_RX0_IRQHandler
    def_weak_handler    CAN1_RX1_IRQHandler
    def_weak_handler    CAN1_SCE_IRQHandler
    def_weak_handler    EXTI9_5_IRQHandler
    def_weak_handler    TIM1_BRK_TIM15_IRQHandler
    def_weak_handler    TIM1_UP_TIM16_IRQHandler
    def_weak_handler    TIM1_TRG_COM_IRQHandler
    def_weak_handler    TIM1_CC_IRQHandler
    def_weak_handler    TIM2_IRQHandler
    def_weak_handler    I2C1_EV_IRQHandler
    def_weak_handler    I2C1_ER_IRQHandler
    def_weak_handler    SPI1_IRQHandler
    def_weak_handler    USART1_IRQHandler
    def_weak_handler    USART2_IRQHandler
    def_weak_handler    USART3_IRQHandler
    def_weak_handler    EXTI15_10_IRQHandler
    def_weak_handler    RTC_Alarm_IRQHandler
    def_weak_handler    SDMMC1_IRQHandler
    def_weak_handler    SPI3_IRQHandler
    def_weak_handler    TIM6_DAC_IRQHandler
    def_weak_handler    TIM7_IRQHandler
    def_weak_handler    DMA2_Channel1_IRQHandler
    def_weak_handler    DMA2_Channel2_IRQHandler
    def_weak_handler    DMA2_Channel3_IRQHandler
    def_weak_handler    DMA2_Channel4_IRQHandler
    def_weak_handler    DMA2_Channel5_IRQHandler
    def_weak_handler    COMP_IRQHandler
    def_weak_handler    LPTIM1_IRQHandler
    def_weak_handler    LPTIM2_IRQHandler
    def_weak_handler    USB_IRQHandler
    def_weak_handler    DMA2_Channel6_IRQHandler
    def_weak_handler    DMA2_Channel7_IRQHandler
    def_weak_handler    LPUART1_IRQHandler
    def_weak_handler    QUADSPI_IRQHandler
    def_weak_handler    I2C3_EV_IRQHandler
    def_weak_handler    I2C3_ER_IRQHandler
    def_weak_handler    SAI1_IRQHandler
    def_weak_handler    SWPMI1_IRQHandler
    def_weak_handler    TSC_IRQHandler
    def_weak_handler    RNG_IRQHandler
    def_weak_handler    FPU_IRQHandler
    def_weak_handler    CRS_IRQHandler

    .end
