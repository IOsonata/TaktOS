# Thread-Metric — TaktOS on STM32F030R8 (STM32F0308-DISCO)

Cortex-M0 @ 48 MHz, ARMv6-M, no FPU, soft-float ABI. Mirrors the
nRF52832 / nRF54L15 / SAM4LC8C benchmark suites in structure: every
project links the same shared sources from `Benchmark/ThreadMetric/src/`
and the same shared headers from `Benchmark/ThreadMetric/include/`.
The STM32F0-specific pieces — UART pins, core clock, SW IRQ, NVIC
priority setup — live in a single `include/board.h` that the shared
`main.cpp` and `tm_port_taktos.cpp` consume through `#include "board.h"`.

## Tests included

Same six TaktOS scheduling/object tests as the other ports, plus the
TaktOS mutex-barging extension. MemoryAllocation is intentionally
excluded: TaktOS has no heap, and the official TM5 cannot be benchmarked
fairly through a static free-list.

- `TaktOS_STM32F0308_BasicProcessing` — TM1
- `TaktOS_STM32F0308_CooperativeScheduling` — TM2
- `TaktOS_STM32F0308_PreemptiveScheduling` — TM3
- `TaktOS_STM32F0308_MessageProcessing` — TM6
- `TaktOS_STM32F0308_SynchronizationProcessing` — TM7
- `TaktOS_STM32F0308_MutexProcessing` — TM8 (TaktOS extension)
- `TaktOS_STM32F0308_MutexBargingTest` — TaktOS extension

## File layout

```
Benchmark/ThreadMetric/STM32F0308/
├── README.md                                  (this file)
├── include/
│   └── board.h                                STM32F0 pin / clock / SW IRQ macros
├── src/
│   └── legacy/                                pre-shared-scheme port files (reference only)
└── TaktOS_STM32F0308_<TestName>/
    └── Eclipse/
        ├── .project                           links shared main.cpp / tm_port_taktos.cpp /
        │                                      tm_report.cpp / tm_api.h plus the per-test source
        ├── .cproject                          M0 / soft-float / IOsonata_STM32F030x8 + TaktOS_M0
        ├── .gitignore
        └── .settings/language.settings.xml
```

There is intentionally no per-project `src/` directory — every source
file each project compiles is linked from `Benchmark/ThreadMetric/src/`
or `Benchmark/ThreadMetric/include/` via `PARENT-3-PROJECT_LOC`
references in `.project`.

## Eclipse project settings (Debug and Release)

- **MCU**: `-mcpu=cortex-m0 -mthumb`
- **FPU / float ABI**: `-mfloat-abi=soft`, FPU Type = *default* — STM32F030R8 has no FPU
- **C++ standard**: `-std=gnu++23`, no RTTI, no exceptions
- **Defined symbols**: `STM32F030x8` (selects ST CMSIS device header), `__PROGRAM_START`
- **Linker script**:
  `${iosonata_loc}/IOsonata/ARM/ST/STM32F0xx/ldscript/gcc_stm32f030x.ld`
- **Libraries**: `TaktOS_M0`, `IOsonata_STM32F030x8`
- **Library search paths**:
  `${iosonata_loc}/IOsonata/ARM/ST/STM32F0xx/STM32F030x8/lib/Eclipse/<Debug|Release>`
  plus the local `ARM/cm0/Eclipse/<Debug|Release>` for `libTaktOS_M0.a`

If your IOsonata tree places the compiled STM32F0 library at a different
path, edit the matching `listOptionValue` in each `.cproject`.

## board.h — what it supplies

`include/board.h` exposes a small, fixed set of macros and inline
helpers that the shared sources rely on. It is the only STM32F0-specific
glue in this directory.

