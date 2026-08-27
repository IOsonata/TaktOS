/*
 * gcc_ra4m1.ld - linker script for Renesas RA4M1 on Arduino Nano R4.
 *
 *   Flash : 256 KB @ 0x00000000
 *   RAM   :  32 KB @ 0x20000000
 *
 * Data flash (8 KB @ 0x40100000) and option-setting memory are NOT mapped
 * here — they are not used by this benchmark port.
 *
 * MSP starts at _estack (top of RAM) and grows down. TaktOS is
 * static-allocation-only, but a small newlib heap is reserved in case
 * anything inadvertently calls malloc (e.g. sprintf paths that newlib-nano
 * might hit).
 *
 * NOTE on flash origin: this script links the image at 0x00000000 (bare-metal
 * convention — what you get when flashing via SWD with J-Link / CMSIS-DAP).
 * The Arduino factory bootloader, if still present, occupies the low region
 * and exposes DFU over the RA4M1 native USB; uploading via arduino-cli would
 * place the sketch at 0x00004000 (UNO R4 Minima convention) and is NOT the
 * target of this linker script. For DFU-flashed binaries, change FLASH
 * ORIGIN to 0x00004000 and reduce FLASH LENGTH to 240K.
 */

ENTRY(ResetEntry)

_Min_Heap_Size  = 0x0;     /* No heap: TaktOS is static-allocation-only.
                              Setting to 0 avoids wasting RAM that would
                              otherwise go to a never-used newlib heap. */
_Min_Stack_Size = 0x400;   /* 1 KB MSP reserve — ISRs + pre-scheduler init.
                              RA4M1 has 32 KB SRAM so we can afford headroom. */

MEMORY
{
    /* Arduino Nano R4 ships with a DFU bootloader at 0x00000000-0x00003FFF.
     * dfu-util writes user code starting at 0x00004000, so our link base
     * must match. If you remove the bootloader and flash via SWD, change
     * ORIGIN back to 0x00000000 and LENGTH to 256K. */
    FLASH (rx)  : ORIGIN = 0x00004000, LENGTH = 240K
    RAM   (rw)  : ORIGIN = 0x20000000, LENGTH = 32K
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
