/**
 * @file    startup_ra4m1.S
 * @brief   Diagnostic LED patterns, fault traps, and _init/_fini stubs.
 *
 * The vector table and Reset entry are now in vector_ra4m1.c and
 * ResetEntry.c (IOsonata pattern — direct reset vector, no asm Reset_Handler).
 *
 * What remains here:
 *   - pattern_short / pattern_long / pattern_fast_blink / pattern_steady
 *     (LED milestone feedback — called from SystemInit and from
 *     ResetEntry during debug runs)
 *   - HardFault_Handler / MemManage_Handler / BusFault_Handler /
 *     UsageFault_Handler : STRONG overrides of the weak aliases in
 *     vector_ra4m1.c, each branching to fault_trap which lights the LED
 *     solid-on and spins.
 *   - _init / _fini empty stubs (not used now that __libc_init_array is
 *     skipped, but kept so any stray reference does not break the link).
 */

    .syntax unified
    .cpu    cortex-m4
    .fpu    fpv4-sp-d16
    .thumb

    /* PORT2 registers — for the earliest-possible boot LED sign. */
    .equ    PORT2_PDR,      0x40040040
    .equ    PORT2_PODR,     0x40040042
    .equ    LED_MASK,       (1 << 4)        /* P204 */

/* ========================================================================= */
/*  LED milestone patterns. All at 48 MHz HOCO (bootloader leaves it on).   */
/*                                                                            */
/*    pattern_short      = one 0.3-s pulse then 1-s dark                    */
/*    pattern_long       = one 1.5-s pulse then 1-s dark                    */
/*    pattern_fast_blink = three 0.1-s pulses, spaced 0.1 s, then 1-s dark */
/*    pattern_steady     = NEVER RETURNS; 1-s on / 1-s off forever          */
/* ========================================================================= */
    .section .text.debug_led_post, "ax", %progbits
    .thumb

    .thumb_func
    .type   pattern_short, %function
    .global pattern_short
pattern_short:
    push    {r4, lr}
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    orr     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r3, =4800000              /* ~0.3 s @ 48 MHz (2 cy/iter) */
1:  subs    r3, r3, #1
    bne     1b
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    bic     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r3, =24000000             /* ~1 s dark gap */
2:  subs    r3, r3, #1
    bne     2b
    pop     {r4, pc}

    .thumb_func
    .type   pattern_long, %function
    .global pattern_long
pattern_long:
    push    {r4, lr}
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    orr     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r3, =36000000             /* ~1.5 s @ 48 MHz */
3:  subs    r3, r3, #1
    bne     3b
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    bic     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r3, =24000000             /* ~1 s dark gap */
4:  subs    r3, r3, #1
    bne     4b
    pop     {r4, pc}

    .thumb_func
    .type   pattern_fast_blink, %function
    .global pattern_fast_blink
pattern_fast_blink:
    push    {r4, r5, lr}
    movs    r4, #3                    /* 3 quick flashes */
pfb_loop:
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    orr     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r5, =2400000              /* ~0.1 s on */
5:  subs    r5, r5, #1
    bne     5b
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    bic     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r5, =2400000              /* ~0.1 s off */
6:  subs    r5, r5, #1
    bne     6b
    subs    r4, r4, #1
    bne     pfb_loop
    /* Final 1-s dark gap. */
    ldr     r5, =24000000
7:  subs    r5, r5, #1
    bne     7b
    pop     {r4, r5, pc}

    .thumb_func
    .type   pattern_steady, %function
    .global pattern_steady
pattern_steady:
    /* Forever: 1 s on, 1 s off. Never returns. */
ps_loop:
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    orr     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r3, =24000000             /* ~1 s on */
8:  subs    r3, r3, #1
    bne     8b
    ldr     r1, =PORT2_PODR
    ldrh    r2, [r1]
    bic     r2, r2, #LED_MASK
    strh    r2, [r1]
    ldr     r3, =24000000             /* ~1 s off */
9:  subs    r3, r3, #1
    bne     9b
    b       ps_loop

    /* Legacy symbols kept as no-ops to avoid linker errors from stale
     * .o files that might still reference them. */
    .thumb_func
    .type   debug_led_blink_6, %function
    .type   debug_led_blink_7, %function
    .type   debug_led_blink_8, %function
    .type   debug_led_blink_9, %function
    .global debug_led_blink_6
    .global debug_led_blink_7
    .global debug_led_blink_8
    .global debug_led_blink_9
debug_led_blink_6:
debug_led_blink_7:
debug_led_blink_8:
debug_led_blink_9:
    bx      lr

/* ========================================================================= */
/*  Fault traps — DISTINCT LED patterns per fault type so that "solid-on"   */
/*  tells us WHICH fault fired instead of being a single opaque state.      */
/*                                                                            */
/*    HardFault    : 1 long pulse,  then solid on                            */
/*    MemManage    : 2 short pulses, then solid on                           */
/*    BusFault     : 3 short pulses, then solid on                           */
/*    UsageFault   : 4 short pulses, then solid on   (FPU faults land here) */
/*                                                                            */
/*  All end in a tight ft_spin so the LED stays lit as unambiguous evidence. */
/* ========================================================================= */
    .section .text.fault_trap, "ax", %progbits
    .thumb

    /* Common tail: LED solid on + infinite spin. */
    .thumb_func
    .type   fault_hold, %function
fault_hold:
    ldr     r0, =PORT2_PDR
    ldrh    r1, [r0]
    orr     r1, r1, #LED_MASK
    strh    r1, [r0]
    ldr     r0, =PORT2_PODR
    ldrh    r1, [r0]
    orr     r1, r1, #LED_MASK
    strh    r1, [r0]
ft_spin:
    b       ft_spin

    .thumb_func
    .type   HardFault_Handler, %function
    .global HardFault_Handler
HardFault_Handler:
    bl      pattern_long         /* 1 long pulse */
    b       fault_hold

    .thumb_func
    .type   MemManage_Handler, %function
    .global MemManage_Handler
MemManage_Handler:
    bl      pattern_short        /* 2 short pulses */
    bl      pattern_short
    b       fault_hold

    .thumb_func
    .type   BusFault_Handler, %function
    .global BusFault_Handler
BusFault_Handler:
    bl      pattern_short        /* 3 short pulses */
    bl      pattern_short
    bl      pattern_short
    b       fault_hold

    .thumb_func
    .type   UsageFault_Handler, %function
    .global UsageFault_Handler
UsageFault_Handler:
    bl      pattern_short        /* 4 short pulses — FPU faults come here */
    bl      pattern_short
    bl      pattern_short
    bl      pattern_short
    b       fault_hold

/* ========================================================================= */
/*  Empty _init / _fini.  Not called now that ResetEntry skips              */
/*  __libc_init_array, but kept weak in case any TU still references them. */
/* ========================================================================= */
    .section .text._init, "ax", %progbits
    .thumb
    .weak   _init
    .type   _init, %function
_init:
    bx      lr
    .size   _init, .-_init

    .section .text._fini, "ax", %progbits
    .thumb
    .weak   _fini
    .type   _fini, %function
_fini:
    bx      lr
    .size   _fini, .-_fini

    .end