| Macro / inline                | Purpose                                                              |
|-------------------------------|----------------------------------------------------------------------|
| `TM_CORE_CLOCK_HZ`            | 48 000 000 — CPU clock after IOsonata `SystemInit` (HSI -> PLL ×6)   |
| `UART_DEVNO`                  | 1 — IOsonata DevNo for USART1                                        |
| `UART_TX_PORT/PIN/PINOP`      | PortA=0, PA9, AF1 — USART1 TXD                                       |
| `UART_RX_PORT/PIN/PINOP`      | PortA=0, PA10, AF1 — USART1 RXD                                      |
| `TM_SW_IRQn`                  | `22` — borrowed NVIC line (TIM17 slot) for `tm_cause_interrupt`      |
| `TM_SW_IRQ_VECTOR`            | `TIM17_IRQHandler` — symmetry with the other ports; not used by these seven tests |
| `TmCauseInterrupt()`          | inline — pends the SW IRQ via a direct write to NVIC `ISPR`          |
| `TmSetKernelPriorities()`     | inline — drops PendSV / SysTick to lowest NVIC priority via SHPR3    |
| `TmEnableSoftwareInterrupt()` | inline — sets the borrowed-IRQ priority and enables it               |

Because none of the seven tests exercises `tm_cause_interrupt`, the
TIM17 vector itself is left at the IOsonata weak default. The TIM17
peripheral is never clocked or configured. Only the NVIC slot's
priority/pending bits are touched, and only at `tm_initialize()`.

## board.h is self-contained — no STM32F0 CMSIS dependency

`board.h` does **not** include any STM32F0 device header. The NVIC and
System Control Block live at fixed architectural addresses defined by
the ARMv6-M ARM, so the four register accesses inside the inline
helpers go straight to those addresses (`NVIC_ISPR` at `0xE000E200`,
`NVIC_IPR` at `0xE000E400`, `NVIC_ISER` at `0xE000E100`, `SCB_SHPR3`
at `0xE000ED20`). The previous STM32F0308 port (in `src/legacy/`) used
the same approach, and the IOsonata `libIOsonata_STM32F030x8.a`
continues to handle every other peripheral register access through its
own driver layer.

## Cortex-M0 caveat — word-access NVIC IPR

ARMv6-M (Cortex-M0) requires word-aligned access to the NVIC IPR
registers. Byte access faults — unlike the Cortex-M4 ports
(SAM4LC8C, nRF52832) where NVIC_IPR can be written one byte per IRQ.
`TmEnableSoftwareInterrupt()` therefore does a read-modify-write on the
32-bit IPR word that holds IRQ 22's priority byte (IPR\[5\], shift 16).
If you copy this `board.h` to another Cortex-M0 / M0+ target, that
read-modify-write idiom must stay; if you copy it to another Cortex-M4
target, the M4 byte-access version (see SAM4LC8C `board.h`) is fine.

## UART pin mapping

`UART_TX_PINOP = 1` (AF1) is correct for USART1 on PA9/PA10 per the
STM32F030R8 datasheet (Table 14 "Alternate function mappings"). Both
the STM32F0308-DISCO and the NUCLEO-F030R8 route the ST-LINK/V2-1
Virtual COM Port to USART1 PA9/PA10, so the same `board.h` works on
either board out of the box.

If you need a different USART, the mapping is:

- USART2 on PA2/PA3 → AF1 (PINOP = 1)
- USART1 on PA9/PA10 → AF1 (PINOP = 1) ← default here
- USART1 on PB6/PB7 → AF0 (PINOP = 0) — alternative pin set

## What changed from the previous STM32F0 port

The pre-shared-scheme STM32F0 port (now in `src/legacy/`) hard-coded
the USART1 register pokes, GPIO multiplexer registers, NVIC IPR / ISER /
ISPR, SCB SHPR3, and a per-thread compaction table sized by
`TM_TAKTOS_MAX_SLOTS` per project. It also carried a parallel
self-contained boot path (`startup_stm32f030r8.S`, `system_stm32f030.c`,
`gcc_stm32f030r8.ld`) that was never linked into any of the seven
`.project` files.

The new scheme keeps every line of STM32F0 specifics in
`include/board.h` (no peripheral register pokes — only the four
ARMv6-M architectural addresses), reuses the shared
`tm_port_taktos.cpp` and `tm_report.cpp` from
`Benchmark/ThreadMetric/src/`, and reuses the shared `main.cpp` that
brings up the IOsonata `UART` console. The result is one numerical set
generated by exactly the same C++ source as the other targets, modulo
the M0 instruction set and the `IOsonata_STM32F030x8` library link.
