/**
 * @file    startup_ra4m1.S
 * @brief   Startup + interrupt vector table for Renesas RA4M1 (Cortex-M4F).
 *
 * Target:  Arduino Nano R4 (RA4M1 @ 48 MHz, 256 KB flash, 32 KB SRAM, FPU).
 *
 * On reset:
 *   1. MSP is initialised from the first word of the vector table.
 *   2. Reset_Handler copies .data from flash to RAM.
 *   3. .bss is zeroed.
 *   4. SystemInit() selects HOCO @ 48 MHz as ICLK and brings clocks up.
 *   5. FPU (CP10/CP11) is enabled in CPACR before static ctors run.
 *   6. __libc_init_array() runs C++ static ctors / init_array entries.
 *   7. main() is called.
 *
 * TaktOS's PendSV_Handler, SysTick_Handler, and SVC_Handler are strong
 * symbols in the linked TaktOS_M4 ReleaseFPU static library; they override
 * the weak aliases defined here at link time.
 *
 * The RA4M1 ICU feeds 32 NVIC IRQs (IRQ 0..31) via IELSRn event-link
 * registers (n = IELSR slot). The vector table below reserves 32 IELn_IRQHandler
 * slots. tm_port_taktos.cpp uses slot 31 (IELSR31) for tm_cause_interrupt()
 * with no peripheral event assigned — only the NVIC pending bit is pulsed.
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
    .word   MemManage_Handler       /*  4: MemManage                       */
    .word   BusFault_Handler        /*  5: BusFault                        */
    .word   UsageFault_Handler      /*  6: UsageFault                      */
    .word   0                       /*  7 reserved                         */
    .word   0                       /*  8 reserved                         */
    .word   0                       /*  9 reserved                         */
    .word   0                       /* 10 reserved                         */
    .word   SVC_Handler             /* 11: SVCall                          */
    .word   DebugMon_Handler        /* 12: DebugMonitor                    */
    .word   0                       /* 13 reserved                         */
    .word   PendSV_Handler          /* 14: PendSV                          */
    .word   SysTick_Handler         /* 15: SysTick                         */

    /* RA4M1 ICU event-link IRQs 0..31 (fed by IELSR0..IELSR31) */
    .word   IEL0_IRQHandler
    .word   IEL1_IRQHandler
    .word   IEL2_IRQHandler
    .word   IEL3_IRQHandler
    .word   IEL4_IRQHandler
    .word   IEL5_IRQHandler
    .word   IEL6_IRQHandler
    .word   IEL7_IRQHandler
    .word   IEL8_IRQHandler
    .word   IEL9_IRQHandler
    .word   IEL10_IRQHandler
    .word   IEL11_IRQHandler
    .word   IEL12_IRQHandler
    .word   IEL13_IRQHandler
    .word   IEL14_IRQHandler
    .word   IEL15_IRQHandler
    .word   IEL16_IRQHandler
    .word   IEL17_IRQHandler
    .word   IEL18_IRQHandler
    .word   IEL19_IRQHandler
    .word   IEL20_IRQHandler
    .word   IEL21_IRQHandler
    .word   IEL22_IRQHandler
    .word   IEL23_IRQHandler
    .word   IEL24_IRQHandler
    .word   IEL25_IRQHandler
    .word   IEL26_IRQHandler
    .word   IEL27_IRQHandler
    .word   IEL28_IRQHandler
    .word   IEL29_IRQHandler
    .word   IEL30_IRQHandler
    .word   IEL31_IRQHandler        /* used by tm_cause_interrupt() */

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
     * Canonical form: advance dest (r1) and source (r3) together one word
     * at a time. Do not reintroduce any dual-increment form — that corrupts
     * the copy stride and leaves .data uninitialised past the first word. */
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

    /* Enable FPU (CPACR: CP10 full access, CP11 full access) BEFORE any C
     * code runs. Required because the TaktOS_M4 ReleaseFPU library and
     * any -mfloat-abi=hard code will execute VFP instructions. */
    ldr     r0, =0xE000ED88         /* CPACR */
    ldr     r1, [r0]
    orr     r1, r1, #(0xF << 20)    /* CP10 + CP11 full access */
    str     r1, [r0]
    dsb
    isb

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
/*  Weak aliases — overridden by strong symbols in the TaktOS_M4 library     */
/*  (PendSV_Handler, SysTick_Handler, SVC_Handler) and by the tm_port        */
/*  layer (IEL31_IRQHandler for tm_cause_interrupt()).                       */
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

    def_weak_handler    IEL0_IRQHandler
    def_weak_handler    IEL1_IRQHandler
    def_weak_handler    IEL2_IRQHandler
    def_weak_handler    IEL3_IRQHandler
    def_weak_handler    IEL4_IRQHandler
    def_weak_handler    IEL5_IRQHandler
    def_weak_handler    IEL6_IRQHandler
    def_weak_handler    IEL7_IRQHandler
    def_weak_handler    IEL8_IRQHandler
    def_weak_handler    IEL9_IRQHandler
    def_weak_handler    IEL10_IRQHandler
    def_weak_handler    IEL11_IRQHandler
    def_weak_handler    IEL12_IRQHandler
    def_weak_handler    IEL13_IRQHandler
    def_weak_handler    IEL14_IRQHandler
    def_weak_handler    IEL15_IRQHandler
    def_weak_handler    IEL16_IRQHandler
    def_weak_handler    IEL17_IRQHandler
    def_weak_handler    IEL18_IRQHandler
    def_weak_handler    IEL19_IRQHandler
    def_weak_handler    IEL20_IRQHandler
    def_weak_handler    IEL21_IRQHandler
    def_weak_handler    IEL22_IRQHandler
    def_weak_handler    IEL23_IRQHandler
    def_weak_handler    IEL24_IRQHandler
    def_weak_handler    IEL25_IRQHandler
    def_weak_handler    IEL26_IRQHandler
    def_weak_handler    IEL27_IRQHandler
    def_weak_handler    IEL28_IRQHandler
    def_weak_handler    IEL29_IRQHandler
    def_weak_handler    IEL30_IRQHandler
    def_weak_handler    IEL31_IRQHandler

    .end
