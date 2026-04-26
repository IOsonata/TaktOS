# Thread-Metric — TaktOS on ATSAM4LC8C

Cortex-M4 @ 48 MHz, no FPU, soft-float ABI. Mirrors the nRF52832 /
nRF54L15 benchmark suites in structure: every project links the same
shared sources from `Benchmark/ThreadMetric/src/` and the same shared
headers from `Benchmark/ThreadMetric/include/`. The SAM4L-specific
pieces — UART pins, core clock, SW IRQ, NVIC priority setup — live in
a single `include/board.h` that the shared `main.cpp` and
`tm_port_taktos.cpp` consume through `#include "board.h"`.

## Tests included

Same six TaktOS scheduling/object tests as the nRF52832 port, plus the
TaktOS mutex-barging extension. MemoryAllocation is intentionally
excluded: TaktOS has no heap, and the official TM5 cannot be benchmarked
fairly through a static free-list.

- `TaktOS_SAM4LC8C_BasicProcessing` — TM1
- `TaktOS_SAM4LC8C_CooperativeScheduling` — TM2
- `TaktOS_SAM4LC8C_PreemptiveScheduling` — TM3
- `TaktOS_SAM4LC8C_MessageProcessing` — TM6
- `TaktOS_SAM4LC8C_SynchronizationProcessing` — TM7
- `TaktOS_SAM4LC8C_MutexProcessing` — TM8 (TaktOS extension)
- `TaktOS_SAM4LC8C_MutexBargingTest` — TaktOS extension

## File layout

```
Benchmark/ThreadMetric/SAM4LC8C/
├── README.md                                  (this file)
├── include/
│   └── board.h                                SAM4L pin / clock / SW IRQ macros
├── src/
│   └── legacy/                                pre-shared-scheme port files (reference only)
└── TaktOS_SAM4LC8C_<TestName>/
    └── Eclipse/
        ├── .project                           links shared main.cpp / tm_port_taktos.cpp /
        │                                      tm_report.cpp / tm_api.h plus the per-test source
        ├── .cproject                          M4 / soft-float / IOsonata_SAM4LCxC + TaktOS_M4
        ├── .gitignore
        └── .settings/language.settings.xml
```

There is intentionally no per-project `src/` directory anymore — every
source file each project compiles is linked from
`Benchmark/ThreadMetric/src/` or `Benchmark/ThreadMetric/include/` via
`PARENT-3-PROJECT_LOC` references in `.project`.

## Eclipse project settings (Debug and Release)

- **MCU**: `-mcpu=cortex-m4 -mthumb`
- **FPU / float ABI**: `-mfloat-abi=soft`, FPU Type = *none* — SAM4L (C
  variant) has no FPU
- **C++ standard**: `-std=gnu++23`, no RTTI, no exceptions
- **Defined symbols**: `__SAM4LC8C__`, `__PROGRAM_START`
- **Linker script**:
  `${iosonata_loc}/IOsonata/ARM/Microchip/SAM4L/ldscript/gcc_sam4lx8.ld`
- **Libraries**: `TaktOS_M4`, `IOsonata_SAM4LCxC`
- **Library search paths**:
  `${iosonata_loc}/IOsonata/ARM/Microchip/SAM4L/SAM4LCxC/lib/Eclipse/<Debug|Release>`
  plus the local `ARM/cm4/Eclipse/<Debug|Release>` for `libTaktOS_M4.a`

If your IOsonata tree places the compiled SAM4L library at a different
path, edit the matching `listOptionValue` in each `.cproject`.

## board.h — what it supplies

`include/board.h` exposes a small, fixed set of macros and inline
helpers that the shared sources rely on. It is the only SAM4L-specific
glue in this directory.

