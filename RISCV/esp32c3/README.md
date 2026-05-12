# TaktOS — ESP32-C3 / ESP32-C6 RISC-V Port

Bare-metal TaktOS port for Espressif **ESP32-C3** (RV32IMC) and
**ESP32-C6** (RV32IMAC + Zbb) running entirely in machine mode (no
supervisor, no OS underneath).

---

## File map

```
RISCV/
├── include/
│   ├── TaktOSCriticalSection.h    ← generic RISC-V critical section API
│   ├── TaktKernelCore.h           ← generic RISC-V stack/frame sizing
│   └── TaktKernelTick.h           ← generic tick declaration shim
└── esp32c3/
    ├── include/
    │   ├── takt_riscv_port.h      ← target-port macros for ESP32-C3/C6
    │   ├── TaktOSCriticalSection.h← thin wrapper over RISCV/include
    │   └── TaktKernelTick.h       ← thin wrapper over RISCV/include
    ├── ctx_switch_rv32.S          ← SAFETY BOUNDARY: unified trap handler + TaktOSStartFirst
    └── src/
        ├── TaktKernelRV32_esp32c3.cpp ← SAFETY BOUNDARY: TaktKernelStackInit + TaktOSTickInit
        ├── systimer_esp32c3.h         ← ESP32-C3/C6 SYSTIMER land-layer (inline)
        └── intmtx_esp32c3.h           ← ESP32-C3/C6 interrupt matrix land-layer (inline)
```

---

## ISA differences from the GD32VF103 (CLINT) port

| Feature            | GD32VF103 (v1.0 RISC-V)   | ESP32-C3 / C6          |
|--------------------|---------------------------|------------------------|
| Context switch trigger | CLINT MSIP (0x02000000) | SYSTEM_CPU_INTR_FROM_CPU_0_REG (0x600C0014) via interrupt matrix |
| Tick source        | CLINT mtime / mtimecmp    | SYSTIMER TARGET0 (80 MHz fixed clock) |
| Tick interrupt     | Machine timer (mcause=7)  | External interrupt via interrupt matrix (mcause=11) |
| Interrupt router   | None (CLINT + PLIC)       | Interrupt matrix (INTERRUPT_CORE0) |
| Zbb support        | Optional (GD32VF103 = no) | C3: no; C6: yes (pass -march=..._zbb) |

---

## Context-switch mechanism (PendSV analog)

ESP32-C3/C6 have no CLINT and cannot trigger the machine software interrupt
(mip.MSIP) via software on a standard register.  Instead, TaktOS uses:

```
TaktOSCtxSwitch()  →  write 1 to SYSTEM_CPU_INTR_FROM_CPU_0_REG
                       (0x600C0014, bit 0)
                   →  fires CPU INT 29 through interrupt matrix
                   →  mcause = 0x8000000B (external interrupt)
                   →  _takt_trap_rv32 dispatches on EIP_STATUS bit 29
                   →  full register-save context switch
                   →  clear SYSTEM_CPU_INTR_FROM_CPU_0_REG before mret
```

CPU INT 29 is configured at **priority 1** (lowest configurable).  The SYSTIMER
tick runs at **priority 2**.  The interrupt threshold is **0** (all fire).
This guarantees the same ordering as ARM SysTick → PendSV: tick fires first,
deferred switch fires after all higher-priority work completes.

---

## CPU interrupt allocation

| CPU INT | Source                    | Type  | Priority | Role              |
|---------|---------------------------|-------|----------|-------------------|
| 2       | SYSTIMER_TARGET0 (src 37) | level | 2        | OS tick           |
| 29      | CPU_INTR_FROM_CPU_0 (src 0) | edge | 1      | Deferred ctx switch |

---

## Context frame (128 bytes, 32 words)

All 30 general-purpose registers (x1–x31 except x2/sp) plus `mepc` and
`mstatus` are saved on the current task's stack.  `x2` (sp) is stored
directly in `TaktOSThread_t.pSp`.

Frame size: **128 bytes** per task context.  Allocate stacks accordingly
(add 128 bytes of headroom beyond the expected peak stack depth).

---

## Toolchain

| Item           | Value |
|----------------|-------|
| Compiler       | `riscv32-unknown-elf-g++` (xPack GNU RISC-V Embedded GCC) |
| C++ standard   | `-std=gnu++23 -fno-exceptions -fno-rtti` |
| ISA flags (C3) | `-march=rv32imc -mabi=ilp32` |
| ISA flags (C6) | `-march=rv32imafc_zbb -mabi=ilp32f` |
| Defines        | `TAKT_ARCH_RISCV=1` |

---

## Eclipse project include paths

Add to the Eclipse project include path:
```
RISCV/esp32c3/include      (target-port wrappers for the shared RISC-V headers)
RISCV/include              (generic RISC-V architecture headers)
include/                   (portable kernel headers)
```

---

## ESP32-C6 support

ESP32-C6 is a **drop-in extension** of this port:

- Same register addresses for SYSTIMER, interrupt matrix, and SYSTEM_CPU_INTR.
- Same `ctx_switch_rv32.S` and `TaktKernelRV32_esp32c3.cpp`.
- Compile with `-march=rv32imafc_zbb` to enable hardware `clz` (Zbb).
  `__builtin_clz()` in TaktReadyTask/TaktBlockTask automatically emits
  the 1-cycle `clz` instruction — no source changes required.

---

## Open items

1. **mcycle validation** — measure context switch latency on ESP32-C3
   using the `mcycle` CSR.  Design target: ≤108 cycles at 160 MHz.
   Add `bench/esp32c3/` benchmark project (modelled on `bench/gd32vf103/`).

2. **Stack overflow detection** — `TAKTOS_PMP_ENABLE=1` guard uses PMP entry 0
   (NAPOT, 32-byte no-access region at `TCB.pStackBottom`).  Verify PMP
   enforcement on ESP32-C3 silicon.

3. **FPU context (ESP32-C6 only)** — ESP32-C6 has the F extension.
   Add `TAKTOS_FP_SAVE=1` gate to save/restore f0–f31 + fcsr in the frame.
   Extend frame by 33 words (132 bytes) when enabled.

4. **Wi-Fi / BLE co-existence** — ESP32-C3/C6 Wi-Fi ISRs run at elevated
   machine-mode interrupt priority.  Verify that TaktOS CPU INT priorities
   do not clash with Espressif ROM/IDF reserved CPU INTs (0–6 are typically
   reserved by ROM).  Consider shifting tick to CPU INT 8 and ctx to CPU INT 7.
