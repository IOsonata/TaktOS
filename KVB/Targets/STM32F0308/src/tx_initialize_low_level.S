@/**************************************************************************
@ * Custom tx_initialize_low_level.S for KVB on STM32F0308-DISCO.
@ *
@ * Replaces the upstream ports/cortex_m0/gnu/example_build version.
@ *
@ * Differences from upstream:
@ *
@ *   - Does NOT define _vectors.  IOsonata's startup file owns the vector
@ *     table; we ship without our own vector table.
@ *
@ *   - Does NOT alias SysTick_Handler at file scope.  Upstream provides
@ *     a strong SysTick_Handler that calls _tx_timer_interrupt; this
@ *     would multiply-define against IOsonata's weak SysTick_Handler.
@ *     Instead we provide a SysTick_Handler with the correct M0 wrapper
@ *     that calls _tx_timer_interrupt.  IOsonata declares SysTick_Handler
@ *     as weak in its vector table, so our strong definition wins.
@ *
@ *   - Does NOT alias PendSV_Handler here.  ThreadX's _tx_thread_schedule
@ *     defines __tx_PendSVHandler; we provide a one-instruction shim
@ *     PendSV_Handler -> __tx_PendSVHandler in C (kvb_threadx_glue.c)
@ *     so IOsonata's vector table dispatches to it.
@ *
@ *   - SysTick configured for 1000 Hz at 48 MHz core clock (47999
@ *     reload), matching FreeRTOS configTICK_RATE_HZ on the same
@ *     target.  Upstream defaults to 100 Hz at 8 MHz.
@ *
@ *   - Does NOT touch __RAM_segment_used_end__.  IOsonata's linker
@ *     script doesn't define that symbol.  KVB does not use ThreadX's
@ *     byte_pool feature, so first_unused_memory is left at NULL —
@ *     tx_application_define receives NULL and ignores it (we allocate
@ *     all KVB objects statically, see kvb_port_threadx.c).
@ *
@ *   - System stack pointer is captured live via MRS r0, MSP at entry,
@ *     not read from a vector-table symbol.  This survives any vector
@ *     table layout choice IOsonata makes now or in the future.
@ *
@ * Origin notice:  derived from
@ *   eclipse-threadx/threadx ports/cortex_m0/gnu/example_build/
@ *     tx_initialize_low_level.S
@ * Copyright (c) 2024 Microsoft Corporation, MIT license, with
@ * customizations for IOsonata/KVB integration on STM32F0308-DISCO.
@ **************************************************************************/

    .syntax unified
    .text
    .align 4

    .global _tx_thread_system_stack_ptr
    .global _tx_initialize_unused_memory
    .global _tx_thread_context_save
    .global _tx_thread_context_restore
    .global _tx_timer_interrupt

    @ STM32F030R8 @ 48 MHz, 1000 Hz tick
    .equ    SYSTEM_CLOCK,    48000000
    .equ    SYSTICK_CYCLES,  ((SYSTEM_CLOCK / 1000) - 1)


@/* _tx_initialize_low_level
@ *
@ * Called once from _tx_initialize_kernel_enter at scheduler startup
@ * to configure the periodic tick interrupt source, save the system
@ * stack pointer for ISR processing, and record the first available
@ * RAM address for tx_application_define.
@ */
    .global _tx_initialize_low_level
    .thumb_func
_tx_initialize_low_level:

    @ Disable interrupts during ThreadX initialization.
    CPSID   i

    @ Set base of available memory to NULL — KVB does not use ThreadX's
    @ byte_pool, all allocations are static.  tx_application_define
    @ receives 0 as first_unused_memory and ignores it.
    LDR     r0, =_tx_initialize_unused_memory
    MOVS    r1, #0
    STR     r1, [r0]

    @ Save the system stack pointer (current MSP).  ThreadX uses this
    @ value during ISR context save/restore and when bootstrapping the
    @ first thread.  This works regardless of where the vector table
    @ lives or what symbol IOsonata uses for it.
    LDR     r0, =_tx_thread_system_stack_ptr
    MRS     r1, MSP
    STR     r1, [r0]

    @ Configure SysTick for 1000 Hz tick at 48 MHz.
    @ SysTick base = 0xE000E010
    @     SYST_CSR     0x00 — control & status
    @     SYST_RVR     0x04 — reload value
    @     SYST_CVR     0x08 — current value
    @
    @ CSR write value 0x7 = ENABLE | TICKINT | CLKSOURCE (CPU clock).
    LDR     r0, =0xE000E010
    LDR     r1, =SYSTICK_CYCLES
    STR     r1, [r0, #4]            @ SYST_RVR
    MOVS    r1, #0
    STR     r1, [r0, #8]            @ SYST_CVR — clear current
    MOVS    r1, #7
    STR     r1, [r0, #0]            @ SYST_CSR — enable + tickint + cpuclk

    @ Configure system handler priorities.  Cortex-M0 only exposes the
    @ priority registers SHPR2 (E000ED1C) and SHPR3 (E000ED20).
    @
    @ SHPR3 layout (byte-wise from low to high address):
    @   [E000ED20] = DebugMon prio
    @   [E000ED21] = reserved
    @   [E000ED22] = PendSV  prio
    @   [E000ED23] = SysTick prio
    @
    @ ARMv6-M priority is in bits [7:6] of each byte (only 2 bits
    @ implemented on M0).  0xC0 = priority 3 (lowest), 0x40 = priority 1.
    @
    @ CRITICAL: SysTick MUST be a HIGHER priority than PendSV.  When
    @ ThreadX has no ready thread to schedule, PendSV enters its
    @ __tx_ts_wait idle-wait loop and stays there indefinitely (it
    @ never returns until a thread becomes ready).  If SysTick is at
    @ the same priority as PendSV, ARMv6-M will not let SysTick
    @ preempt the running PendSV, so SysTick's tick-and-wake-sleeping-
    @ threads work never runs.  The wait loop becomes a deadlock —
    @ exactly what an earlier (incorrect) tx_initialize_low_level.S
    @ revision produced on TIME_SLEEP_001.
    @
    @ Mirroring the upstream eclipse-threadx M0/GNU example_build:
    @   SysTick = 0x40 (priority 1, higher than PendSV)
    @   PendSV  = 0xFF (priority 3, lowest)
    LDR     r0, =0xE000ED20
    LDR     r1, =0x40FF0000         @ SysTick=0x40, PendSV=0xFF, reserved/DbgMon=0
    STR     r1, [r0]

    BX      lr


@/* SysTick_Handler — the chip-level SysTick exception entry.
@ *
@ * IOsonata declares SysTick_Handler weak in its vector table, so this
@ * strong definition takes precedence at link time.  The wrapper's
@ * sole job is to call _tx_timer_interrupt with the M0-correct
@ * PUSH/POP-around-LR pattern (ARMv6-M can't push/pop LR directly
@ * with high-register addressing, so we save it in r1 and copy back).
@ */
    .global SysTick_Handler
    .thumb_func
SysTick_Handler:
    PUSH    {r0, lr}
    BL      _tx_timer_interrupt
    POP     {r0, r1}
    MOV     lr, r1
    BX      lr


@ End of file.  ThreadX's PendSV handler (__tx_PendSVHandler) lives
@ in tx_thread_schedule.S; the bridge from PendSV_Handler -> __tx_PendSVHandler
@ is provided in kvb_threadx_glue.c using a function alias attribute.