| Macro / inline                | Purpose                                                              |
|-------------------------------|----------------------------------------------------------------------|
| `TM_CORE_CLOCK_HZ`            | 48 000 000 — CPU clock after IOsonata `SystemInit`                   |
| `UART_DEVNO`                  | 1 — IOsonata DevNo for USART1                                        |
| `UART_TX_PORT/PIN/PINOP`      | PortC=2, PC27, function B (PINOP=1) — USART1 TXD                     |
| `UART_RX_PORT/PIN/PINOP`      | PortC=2, PC26, function B (PINOP=1) — USART1 RXD                     |
| `TM_SW_IRQn`                  | `73` — borrowed NVIC line (TRNG slot) for `tm_cause_interrupt`       |
| `TM_SW_IRQ_VECTOR`            | `TRNG_Handler` — symmetry with the other ports; not used by these seven tests |
| `TmCauseInterrupt()`          | inline — pends the SW IRQ via a direct write to NVIC `ISPR`          |
| `TmSetKernelPriorities()`     | inline — drops PendSV / SysTick to lowest NVIC priority              |
| `TmEnableSoftwareInterrupt()` | inline — sets the borrowed-IRQ priority and enables it               |

Because none of the seven tests exercises `tm_cause_interrupt`, the
TRNG vector itself is left at the IOsonata weak default. The TRNG
peripheral is never clocked or configured. Only the NVIC slot's
priority/pending bits are touched, and only at `tm_initialize()`.

## board.h is self-contained — no SAM4L CMSIS dependency

`board.h` does **not** include any SAM4L device header. The NVIC and
System Control Block live at fixed architectural addresses defined by
the ARM ARM v7-M, so `TmCauseInterrupt` / `TmSetKernelPriorities` /
`TmEnableSoftwareInterrupt` go straight to those addresses (`NVIC_ISPR`
at `0xE000E200`, `NVIC_IPR` at `0xE000E400`, `NVIC_ISER` at
`0xE000E100`, `SCB_SHPR3` at `0xE000ED20`). The previous SAM4L port
(in `src/legacy/`) used the same approach, and the IOsonata
`libIOsonata_SAM4LCxC.a` continues to handle every other peripheral
register access through its own driver layer — `board.h` only deals
with the four ARM-defined system registers.

This means `board.h` builds cleanly without any IOsonata SAM4L CMSIS
include path, regardless of where (or whether) your IOsonata tree
keeps device-pack headers.

## UART pin caveat — PINOP function letter

`UART_TX_PINOP = 1` (function B) is the starting guess based on the
Atmel SAM4L8 Xplained Pro User Guide (Atmel-42103B, §4.3.2) and the
ASF board file
`sam/boards/sam4l8_xplained_pro/sam4l8_xplained_pro.h`. The `PINOP`
field selects which of the eight peripheral functions (A–H = 0–7) maps
PC26/PC27 to USART1.

If output does not appear after flashing:

1. First check the wiring — the EDBG VCOM is on USART1 PC26/PC27 only
   on the SAM4L8 Xplained Pro. The SAM4L-EK uses USART2 on PC11/PC12.
2. If wiring is correct, try `UART_TX_PINOP = 0` (function A), then 2
   (C), and so on. The definitive value is in Table 3-1 "GPIO
   Controller Function Multiplexing — 100-pin Package" of the SAM4L
   datasheet. Whatever value works for TX, mirror it on `UART_RX_PINOP`.

## What changed from the previous SAM4L port

The pre-shared-scheme SAM4L port (now in `src/legacy/`) hard-coded the
SAM4L PM unlock sequence, GPIO multiplexer registers, USART register
offsets, NVIC SHPR3, and STIR-via-ISPR fallback inside a single
`tm_port_taktos.cpp`. That port worked, but it duplicated work that
IOsonata's `UART` driver already does correctly (clock gating, pin-mux,
baud calc, FIFO/DMA), and it had a different API surface than the
nRF52832 / nRF54L15 ports — making cross-MCU comparisons harder than
they needed to be.

The new scheme keeps every line of SAM4L specifics in `include/board.h`
(no register pokes — only CMSIS calls), reuses the shared
`tm_port_taktos.cpp` and `tm_report.cpp` from
`Benchmark/ThreadMetric/src/`, and reuses the shared `main.cpp` that
brings up the IOsonata `UART` console. The result is one numerical set
generated by exactly the same C++ source as the other targets, modulo
the M4 soft-float ABI and SAM4L-specific BSP library link.
