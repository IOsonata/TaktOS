/**
 * @file    startup_stm32h753.S
 * @brief   Startup + interrupt vector table for STM32H753ZI (Cortex-M7).
 *
 * Target: NUCLEO-H753ZI (STM32H753ZI, Cortex-M7 + DP FPU + D/I cache, MPU).
 *
 * On reset:
 *   1. MSP is initialised from the first word of the vector table.
 *   2. Reset_Handler copies .data from flash to RAM.
 *   3. .bss is zeroed.
 *   4. SystemInit() enables I-cache + D-cache, sets flash wait states
 *      appropriate for the reset-default VOS3 / HSI 64 MHz clock config.
 *   5. __libc_init_array() runs C++ static ctors.
 *   6. main() is called.
 *
 * Soft-float build: this port links against the TaktOS_M7 library
 * (Debug/Release, not ReleaseFPU — which doesn't exist in ARM/cm7/Eclipse
 * at this time). Therefore we do NOT enable CPACR CP10/CP11 — no VFP
 * instructions will execute. Thread-Metric is pure integer work, so the
 * FPU is irrelevant to the benchmark.
 *
 * TaktOS's PendSV_Handler, SysTick_Handler, and SVC_Handler are strong
 * symbols in libTaktOS_M7.a; they override the weak aliases defined here.
 *
 * TIM7_IRQHandler (IRQ 55) is used by tm_cause_interrupt(). TIM7 is never
 * clocked — only its NVIC pending bit is pulsed via STIR.
 */

    .syntax unified
    .cpu    cortex-m7
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
/*  Vector table — Cortex-M7 sys exceptions + STM32H753 peripheral IRQs       */
/*  This is the authoritative STM32H753 vector map (CMSIS stm32h753xx).       */
/* ========================================================================= */
    .section .isr_vector,"a",%progbits
    .global g_pfnVectors
    .type   g_pfnVectors, %object

