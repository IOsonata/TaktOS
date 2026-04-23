/*
 * gcc_stm32l432kc.ld - linker script for STM32L432KC on NUCLEO-L432KC
 *
 *   Flash : 256 KB @ 0x08000000
 *   SRAM1 :  48 KB @ 0x20000000     (primary RAM - used for everything)
 *   SRAM2 :  16 KB @ 0x10000000     (NOT mapped; available if later needed)
 *
 * MSP starts at _estack (top of SRAM1) and grows down. TaktOS is
 * static-allocation-only, so the heap is set to 0.
 */

ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x0;     /* TaktOS is static-allocation-only. */
_Min_Stack_Size = 0x400;   /* 1 KB MSP reserve - pre-scheduler init + ISRs. */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
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

    /* ThreadX port compatibility aliases.
     * threadx_initialize_low_level.S uses CMSIS/Nordic-style symbol names:
     *   __data_end__  - end of initialised-data + BSS, i.e. first free RAM
     *                   (matches the convention used by CMSIS gcc startups)
     *   __Vectors     - vector table base symbol
     * Our tree uses _ebss and g_pfnVectors. Provide the aliases so the
     * ThreadX init assembly resolves against the existing symbols without
     * modifying startup_stm32l432kc.S or threadx_initialize_low_level.S. */
    PROVIDE ( __data_end__ = _ebss );
    PROVIDE ( __Vectors    = g_pfnVectors );

    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
