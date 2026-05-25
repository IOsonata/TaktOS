# TaktOS RISC-V architecture layer

**Status (May 2026):** the `RISCV/rv32/` arch lib is the first deliverable RISC-V port. It builds `libTaktOS_RV32.a` for `rv32imac_zicsr / ilp32` and ships CLINT-compatible weak defaults for `TaktOSTickInit` and `TaktHalTrapDispatch`. The port is compile-tested and follows the same file structure as the ARM ports; on-target validation (KVB / Thread-Metric on RV32 silicon) is pending. The legacy `RISCV/esp32c3/` tree is a design sketch only and is not built. Treat ARM as the production target and RV32 as bring-up.

---

## Layout

TaktOS keeps the RISC-V port split the same way as the ARM series:

- `RISCV/include/` holds the generic RISC-V architecture headers used by the
  portable kernel.
- `RISCV/rv32/` is the standard RV32 architecture port used by CLINT-style
  machine-timer / software-interrupt targets.
- `RISCV/esp32c3/` is a separate non-standard Espressif sketch kept for
  reference. ESP32-C3 has no CLINT - its timer / interrupt fabric is
  routed through SYSTIMER + INTMTX. A production ESP32-C3 build overrides
  the weak CLINT defaults with strong defs supplied by the application.

The kernel does not pull in a vendor HAL here. A target port contributes only
what TaktOS itself needs:

- thread frame sizing (`port_rv32.h` - 34-word frame: x1-x31 + mepc + mstatus, 136 B)
- deferred context-switch trigger (write 1 to `*g_TaktSoftIntReg`)
- tick source (CLINT mtime / mtimecmp by default; strong override per chip)
- trap dispatch (CLINT MSIP + MTIP by default; strong override per chip)
- first-task launcher

## Shared headers

- `include/TaktKernelCore.h` - `TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD`,
  `TAKT_DEFAULT_SOFT_INT_ADDR` (= `0x02000000u`, standard CLINT MSIP)
- `include/TaktOSCriticalSection.h` - `TaktOSEnterCritical` /
  `TaktOSExitCritical` via `csrr` / `csrci` / `csrw mstatus`, plus
  `TaktOSCtxSwitch` inlined as `*g_TaktSoftIntReg = 1`
- `include/TaktKernelTick.h` - tick entry declaration for the trap path
- `include/takt_riscv_port_api.h` - small target-port API surface

## `RISCV/rv32/` - the deliverable port

```
RISCV/rv32/
+-- include/
|   +-- port_rv32.h               # 34-word frame layout, FP gate __riscv_flen
|   +-- TaktKernelCore.h          # thin wrapper around ../include/TaktKernelCore.h
|   +-- TaktKernelTick.h          # thin wrapper
|   +-- TaktOSCriticalSection.h   # thin wrapper
+-- src/
|   +-- ctx_switch.S              # SAFETY BOUNDARY - trap save/restore (rv32imac_zicsr)
|                                 # FP context behind __riscv_flen
|   +-- TaktKernelRiscv.cpp       # stack init + weak CLINT TaktOSTickInit
|                                 # + weak CLINT TaktHalTrapDispatch
+-- Eclipse/
    +-- Debug                     # libTaktOS_RV32.a (Debug)
    +-- Release                   # libTaktOS_RV32.a (Release)
```

`TaktKernelRiscv.cpp` is the RISC-V counterpart to `ARM/src/TaktKernelCM.cpp` - a single file holding the arch kernel: stack init plus the two weak hooks. Strong overrides of `TaktOSTickInit` and `TaktHalTrapDispatch` can come from application code, a chip-specific source file the user writes, or anywhere else convenient. TaktOS is HAL-agnostic.

## Configuration

Pass the chip's software-interrupt MMIO address through `TaktOSCfg_t.SoftIntAddr`:

```c
// CLINT-based chip (FE310, BL616, R9A02G021, ESP32-C6/H2):
TaktOSCfg_t cfg = { .KernClockHz = 16000000u };          // SoftIntAddr defaults to 0x02000000
TaktOSInit(&cfg);

// GD32VF103 (CLINT at non-standard base):
TaktOSCfg_t cfg = { .KernClockHz = 108000000u, .SoftIntAddr = 0xD1000000u };
TaktOSInit(&cfg);

// ESP32-C3 (no CLINT - INTMTX-routed SYSTEM register):
TaktOSCfg_t cfg = { .KernClockHz = 16000000u, .SoftIntAddr = 0x600C0014u };  // SYSTEM_CPU_INTR_FROM_CPU_0
TaktOSInit(&cfg);
```

Hot-path cost is one load of `g_TaktSoftIntReg` plus one store to MMIO (about 3-4 cycles), fully inlined at every `TaktOSCtxSwitch()` call site - the same shape as ARM's hardcoded SCB ICSR write, with one extra load to fetch the chip-supplied address.

For CLINT-compatible chips the weak `TaktHalTrapDispatch` in `TaktKernelRiscv.cpp` handles two `mcause` values without any user code:

- `0x80000007` (machine timer) - re-arm `mtimecmp += period`, call `TaktKernelTickHandler()`, return 0. The kernel pends a swap via `TaktOSCtxSwitch()` when needed; the resulting MSIP fires as the next trap - mirrors the ARM SysTick to PendSV tail-chain.
- `0x80000003` (machine software interrupt) - clear `*g_TaktSoftIntReg`, return 1.
- anything else - return 0; a strong override extends for external IRQs and exceptions.

`mtime` and `mtimecmp` are derived from the single `SoftIntAddr` value at the standard CLINT layout (MSIP at `base + 0x0000`, mtimecmp at `base + 0x4000`, mtime at `base + 0xBFF8`). No second config field is needed for CLINT chips.

## FP variants

Three ABI variants are defined; only `rv32_ilp32` is shipped in this drop:

| ABI | `-march` | `-mabi` | Status |
|---|---|---|---|
| rv32_ilp32 | `rv32imac_zicsr` | `ilp32` | Shipped (`libTaktOS_RV32.a`) |
| rv32_ilp32f | `rv32imafc_zicsr` | `ilp32f` | Deferred until a target chip needs it |
| rv32_ilp32d | `rv32imafdc_zicsr` | `ilp32d` | Deferred until a target chip needs it |

When an FP variant is added it becomes another build configuration in the same `RISCV/rv32/Eclipse/` project, not a separate arch directory. FP context save/restore in `ctx_switch.S` is already gated behind `__riscv_flen`.

## Toolchain

xPack `riscv-none-elf-gcc` 13.2.0 with picolibc for freestanding builds. Same `-std=gnu++23 -fno-exceptions -fno-rtti -Os` flags as the ARM ports.
