# Legacy STM32F0308 Thread-Metric port files

These five files were the per-MCU port, console driver, and a parallel
self-contained boot/clock setup used before the Thread-Metric harness
was migrated to the shared scheme that nRF52832 / nRF54L15 / SAM4LC8C /
STM32F0308 now all use.

They are kept here for reference only. **None of the seven
STM32F0308 TaktOS projects in this directory link them.** All projects
now link:

- `Benchmark/ThreadMetric/src/main.cpp` — shared, IOsonata `UART` console
- `Benchmark/ThreadMetric/src/tm_port_taktos.cpp` — shared, arch-neutral
- `Benchmark/ThreadMetric/src/tm_report.cpp` — shared, IOsonata `UART` printf
- `Benchmark/ThreadMetric/STM32F0308/include/board.h` — STM32F0 pin / clock / SW IRQ macros

Files in this folder:

- `tm_port_taktos.cpp` — old per-MCU port with hard-coded SHPR3 / NVIC IPR
  pokes, `TM_TAKTOS_MAX_SLOTS` thread-id compaction table, and the
  `TM_PORT_BOOT_DIAGNOSE` boot blink-out. Superseded by the shared port +
  the inline helpers in `include/board.h`.
- `tm_console_stm32f0308.cpp` — direct-USART1 console driver written
  before the IOsonata STM32F030x8 library shipped a UART driver and
  before the shared `tm_report.cpp` + IOsonata `UART` route was used
  on every target.
- `startup_stm32f030r8.S`, `system_stm32f030.c`, `gcc_stm32f030r8.ld` —
  a parallel self-contained boot path. Not referenced by any of the
  seven `.project` files: every project links the IOsonata-supplied
  startup / linker script via `IOsonata_STM32F030x8` and
  `${iosonata_loc}/IOsonata/ARM/ST/STM32F0xx/ldscript/gcc_stm32f030x.ld`.

Safe to delete once nobody is referring to them.