g_pfnVectors:
    /* Cortex-M system exceptions */
    .word   _estack                               /*  0: Initial MSP         */
    .word   Reset_Handler                         /*  1: Reset               */
    .word   NMI_Handler                           /*  2: NMI                 */
    .word   HardFault_Handler                     /*  3: HardFault           */
    .word   MemManage_Handler                     /*  4: MemManage           */
    .word   BusFault_Handler                      /*  5: BusFault            */
    .word   UsageFault_Handler                    /*  6: UsageFault          */
    .word   0                                     /*  7: reserved            */
    .word   0                                     /*  8: reserved            */
    .word   0                                     /*  9: reserved            */
    .word   0                                     /* 10: reserved            */
    .word   SVC_Handler                           /* 11: SVCall              */
    .word   DebugMon_Handler                      /* 12: DebugMonitor        */
    .word   0                                     /* 13: reserved            */
    .word   PendSV_Handler                        /* 14: PendSV              */
    .word   SysTick_Handler                       /* 15: SysTick             */

    /* External IRQs (STM32H753) — positions per CMSIS stm32h753xx.h */
    .word   WWDG_IRQHandler                       /*  0 */
    .word   PVD_AVD_IRQHandler                    /*  1 */
    .word   TAMP_STAMP_IRQHandler                 /*  2 */
    .word   RTC_WKUP_IRQHandler                   /*  3 */
    .word   FLASH_IRQHandler                      /*  4 */
    .word   RCC_IRQHandler                        /*  5 */
    .word   EXTI0_IRQHandler                      /*  6 */
    .word   EXTI1_IRQHandler                      /*  7 */
    .word   EXTI2_IRQHandler                      /*  8 */
    .word   EXTI3_IRQHandler                      /*  9 */
    .word   EXTI4_IRQHandler                      /* 10 */
    .word   DMA1_Stream0_IRQHandler               /* 11 */
    .word   DMA1_Stream1_IRQHandler               /* 12 */
    .word   DMA1_Stream2_IRQHandler               /* 13 */
    .word   DMA1_Stream3_IRQHandler               /* 14 */
    .word   DMA1_Stream4_IRQHandler               /* 15 */
    .word   DMA1_Stream5_IRQHandler               /* 16 */
    .word   DMA1_Stream6_IRQHandler               /* 17 */
    .word   ADC_IRQHandler                        /* 18 */
    .word   FDCAN1_IT0_IRQHandler                 /* 19 */
    .word   FDCAN2_IT0_IRQHandler                 /* 20 */
    .word   FDCAN1_IT1_IRQHandler                 /* 21 */
    .word   FDCAN2_IT1_IRQHandler                 /* 22 */
    .word   EXTI9_5_IRQHandler                    /* 23 */
    .word   TIM1_BRK_IRQHandler                   /* 24 */
    .word   TIM1_UP_IRQHandler                    /* 25 */
    .word   TIM1_TRG_COM_IRQHandler               /* 26 */
    .word   TIM1_CC_IRQHandler                    /* 27 */
    .word   TIM2_IRQHandler                       /* 28 */
    .word   TIM3_IRQHandler                       /* 29 */
    .word   TIM4_IRQHandler                       /* 30 */
    .word   I2C1_EV_IRQHandler                    /* 31 */
    .word   I2C1_ER_IRQHandler                    /* 32 */
    .word   I2C2_EV_IRQHandler                    /* 33 */
    .word   I2C2_ER_IRQHandler                    /* 34 */
    .word   SPI1_IRQHandler                       /* 35 */
    .word   SPI2_IRQHandler                       /* 36 */
    .word   USART1_IRQHandler                     /* 37 */
    .word   USART2_IRQHandler                     /* 38 */
    .word   USART3_IRQHandler                     /* 39 — console (polled, NVIC line not enabled) */
    .word   EXTI15_10_IRQHandler                  /* 40 */
    .word   RTC_Alarm_IRQHandler                  /* 41 */
    .word   0                                     /* 42: reserved */
    .word   TIM8_BRK_TIM12_IRQHandler             /* 43 */
    .word   TIM8_UP_TIM13_IRQHandler              /* 44 */
    .word   TIM8_TRG_COM_TIM14_IRQHandler         /* 45 */
    .word   TIM8_CC_IRQHandler                    /* 46 */
    .word   DMA1_Stream7_IRQHandler               /* 47 */
    .word   FMC_IRQHandler                        /* 48 */
    .word   SDMMC1_IRQHandler                     /* 49 */
    .word   TIM5_IRQHandler                       /* 50 */
    .word   SPI3_IRQHandler                       /* 51 */
    .word   UART4_IRQHandler                      /* 52 */
    .word   UART5_IRQHandler                      /* 53 */
    .word   TIM6_DAC_IRQHandler                   /* 54 */
    .word   TIM7_IRQHandler                       /* 55 — used by tm_cause_interrupt() */
    .word   DMA2_Stream0_IRQHandler               /* 56 */
    .word   DMA2_Stream1_IRQHandler               /* 57 */
    .word   DMA2_Stream2_IRQHandler               /* 58 */
    .word   DMA2_Stream3_IRQHandler               /* 59 */
    .word   DMA2_Stream4_IRQHandler               /* 60 */
    .word   ETH_IRQHandler                        /* 61 */
    .word   ETH_WKUP_IRQHandler                   /* 62 */
    .word   FDCAN_CAL_IRQHandler                  /* 63 */
    .word   0                                     /* 64: reserved */
    .word   0                                     /* 65: reserved */
    .word   0                                     /* 66: reserved */
    .word   0                                     /* 67: reserved */
    .word   DMA2_Stream5_IRQHandler               /* 68 */
    .word   DMA2_Stream6_IRQHandler               /* 69 */
    .word   DMA2_Stream7_IRQHandler               /* 70 */
    .word   USART6_IRQHandler                     /* 71 */
    .word   I2C3_EV_IRQHandler                    /* 72 */
    .word   I2C3_ER_IRQHandler                    /* 73 */
    .word   OTG_HS_EP1_OUT_IRQHandler             /* 74 */
    .word   OTG_HS_EP1_IN_IRQHandler              /* 75 */
    .word   OTG_HS_WKUP_IRQHandler                /* 76 */
    .word   OTG_HS_IRQHandler                     /* 77 */
    .word   DCMI_IRQHandler                       /* 78 */
    .word   CRYP_IRQHandler                       /* 79 */
    .word   HASH_RNG_IRQHandler                   /* 80 */
    .word   FPU_IRQHandler                        /* 81 */
    .word   UART7_IRQHandler                      /* 82 */
    .word   UART8_IRQHandler                      /* 83 */
    .word   SPI4_IRQHandler                       /* 84 */
    .word   SPI5_IRQHandler                       /* 85 */
    .word   SPI6_IRQHandler                       /* 86 */
    .word   SAI1_IRQHandler                       /* 87 */
    .word   LTDC_IRQHandler                       /* 88 */
    .word   LTDC_ER_IRQHandler                    /* 89 */
    .word   DMA2D_IRQHandler                      /* 90 */
    .word   SAI2_IRQHandler                       /* 91 */
    .word   QUADSPI_IRQHandler                    /* 92 */
    .word   LPTIM1_IRQHandler                     /* 93 */
    .word   CEC_IRQHandler                        /* 94 */
    .word   I2C4_EV_IRQHandler                    /* 95 */
    .word   I2C4_ER_IRQHandler                    /* 96 */
    .word   SPDIF_RX_IRQHandler                   /* 97 */
    .word   OTG_FS_EP1_OUT_IRQHandler             /* 98 */
    .word   OTG_FS_EP1_IN_IRQHandler              /* 99 */
    .word   OTG_FS_WKUP_IRQHandler                /* 100 */
    .word   OTG_FS_IRQHandler                     /* 101 */
    .word   DMAMUX1_OVR_IRQHandler                /* 102 */
    .word   HRTIM1_Master_IRQHandler              /* 103 */
    .word   HRTIM1_TIMA_IRQHandler                /* 104 */
    .word   HRTIM1_TIMB_IRQHandler                /* 105 */
    .word   HRTIM1_TIMC_IRQHandler                /* 106 */
    .word   HRTIM1_TIMD_IRQHandler                /* 107 */
    .word   HRTIM1_TIME_IRQHandler                /* 108 */
    .word   HRTIM1_FLT_IRQHandler                 /* 109 */
    .word   DFSDM1_FLT0_IRQHandler                /* 110 */
    .word   DFSDM1_FLT1_IRQHandler                /* 111 */
    .word   DFSDM1_FLT2_IRQHandler                /* 112 */
    .word   DFSDM1_FLT3_IRQHandler                /* 113 */
    .word   SAI3_IRQHandler                       /* 114 */
    .word   SWPMI1_IRQHandler                     /* 115 */
    .word   TIM15_IRQHandler                      /* 116 */
    .word   TIM16_IRQHandler                      /* 117 */
    .word   TIM17_IRQHandler                      /* 118 */
    .word   MDIOS_WKUP_IRQHandler                 /* 119 */
    .word   MDIOS_IRQHandler                      /* 120 */
    .word   JPEG_IRQHandler                       /* 121 */
    .word   MDMA_IRQHandler                       /* 122 */
    .word   0                                     /* 123: reserved */
    .word   SDMMC2_IRQHandler                     /* 124 */
    .word   HSEM1_IRQHandler                      /* 125 */
    .word   0                                     /* 126: reserved */
    .word   ADC3_IRQHandler                       /* 127 */
    .word   DMAMUX2_OVR_IRQHandler                /* 128 */
    .word   BDMA_Channel0_IRQHandler              /* 129 */
    .word   BDMA_Channel1_IRQHandler              /* 130 */
    .word   BDMA_Channel2_IRQHandler              /* 131 */
    .word   BDMA_Channel3_IRQHandler              /* 132 */
    .word   BDMA_Channel4_IRQHandler              /* 133 */
    .word   BDMA_Channel5_IRQHandler              /* 134 */
    .word   BDMA_Channel6_IRQHandler              /* 135 */
    .word   BDMA_Channel7_IRQHandler              /* 136 */
    .word   COMP1_IRQHandler                      /* 137 */
    .word   LPTIM2_IRQHandler                     /* 138 */
    .word   LPTIM3_IRQHandler                     /* 139 */
    .word   LPTIM4_IRQHandler                     /* 140 */
    .word   LPTIM5_IRQHandler                     /* 141 */
    .word   LPUART1_IRQHandler                    /* 142 */
    .word   0                                     /* 143: reserved */
    .word   CRS_IRQHandler                        /* 144 */
    .word   ECC_IRQHandler                        /* 145 */
    .word   SAI4_IRQHandler                       /* 146 */
    .word   0                                     /* 147: reserved */
    .word   0                                     /* 148: reserved */
    .word   WAKEUP_PIN_IRQHandler                 /* 149 */

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
     * Single-increment loop — advance r1 (dest) and r3 (source) together. */
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

    /* Note: CPACR CP10/CP11 is NOT enabled here — this is a soft-float
     * build against libTaktOS_M7.a (Release config, no FPU). No VFP
     * instructions will be executed, so the FPU stays off. Thread-Metric
     * does no floating-point work. */

    bl      SystemInit
    bl      __libc_init_array
    bl      main

