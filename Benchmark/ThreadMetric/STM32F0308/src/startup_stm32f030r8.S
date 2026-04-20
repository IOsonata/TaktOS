/**
 * @file    startup_stm32f030r8.S
 * @brief   Startup + interrupt vector table for STM32F030R8 (Cortex-M0).
 *
 * On reset:
 *   1. MSP is initialised from the first word of the vector table.
 *   2. Reset_Handler copies .data from flash to RAM.
 *   3. .bss is zeroed.
 *   4. SystemInit() configures HSI and PLL to reach 48 MHz SYSCLK.
 *   5. __libc_init_array() runs C++ static ctors / init_array entries.
 *   6. main() is called.
 *
 * TaktOS's PendSV_Handler and SysTick_Handler (and typically SVC_Handler) are
 * strong symbols in the linked TaktOS_M0 static library; they override the
 * weak aliases defined here at link time.
 */

    .syntax unified
    .cpu    cortex-m0
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
    .type   g_pfnVectors, %object

g_pfnVectors:
    .word   _estack                 /*  0: Initial MSP                     */
    .word   Reset_Handler           /*  1: Reset                           */
    .word   NMI_Handler             /*  2: NMI                             */
    .word   HardFault_Handler       /*  3: HardFault                       */
    .word   0                       /*  4 reserved                         */
    .word   0                       /*  5 reserved                         */
    .word   0                       /*  6 reserved                         */
    .word   0                       /*  7 reserved                         */
    .word   0                       /*  8 reserved                         */
    .word   0                       /*  9 reserved                         */
    .word   0                       /* 10 reserved                         */
    .word   SVC_Handler             /* 11: SVCall                          */
    .word   0                       /* 12 reserved                         */
    .word   0                       /* 13 reserved                         */
    .word   PendSV_Handler          /* 14: PendSV                          */
    .word   SysTick_Handler         /* 15: SysTick                         */

    /* External interrupts (STM32F030x8) */
    .word   WWDG_IRQHandler                    /*  0 */
    .word   0                                  /*  1 reserved */
    .word   RTC_IRQHandler                     /*  2 */
    .word   FLASH_IRQHandler                   /*  3 */
    .word   RCC_IRQHandler                     /*  4 */
    .word   EXTI0_1_IRQHandler                 /*  5 */
    .word   EXTI2_3_IRQHandler                 /*  6 */
    .word   EXTI4_15_IRQHandler                /*  7 */
    .word   0                                  /*  8 reserved */
    .word   DMA1_Channel1_IRQHandler           /*  9 */
    .word   DMA1_Channel2_3_IRQHandler         /* 10 */
    .word   DMA1_Channel4_5_IRQHandler         /* 11 */
    .word   ADC1_IRQHandler                    /* 12 */
    .word   TIM1_BRK_UP_TRG_COM_IRQHandler     /* 13 */
    .word   TIM1_CC_IRQHandler                 /* 14 */
    .word   0                                  /* 15 reserved */
    .word   TIM3_IRQHandler                    /* 16 */
    .word   0                                  /* 17 reserved */
    .word   0                                  /* 18 reserved */
    .word   TIM14_IRQHandler                   /* 19 */
    .word   0                                  /* 20 reserved */
    .word   TIM16_IRQHandler                   /* 21 */
    .word   TIM17_IRQHandler                   /* 22 - used by tm_cause_interrupt() */
    .word   I2C1_IRQHandler                    /* 23 */
    .word   I2C2_IRQHandler                    /* 24 */
    .word   SPI1_IRQHandler                    /* 25 */
    .word   SPI2_IRQHandler                    /* 26 */
    .word   USART1_IRQHandler                  /* 27 */
    .word   USART2_IRQHandler                  /* 28 */
    .word   0                                  /* 29 reserved */
    .word   0                                  /* 30 reserved */
    .word   0                                  /* 31 reserved */

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

    /* Copy .data: [_sidata ..) in flash -> [_sdata .. _edata) in RAM.
     *
     * Canonical form: advance r1 (dest) and r3 (source) together, one word
     * at a time. r4 is not used.
     *
     * History: an earlier version of this loop used a (r1, r4) address pair
     * and advanced BOTH r1 and r4, which caused the store stride to be 8
     * bytes instead of 4. Everything after the first .data word was left
     * uninitialized from SRAM power-up state, producing non-deterministic
     * behaviour in tm_test_duration and SystemCoreClock, including the
     * "benchmarks print immediately" symptom when tm_test_duration came up
     * as 0. Do not reintroduce the dual-register increment form. */
    ldr     r1, =_sdata
    ldr     r2, =_edata
    ldr     r3, =_sidata
1:  cmp     r1, r2
    bhs     2f
    ldr     r0, [r3]
    str     r0, [r1]
    adds    r3, r3, #4
    adds    r1, r1, #4
    b       1b
2:

    /* Zero .bss */
    ldr     r1, =_sbss
    ldr     r2, =_ebss
    movs    r0, #0
3:  cmp     r1, r2
    bhs     4f
    str     r0, [r1]
    adds    r1, r1, #4
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
/*  Weak aliases — overridden by strong symbols in the TaktOS_M0 library     */
/*  (PendSV_Handler, SysTick_Handler, SVC_Handler) and by the tm_port        */
/*  layer (TIM17_IRQHandler for tm_cause_interrupt()).                       */
/* ========================================================================= */
    .macro  def_weak_handler name
    .weak   \name
    .thumb_set \name, Default_Handler
    .endm

    def_weak_handler    NMI_Handler
    def_weak_handler    HardFault_Handler
    def_weak_handler    SVC_Handler
    def_weak_handler    PendSV_Handler
    def_weak_handler    SysTick_Handler

    def_weak_handler    WWDG_IRQHandler
    def_weak_handler    RTC_IRQHandler
    def_weak_handler    FLASH_IRQHandler
    def_weak_handler    RCC_IRQHandler
    def_weak_handler    EXTI0_1_IRQHandler
    def_weak_handler    EXTI2_3_IRQHandler
    def_weak_handler    EXTI4_15_IRQHandler
    def_weak_handler    DMA1_Channel1_IRQHandler
    def_weak_handler    DMA1_Channel2_3_IRQHandler
    def_weak_handler    DMA1_Channel4_5_IRQHandler
    def_weak_handler    ADC1_IRQHandler
    def_weak_handler    TIM1_BRK_UP_TRG_COM_IRQHandler
    def_weak_handler    TIM1_CC_IRQHandler
    def_weak_handler    TIM3_IRQHandler
    def_weak_handler    TIM14_IRQHandler
    def_weak_handler    TIM16_IRQHandler
    def_weak_handler    TIM17_IRQHandler
    def_weak_handler    I2C1_IRQHandler
    def_weak_handler    I2C2_IRQHandler
    def_weak_handler    SPI1_IRQHandler
    def_weak_handler    SPI2_IRQHandler
    def_weak_handler    USART1_IRQHandler
    def_weak_handler    USART2_IRQHandler

    .end
