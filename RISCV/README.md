# TaktOS RISC-V architecture layer

TaktOS keeps the RISC-V port split the same way as the ARM series:

- `RISCV/include/` holds the generic RISC-V architecture headers used by the
  portable kernel.
- `RISCV/rv32/` is the standard RV32 architecture port used by CLINT-style
  machine timer / software-interrupt targets.
- `RISCV/esp32c3/` remains a separate non-standard Espressif architecture port
  because its timer / interrupt fabric is not generic CLINT-style RV32.

The kernel does not pull in a vendor HAL here.  A target port contributes only
what TaktOS itself needs:

- thread frame sizing
- deferred context-switch trigger
- tick entry declaration when the trap path calls it directly
- first-task launcher declaration

## Shared headers

- `include/TaktKernelCore.h`
- `include/TaktOSCriticalSection.h`
- `include/TaktKernelTick.h`
- `include/takt_riscv_port_contract.h`

## Target wrappers

Each target include directory keeps compatibility with existing include paths by
providing thin wrappers:

- `rv32/include/*.h`
- `esp32c3/include/*.h`

Each wrapper defines the small target-port contract in `takt_riscv_port.h` and
then includes the shared header from `RISCV/include/`.