5:  b       5b
    .size   Reset_Handler, .-Reset_Handler

/* ========================================================================= */
/*  _init / _fini stubs (required by __libc_init_array)                      */
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
/*  Default_Handler                                                          */
/* ========================================================================= */
    .section .text.Default_Handler, "ax", %progbits
    .type   Default_Handler, %function
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, .-Default_Handler

/* ========================================================================= */
/*  Weak aliases — overridden by the TaktOS_M7 library (PendSV, SysTick, SVC)*/
/*  and by the tm_port layer (TIM7_IRQHandler).                              */
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
    def_weak_handler    PVD_AVD_IRQHandler
    def_weak_handler    TAMP_STAMP_IRQHandler
    def_weak_handler    RTC_WKUP_IRQHandler
    def_weak_handler    FLASH_IRQHandler
    def_weak_handler    RCC_IRQHandler
    def_weak_handler    EXTI0_IRQHandler
    def_weak_handler    EXTI1_IRQHandler
    def_weak_handler    EXTI2_IRQHandler
    def_weak_handler    EXTI3_IRQHandler
    def_weak_handler    EXTI4_IRQHandler
    def_weak_handler    DMA1_Stream0_IRQHandler
    def_weak_handler    DMA1_Stream1_IRQHandler
    def_weak_handler    DMA1_Stream2_IRQHandler
    def_weak_handler    DMA1_Stream3_IRQHandler
    def_weak_handler    DMA1_Stream4_IRQHandler
    def_weak_handler    DMA1_Stream5_IRQHandler
    def_weak_handler    DMA1_Stream6_IRQHandler
    def_weak_handler    ADC_IRQHandler
    def_weak_handler    FDCAN1_IT0_IRQHandler
    def_weak_handler    FDCAN2_IT0_IRQHandler
    def_weak_handler    FDCAN1_IT1_IRQHandler
    def_weak_handler    FDCAN2_IT1_IRQHandler
    def_weak_handler    EXTI9_5_IRQHandler
    def_weak_handler    TIM1_BRK_IRQHandler
    def_weak_handler    TIM1_UP_IRQHandler
    def_weak_handler    TIM1_TRG_COM_IRQHandler
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
    def_weak_handler    TIM8_BRK_TIM12_IRQHandler
    def_weak_handler    TIM8_UP_TIM13_IRQHandler
    def_weak_handler    TIM8_TRG_COM_TIM14_IRQHandler
    def_weak_handler    TIM8_CC_IRQHandler
    def_weak_handler    DMA1_Stream7_IRQHandler
    def_weak_handler    FMC_IRQHandler
    def_weak_handler    SDMMC1_IRQHandler
    def_weak_handler    TIM5_IRQHandler
    def_weak_handler    SPI3_IRQHandler
    def_weak_handler    UART4_IRQHandler
    def_weak_handler    UART5_IRQHandler
    def_weak_handler    TIM6_DAC_IRQHandler
    def_weak_handler    TIM7_IRQHandler
    def_weak_handler    DMA2_Stream0_IRQHandler
    def_weak_handler    DMA2_Stream1_IRQHandler
    def_weak_handler    DMA2_Stream2_IRQHandler
    def_weak_handler    DMA2_Stream3_IRQHandler
    def_weak_handler    DMA2_Stream4_IRQHandler
    def_weak_handler    ETH_IRQHandler
    def_weak_handler    ETH_WKUP_IRQHandler
    def_weak_handler    FDCAN_CAL_IRQHandler
    def_weak_handler    DMA2_Stream5_IRQHandler
    def_weak_handler    DMA2_Stream6_IRQHandler
    def_weak_handler    DMA2_Stream7_IRQHandler
    def_weak_handler    USART6_IRQHandler
    def_weak_handler    I2C3_EV_IRQHandler
    def_weak_handler    I2C3_ER_IRQHandler
    def_weak_handler    OTG_HS_EP1_OUT_IRQHandler
    def_weak_handler    OTG_HS_EP1_IN_IRQHandler
    def_weak_handler    OTG_HS_WKUP_IRQHandler
    def_weak_handler    OTG_HS_IRQHandler
    def_weak_handler    DCMI_IRQHandler
    def_weak_handler    CRYP_IRQHandler
    def_weak_handler    HASH_RNG_IRQHandler
    def_weak_handler    FPU_IRQHandler
    def_weak_handler    UART7_IRQHandler
    def_weak_handler    UART8_IRQHandler
    def_weak_handler    SPI4_IRQHandler
    def_weak_handler    SPI5_IRQHandler
    def_weak_handler    SPI6_IRQHandler
    def_weak_handler    SAI1_IRQHandler
    def_weak_handler    LTDC_IRQHandler
    def_weak_handler    LTDC_ER_IRQHandler
    def_weak_handler    DMA2D_IRQHandler
    def_weak_handler    SAI2_IRQHandler
    def_weak_handler    QUADSPI_IRQHandler
    def_weak_handler    LPTIM1_IRQHandler
    def_weak_handler    CEC_IRQHandler
    def_weak_handler    I2C4_EV_IRQHandler
    def_weak_handler    I2C4_ER_IRQHandler
    def_weak_handler    SPDIF_RX_IRQHandler
    def_weak_handler    OTG_FS_EP1_OUT_IRQHandler
    def_weak_handler    OTG_FS_EP1_IN_IRQHandler
    def_weak_handler    OTG_FS_WKUP_IRQHandler
    def_weak_handler    OTG_FS_IRQHandler
    def_weak_handler    DMAMUX1_OVR_IRQHandler
    def_weak_handler    HRTIM1_Master_IRQHandler
    def_weak_handler    HRTIM1_TIMA_IRQHandler
    def_weak_handler    HRTIM1_TIMB_IRQHandler
    def_weak_handler    HRTIM1_TIMC_IRQHandler
    def_weak_handler    HRTIM1_TIMD_IRQHandler
    def_weak_handler    HRTIM1_TIME_IRQHandler
    def_weak_handler    HRTIM1_FLT_IRQHandler
    def_weak_handler    DFSDM1_FLT0_IRQHandler
    def_weak_handler    DFSDM1_FLT1_IRQHandler
    def_weak_handler    DFSDM1_FLT2_IRQHandler
    def_weak_handler    DFSDM1_FLT3_IRQHandler
    def_weak_handler    SAI3_IRQHandler
    def_weak_handler    SWPMI1_IRQHandler
    def_weak_handler    TIM15_IRQHandler
    def_weak_handler    TIM16_IRQHandler
    def_weak_handler    TIM17_IRQHandler
    def_weak_handler    MDIOS_WKUP_IRQHandler
    def_weak_handler    MDIOS_IRQHandler
    def_weak_handler    JPEG_IRQHandler
    def_weak_handler    MDMA_IRQHandler
    def_weak_handler    SDMMC2_IRQHandler
    def_weak_handler    HSEM1_IRQHandler
    def_weak_handler    ADC3_IRQHandler
    def_weak_handler    DMAMUX2_OVR_IRQHandler
    def_weak_handler    BDMA_Channel0_IRQHandler
    def_weak_handler    BDMA_Channel1_IRQHandler
    def_weak_handler    BDMA_Channel2_IRQHandler
    def_weak_handler    BDMA_Channel3_IRQHandler
    def_weak_handler    BDMA_Channel4_IRQHandler
    def_weak_handler    BDMA_Channel5_IRQHandler
    def_weak_handler    BDMA_Channel6_IRQHandler
    def_weak_handler    BDMA_Channel7_IRQHandler
    def_weak_handler    COMP1_IRQHandler
    def_weak_handler    LPTIM2_IRQHandler
    def_weak_handler    LPTIM3_IRQHandler
    def_weak_handler    LPTIM4_IRQHandler
    def_weak_handler    LPTIM5_IRQHandler
    def_weak_handler    LPUART1_IRQHandler
    def_weak_handler    CRS_IRQHandler
    def_weak_handler    ECC_IRQHandler
    def_weak_handler    SAI4_IRQHandler
    def_weak_handler    WAKEUP_PIN_IRQHandler

    .end
