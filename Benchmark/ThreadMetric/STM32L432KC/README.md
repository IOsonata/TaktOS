# Thread-Metric benchmarks — NUCLEO-L432KC (TaktOS, FreeRTOS, ThreadX)

Three RTOS variants of the Thread-Metric benchmark suite, all running on the
same hardware under identical clock / memory / UART configuration so the
numbers are directly comparable.

**Target:** NUCLEO-L432KC — STM32L432KCU6, Cortex-M4F @ 80 MHz, 256 KB flash,
48 KB SRAM1.

## Project index

| RTOS      | Projects | Tests                                                      |
|-----------|---------:|------------------------------------------------------------|
| TaktOS    |        7 | Basic, Cooperative, Preemptive, Message, Synchronization, Mutex, MutexBarging |
| FreeRTOS  |        7 | (same 7)                                                   |
| ThreadX   |        7 | (same 7)                                                   |

Total: 21 Eclipse projects. Naming convention mirrors the existing
`nRF52832/` and `nRF54L15/` trees:
`ThreadMetricBenchmark<RTOS>_<Test>_STM32L432KC`.

## Layout

```
STM32L432KC/
├── README.md                            (this file)
├── src/                                 (shared across all 21 projects)
│   ├── startup_stm32l432kc.S            IOsonata-style startup + vector table
│   ├── system_stm32l432kc.c             HSI16→PLL→80 MHz, 4 WS flash, FPU enable
│   ├── gcc_stm32l432kc.ld               linker script
│   ├── tm_console_stm32l432kc.cpp       USART2 on PA2 TX / PA15 RX (ST-LINK VCP)
│   ├── main.c                           shared main for FreeRTOS/ThreadX
│   ├── tm_port_taktos.cpp               Thread-Metric → TaktOS shim
│   ├── tm_port_freertos.c               Thread-Metric → FreeRTOS shim
│   ├── tm_port_threadx.c                Thread-Metric → ThreadX shim
│   ├── threadx_initialize_low_level.S   ThreadX Cortex-M4 low-level init (80 MHz)
│   └── FreeRTOSConfig.h                 FreeRTOS kernel configuration
├── ThreadMetricBenchmarkTaktOS_<test>_STM32L432KC/
│   ├── src/main.cpp                     (C++ main for TaktOS)
│   └── Eclipse/{.project, .cproject, .gitignore, .settings/}
├── ThreadMetricBenchmarkFreeRTOS_<test>_STM32L432KC/
│   └── Eclipse/{.project, .cproject, .gitignore, .settings/}
└── ThreadMetricBenchmarkThreadX_<test>_STM32L432KC/
    └── Eclipse/{.project, .cproject, .gitignore, .settings/}
```

The FreeRTOS and ThreadX projects don't have their own `src/` folder — they
pull `main.c`, their kernel config, and their port file from the shared
`STM32L432KC/src/` directory via `PARENT-2-PROJECT_LOC` links. This matches
the pattern Hoan already tested on TaktOS.

## How the build resolves files

The Eclipse `.project` uses `PARENT-n-PROJECT_LOC` links:

| Link pattern                          | Resolves to                              |
|---------------------------------------|------------------------------------------|
| `PARENT-1-PROJECT_LOC/src/main.cpp`   | `<proj>/src/main.cpp` (TaktOS only)      |
| `PARENT-2-PROJECT_LOC/src/<file>`     | `STM32L432KC/src/<file>` (shared)        |
| `PARENT-3-PROJECT_LOC/src/<test>.c`   | `ThreadMetric/src/<test>.c`              |
| `PARENT-3-PROJECT_LOC/include/...`    | `ThreadMetric/include/{tm_api.h,tx_user.h}` |

**Kernel sources:**

- **TaktOS** — linked from `ARM/cm4/Eclipse/ReleaseFPU/libTaktOS_M4.a`.
- **FreeRTOS** — kernel `.c` files linked in-place via
  `IOCOMPOSER_HOME/external/FreeRTOS-Kernel/`. Run
  `nRF54L15/fetch_freertos_kernel.sh` (or the equivalent) once to populate
  that tree.
- **ThreadX** — common + cortex-m4/gnu port files linked in-place via
  `IOCOMPOSER_HOME/external/threadx/`. 214 files in total. Same convention
  as the existing `nRF52832` / `nRF54L15` ThreadX projects.

## Hardware notes (NUCLEO-L432KC, UM1956)

| Item          | Setting                                                |
|---------------|--------------------------------------------------------|
| MCU           | STM32L432KCU6 (Cortex-M4F, 80 MHz)                     |
| Clock source  | HSI16 (16 MHz internal RC)                             |
| PLL           | /M=1, ×N=10, /R=2 → 80 MHz SYSCLK                      |
| Voltage range | VCORE Range 1 (required above 26 MHz)                  |
| Flash         | 4 WS, prefetch + ICache + DCache enabled               |
| UART VCP      | USART2, PA2 TX (AF7), PA15 RX (AF3), 115200 8N1        |
| SW IRQ        | TIM7_IRQn (55) — pended only, TIM7 never clocked       |
| FPU           | FPv4-SP-D16, hard float ABI                            |

## RTOS vector-table arbitration

All three RTOSes share `startup_stm32l432kc.S`, which declares
`PendSV_Handler`, `SVC_Handler`, and `SysTick_Handler` as weak aliases of
`Default_Handler`. Each RTOS overrides them with strong symbols:

