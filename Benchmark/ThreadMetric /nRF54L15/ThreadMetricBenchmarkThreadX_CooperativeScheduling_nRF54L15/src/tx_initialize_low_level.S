/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 ***************************************************************************/

    .global     _tx_thread_system_stack_ptr
    .global     _tx_initialize_unused_memory
    .global     __data_end__
    .global     _tx_timer_interrupt
    .global     __tx_SVCallHandler
    .global     __tx_PendSVHandler
    .global     __Vectors
    .global     __tx_NMIHandler
    .global     __tx_BadHandler
    .global     __tx_SVCallHandler
    .global     __tx_DBGHandler
    .global     __tx_PendSVHandler
    .global     __tx_SysTickHandler
    .global     __tx_IntHandler

SYSTEM_CLOCK      =   128000000
SYSTICK_CYCLES    =   ((SYSTEM_CLOCK / 1000) -1)

    .text 32
    .align 4
    .syntax unified

    .global  _tx_initialize_low_level
    .thumb_func
_tx_initialize_low_level:
    CPSID   i

    LDR     r0, =_tx_initialize_unused_memory
    LDR     r1, =__data_end__
    ADD     r1, r1, #4
    STR     r1, [r0]

    MOV     r0, #0xE000E000
    LDR     r1, =__Vectors
    STR     r1, [r0, #0xD08]

    LDR     r0, =_tx_thread_system_stack_ptr
    LDR     r1, =__Vectors
    LDR     r1, [r1]
    STR     r1, [r0]

    LDR     r0, =0xE0001000
    LDR     r1, [r0]
    ORR     r1, r1, #1
    STR     r1, [r0]

    MOV     r0, #0xE000E000
    LDR     r1, =SYSTICK_CYCLES
    STR     r1, [r0, #0x14]
    MOV     r1, #0x7
    STR     r1, [r0, #0x10]

    LDR     r1, =0x00000000
    STR     r1, [r0, #0xD18]

    LDR     r1, =0xFF000000
    STR     r1, [r0, #0xD1C]

    LDR     r1, =0x40FF0000
    STR     r1, [r0, #0xD20]

    BX      lr

    .global  __tx_BadHandler
    .thumb_func
__tx_BadHandler:
    B       __tx_BadHandler

    .global  __tx_HardfaultHandler
    .thumb_func
__tx_HardfaultHandler:
    B       __tx_HardfaultHandler

    .global  __tx_SVCallHandler
    .thumb_func
__tx_SVCallHandler:
    B       __tx_SVCallHandler

    .global  __tx_IntHandler
    .thumb_func
__tx_IntHandler:
    PUSH    {r0, lr}
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_enter
#endif
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_exit
#endif
    POP     {r0, lr}
    BX      lr

    .global  __tx_SysTickHandler
    .global  SysTick_Handler
    .thumb_func
__tx_SysTickHandler:
    .thumb_func
SysTick_Handler:
    PUSH    {r0, lr}
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_enter
#endif
    BL      _tx_timer_interrupt
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    BL      _tx_execution_isr_exit
#endif
    POP     {r0, lr}
    BX      lr

    .global  __tx_NMIHandler
    .thumb_func
__tx_NMIHandler:
    B       __tx_NMIHandler

    .global  __tx_DBGHandler
    .thumb_func
__tx_DBGHandler:
    B       __tx_DBGHandler
