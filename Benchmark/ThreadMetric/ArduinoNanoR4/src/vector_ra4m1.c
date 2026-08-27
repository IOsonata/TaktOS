/**-------------------------------------------------------------------------
 * @file    vector_ra4m1.c
 *
 * @brief   Interrupt Vectors table for Renesas RA4M1 (Cortex-M4F)
 *          in IOsonata style (GCC, section .isr_vector).
 *
 *          Vector[0] = __StackTop (initial MSP)
 *          Vector[1] = ResetEntry  (direct reset vector, no asm shim)
 *
 * Author:  derived from IOsonata/ARM/src/Vectors_* (Nguyen Hoan Hoang)
 * ---------------------------------------------------------------------------*/
#include <stdint.h>

/* Linker-script symbol:  top of stack (matches gcc_ra4m1.ld _estack). */
extern unsigned long _estack;

/* Reset entry — first real code run after reset.  Defined in ResetEntry.c. */
extern void ResetEntry(void);

/* ========================================================================= */
/*  Default weak handlers — every IRQ defaults to a spin loop, and strong    */
/*  versions from other translation units (TaktOS, tm_port, startup_ra4m1.S  */
/*  debug fault traps) override the weak ones at link time.                  */
/* ========================================================================= */
void DEF_IRQHandler(void) { while (1) { } }

__attribute__((weak, alias("DEF_IRQHandler"))) void NMI_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void HardFault_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void MemManage_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void BusFault_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void UsageFault_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void SVC_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void DebugMon_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void PendSV_Handler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void SysTick_Handler(void);

/* RA4M1 ICU event-link IRQs 0..31 (fed by IELSR0..IELSR31). */
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL0_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL1_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL2_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL3_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL4_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL5_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL6_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL7_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL8_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL9_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL10_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL11_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL12_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL13_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL14_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL15_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL16_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL17_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL18_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL19_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL20_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL21_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL22_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL23_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL24_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL25_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL26_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL27_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL28_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL29_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL30_IRQHandler(void);
__attribute__((weak, alias("DEF_IRQHandler"))) void IEL31_IRQHandler(void);


/**
 * Vector table placed at flash offset 0x00004000 (origin of FLASH in the
 * linker script).  Reset_Handler slot is ResetEntry — the direct reset
 * vector, no intermediate assembly jump.
 */
__attribute__ ((section(".isr_vector"), used))
void (* const __Vectors[])(void) = {
    (void (*)(void))((uint32_t)&_estack),   /*  0: Initial MSP     */
    ResetEntry,                             /*  1: Reset           */
    NMI_Handler,                            /*  2: NMI             */
    HardFault_Handler,                      /*  3: HardFault       */
    MemManage_Handler,                      /*  4: MemManage       */
    BusFault_Handler,                       /*  5: BusFault        */
    UsageFault_Handler,                     /*  6: UsageFault      */
    0, 0, 0, 0,                             /*  7-10 reserved      */
    SVC_Handler,                            /* 11: SVCall          */
    DebugMon_Handler,                       /* 12: DebugMonitor    */
    0,                                      /* 13 reserved         */
    PendSV_Handler,                         /* 14: PendSV          */
    SysTick_Handler,                        /* 15: SysTick         */

    /* External Interrupts: ICU event-link 0..31 */
    IEL0_IRQHandler,
    IEL1_IRQHandler,
    IEL2_IRQHandler,
    IEL3_IRQHandler,
    IEL4_IRQHandler,
    IEL5_IRQHandler,
    IEL6_IRQHandler,
    IEL7_IRQHandler,
    IEL8_IRQHandler,
    IEL9_IRQHandler,
    IEL10_IRQHandler,
    IEL11_IRQHandler,
    IEL12_IRQHandler,
    IEL13_IRQHandler,
    IEL14_IRQHandler,
    IEL15_IRQHandler,
    IEL16_IRQHandler,
    IEL17_IRQHandler,
    IEL18_IRQHandler,
    IEL19_IRQHandler,
    IEL20_IRQHandler,
    IEL21_IRQHandler,
    IEL22_IRQHandler,
    IEL23_IRQHandler,
    IEL24_IRQHandler,
    IEL25_IRQHandler,
    IEL26_IRQHandler,
    IEL27_IRQHandler,
    IEL28_IRQHandler,
    IEL29_IRQHandler,
    IEL30_IRQHandler,
    IEL31_IRQHandler,
};
