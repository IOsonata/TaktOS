OUTPUT_ARCH(riscv)
ENTRY(_start)

/* Pull in official Espressif ROM symbols from ESP-IDF. Put the directory
 * components/esp_rom/esp32c3/ld on the linker search path. */
INCLUDE esp32c3.rom.ld
INCLUDE esp32c3.rom.api.ld

/* RAM-only linker script for debugger-loaded bring-up on ESP32-C3.
 * Derived from Espressif's documented ESP32-C3 IRAM/DRAM layout. */
MEMORY
{
    iram (rx)  : ORIGIN = 0x40380000, LENGTH = 0x00060000
    dram (rwx) : ORIGIN = 0x3FC80000, LENGTH = 0x00060000
    rtc  (rwx) : ORIGIN = 0x50000000, LENGTH = 0x00002000
}

__stack_size = DEFINED(__stack_size) ? __stack_size : 0x00004000;
__heap_size  = DEFINED(__heap_size)  ? __heap_size  : 0x00002000;

SECTIONS
{
    .init : ALIGN(4)
    {
        KEEP(*(.init))
        KEEP(*(.init.*))
    } > iram

    .text : ALIGN(4)
    {
        *(.literal .literal.*)
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } > iram

    .data : ALIGN(16)
    {
        __data_start = .;
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
        *(.data .data.*)
        __data_end = .;
    } > dram

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } > dram

    .stack (NOLOAD) : ALIGN(16)
    {
        __stack_bottom = .;
        . += __stack_size;
        __stack_top = .;
    } > dram

    .heap (NOLOAD) : ALIGN(16)
    {
        __heap_start = .;
        . += __heap_size;
        __heap_end = .;
    } > dram

    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.eh_frame*)
        *(.riscv.attributes)
    }
}
