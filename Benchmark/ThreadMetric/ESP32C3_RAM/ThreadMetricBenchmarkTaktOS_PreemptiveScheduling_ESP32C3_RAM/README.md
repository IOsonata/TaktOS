# ESP32-C3 RAM bring-up notes

This target is intentionally a **RAM-loaded JTAG/OpenOCD bring-up** target.
It is meant to get the Thread-Metric harness alive before you solve the full
ESP32-C3 image/bootloader flow.

## Assumptions
- `riscv32-esp-elf-*` toolchain
- ESP-IDF ROM linker scripts are available on the linker search path:
  - `esp32c3.rom.ld`
  - `esp32c3.rom.api.ld`
- ELF is loaded directly to RAM by debugger / OpenOCD

## What works in this pack
- `_start`
- stack / gp / mtvec setup
- `.bss` clear
- ROM-backed console output
- benchmark project structure

## What you still need for a full TaktOS port
- `TaktOSStackInit()` for ESP32-C3/RISC-V
- `TaktOSStartFirst()`
- tick source init and ISR routing
- context switch path from your trap/interrupt handler into TaktOS

## Why default CPU clock is 80 MHz here
ESP32-C3 ROM / APB defaults in Espressif headers are 80 MHz. Keep the benchmark
honest until you add your own clock-init path and then update
`TM_ESP32C3_CORE_CLOCK_HZ`.
