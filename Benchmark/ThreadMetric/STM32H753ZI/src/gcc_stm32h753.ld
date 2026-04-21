/*
 * gcc_stm32h753.ld - linker script for STM32H753ZI on NUCLEO-H753ZI.
 *
 *   FLASH : 2 MB  @ 0x08000000
 *   DTCM  : 128 KB @ 0x20000000  (data + main stack — fastest SRAM,
 *                                  tightly coupled to the CPU, no cache
 *                                  coherency concerns, plenty of room)
 *
 * Other STM32H7 memories (ITCM @ 0x00000000, AXI-SRAM @ 0x24000000, SRAM1-4
 * in D2/D3 power domains) are intentionally NOT mapped in this script —
 * Thread-Metric's data footprint fits comfortably in the 128 KB DTCM, and
 * keeping only one SRAM region simplifies the memory map for the benchmark.
 * If a future application needs them, add them here and place sections
 * explicitly.
 *
 * MSP starts at _estack (top of DTCM) and grows down. TaktOS is
 * static-allocation-only; a small heap reservation is kept for safety but
 * sized to 0 by default.
 */

ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x0;      /* No heap — TaktOS never calls malloc. */
_Min_Stack_Size = 0x1000;   /* 4 KB MSP reserve — ISRs + pre-scheduler init. */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM   (rw)  : ORIGIN = 0x20000000, LENGTH = 128K    /* DTCM-RAM */
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7) *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
