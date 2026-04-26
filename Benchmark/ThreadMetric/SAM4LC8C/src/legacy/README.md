# Legacy SAM4L Thread-Metric port files

These three files were the per-MCU port and console driver used before
the Thread-Metric harness was migrated to the shared scheme that
nRF52832 / nRF54L15 / SAM4LC8C now all use.

They are kept here for reference only. **None of the seven SAM4LC8C
TaktOS projects in this directory link them.** All projects now link:

- `Benchmark/ThreadMetric/src/main.cpp` — shared, IOsonata `UART` console
- `Benchmark/ThreadMetric/src/tm_port_taktos.cpp` — shared, arch-neutral
- `Benchmark/ThreadMetric/src/tm_report.cpp` — shared, IOsonata `UART` printf
- `Benchmark/ThreadMetric/SAM4LC8C/include/board.h` — SAM4L pin / clock / SW IRQ macros

Files in this folder:

- `tm_port_taktos.cpp` — old port with hard-coded SAM4L PM unlock, GPIO
  mux, USART register pokes, TRNG NVIC weak-alias.
- `tm_port_taktos.c` — C variant of the same.
- `tm_console_iosonata_sam4l8xpro.cpp` — board console driver written
  before the shared `tm_report.cpp` + IOsonata `UART` route was used
  on every target.

Safe to delete once nobody is referring to them.