| RTOS     | Provides strong symbols from …                                     |
|----------|--------------------------------------------------------------------|
| TaktOS   | `libTaktOS_M4.a` (PendSV_M4.S) — lowest-priority PendSV            |
| FreeRTOS | `port.c` via `xPortPendSVHandler` / `xPortSysTickHandler` macros   |
| ThreadX  | `threadx_initialize_low_level.S` + `tx_thread_schedule.S` etc.     |

`TIM7_IRQHandler` is overridden at strong-symbol level by the RTOS-specific
`tm_port_*.{c,cpp}` to carry the Thread-Metric software interrupt.

## Building

Eclipse workspace: import any project directory as an existing project.
The `.project` links pick up the shared `STM32L432KC/src/` files
automatically. No per-project file copying needed.

Command-line build (Debug config, after Eclipse has generated the makefile):
```
cd ThreadMetricBenchmark<RTOS>_<Test>_STM32L432KC/Eclipse
make -f ../Debug/makefile all
```

## Flashing

Use ST-LINK (on-board) via `openocd -f board/st_nucleo_l4.cfg`, or
STM32CubeProgrammer. ELF is produced at `Debug/<ProjName>.elf`.

## Console

Open the ST-LINK VCP at **115200 8N1**. Results print every 30 s during the
benchmark window — the standard Thread-Metric reporting cadence.

## Benchmark fairness — canonical metrics

All three RTOSes run with identical port-layer resource limits so Thread-Metric
numbers are directly comparable across TaktOS, FreeRTOS, and ThreadX on this
board. Any difference in measured throughput is attributable to the kernel
being tested, not to how the port was sized.

| Resource                      | Value                                       |
|-------------------------------|---------------------------------------------|
| Max threads                   | 15                                          |
| Stack per thread              | 1024 bytes (256 × 32-bit words)             |
| Queue depth × message size    | 10 × 16 bytes                               |
| Semaphores                    | 1 (binary)                                  |
| Mutexes                       | 1 (with priority inheritance where available) |
| Memory pool                   | 2048 bytes / 128-byte blocks                |
| Tick rate                     | 1000 Hz                                     |
| PendSV priority               | `0xF0` (lowest) — all three                 |
| SW-IRQ (`TIM7_IRQn`) priority | `0x80` — all three                          |
| CPU clock                     | 80 MHz (HSI16 → PLL ×10 / 2)                |
| Toolchain flags               | same for all (from the shared `.cproject`)  |

`SysTick` priority differs by RTOS architecture and is not a tuning knob:
TaktOS and FreeRTOS place `SysTick` at `0xF0` alongside PendSV; ThreadX places
it at `0x40` (higher priority than PendSV) as a deliberate design choice in
`threadx_initialize_low_level.S` so tick processing preempts context switches.
Leaving it alone preserves ThreadX's intended timer semantics.

## Compile-time feature parity

All three RTOSes are built with the same kernel-level safety features
enabled so each RTOS does equivalent work per API call. Without this, a
faster "ThreadX-with-no-error-checking" or "FreeRTOS-with-no-stack-check"
number would be comparing different product shapes, not different kernel
performance.

| Feature                         | TaktOS            | FreeRTOS                                      | ThreadX                                  |
|---------------------------------|-------------------|-----------------------------------------------|------------------------------------------|
| FPU instructions compiled in    | `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`    | `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`                                | `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`                           |
| FPU context save at RTOS level  | built-in          | `vPortEnableVFP()` in `ARM_CM4F`/`ARM_CM7` port | `TX_ENABLE_FPU_SUPPORT`                |
| Stack overflow check            | built-in          | `configCHECK_FOR_STACK_OVERFLOW = 2` (fill+check) + `vApplicationStackOverflowHook` | `TX_ENABLE_STACK_CHECKING`              |
| Null pointer parameter check    | built-in          | `configASSERT` with panic hook                | minimal (via `_tx_*` internals)          |
| Full parameter validation       | off               | off (default)                                 | off (`TX_DISABLE_ERROR_CHECKING` in port) |

All three compile with the same FPU flag (`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`) inherited from
the shared TaktOS `.cproject` template. The three FPU-context-save
mechanisms look different on paper but give each RTOS a functionally
equivalent "FPU-task allowed" capability. Cortex-M4F / Cortex-M7 hardware
lazy-stacks S0–S15 on exception entry regardless of RTOS, so per-task FPU
use has the same hardware cost in all three.

Full parameter validation is deliberately off in all three. TaktOS does
null pointer checks only (not exhaustive parameter validation); FreeRTOS's
`configASSERT` catches similar error conditions; ThreadX routes API calls
through `_tx_*` entry points (via `TX_DISABLE_ERROR_CHECKING` defined in
`tm_port_threadx.c`) which skip the `_txe_*` full-validation wrappers. If
`TX_DISABLE_ERROR_CHECKING` were removed, ThreadX would do substantially
more per-call validation than either of the other two and the numbers
would no longer be comparable.

## Expected priority wiring

| Priority (numeric, 4-bit) | Exception / IRQ                  | Purpose                           |
|---------------------------|----------------------------------|-----------------------------------|
| 0x00 (highest)            | SVC (FreeRTOS / TaktOS entry)    | Kernel entry                      |
| 0x80                      | TIM7_IRQn (SW IRQ)               | `tm_cause_interrupt()`            |
| 0xF0 (lowest)             | PendSV, SysTick                  | Deferred context switch + tick    |

SW IRQ numerically beats PendSV/SysTick so the ISR runs promptly; PendSV
tail-chains afterwards to do scheduling work.
