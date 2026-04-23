# Thread-Metric benchmarks — B-L475E-IOT01A2 (TaktOS, FreeRTOS, ThreadX)

**21 Eclipse projects** running the official Eclipse ThreadX [Thread-Metric][tm]
benchmark suite on a **B-L475E-IOT01A2 IoT Discovery Node** (STM32L475VG,
Cortex-M4F @ 80 MHz, 1 MB flash, 96 KB SRAM1). Three RTOSes run side-by-side
under identical clock / memory / UART configuration so the numbers are
directly comparable.

| RTOS      | Projects | Kernel source                                                       |
|-----------|---------:|---------------------------------------------------------------------|
| TaktOS    |        7 | `TaktOS_M4` static library from `ARM/cm4/Eclipse/{DebugFPU,ReleaseFPU}` |
| FreeRTOS  |        7 | `IOCOMPOSER_HOME/external/FreeRTOS-Kernel` (ARM_CM4F port)          |
| ThreadX   |        7 | `IOCOMPOSER_HOME/external/threadx` (cortex_m4/gnu port)             |

All 21 projects share one `src/` folder with startup, clock config, linker
script, and UART console. Each RTOS contributes only its own `tm_port_*`
file plus its kernel configuration header. Vector-table arbitration:

| Vector              | TaktOS owner              | FreeRTOS owner             | ThreadX owner                     |
|---------------------|---------------------------|----------------------------|-----------------------------------|
| `PendSV_Handler`    | `libTaktOS_M4.a`          | `port.c` (CM4F)            | `tx_thread_schedule.S`            |
| `SVC_Handler`       | `libTaktOS_M4.a`          | `port.c` (CM4F)            | `threadx_initialize_low_level.S`  |
| `SysTick_Handler`   | `libTaktOS_M4.a`          | `port.c` (CM4F)            | `threadx_initialize_low_level.S`  |
| `TIM7_IRQHandler`   | `tm_port_taktos.cpp`      | `tm_port_freertos.c`       | `tm_port_threadx.c`               |

`startup_stm32l475vg.S` declares all four as weak aliases of `Default_Handler`;
strong symbols from each RTOS's port code win at link time.

[tm]: https://github.com/eclipse-threadx/threadx/tree/master/utility/benchmark_application/thread_metric

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

## Layout

```
STM32L475VG/
├── README.md                                (this file)
├── src/                                     (shared across all 7 projects)
│   ├── startup_stm32l475vg.S                IOsonata-style startup + vector table
│   ├── system_stm32l475vg.c                 HSI16→PLL→80 MHz, 4 WS flash, FPU enable
│   ├── gcc_stm32l475vg.ld                   linker script
│   ├── tm_console_stm32l475vg.cpp         USART1 on PB6/PB7 (ST-LINK VCP)  
│   └── tm_port_taktos.cpp                   Thread-Metric -> TaktOS shim (M4F)
└── ThreadMetricBenchmarkTaktOS_<test>_STM32L475VG/
    ├── src/main.cpp
    └── Eclipse/
        ├── .project
        ├── .cproject
        ├── .gitignore
        └── .settings/language.settings.xml
```

Seven test variants, one project each:
`BasicProcessing`, `CooperativeScheduling`, `PreemptiveScheduling`,
`MessageProcessing`, `SynchronizationProcessing`, `MutexProcessing`,
`MutexBargingTest`.

## How the build resolves files

The Eclipse `.project` uses `PARENT-n-PROJECT_LOC` links:

| Link                                 | Resolves to                                        |
|--------------------------------------|----------------------------------------------------|
| `PARENT-1-PROJECT_LOC/src/main.cpp`  | `<proj>/src/main.cpp`                              |
| `PARENT-2-PROJECT_LOC/src/<file>`    | `STM32L475VG/src/<file>`                         |
| `PARENT-3-PROJECT_LOC/src/<test>.c`  | `ThreadMetric/src/<test>.c`                        |
| `PARENT-3-PROJECT_LOC/include/...`   | `ThreadMetric/include/tm_api.h`                    |

TaktOS kernel is linked from `ARM/cm4/Eclipse/DebugFPU/libTaktOS_M4.a`.

## Hardware notes (B-L475E-IOT01A2, UM2153)

| Item          | Setting                                              |
|---------------|------------------------------------------------------|
| MCU           | STM32L475VGU6 (Cortex-M4F, 80 MHz)                   |
| Clock source  | HSI16 (16 MHz internal RC)                           |
| PLL           | /M=1, ×N=10, /R=2 → 80 MHz SYSCLK                    |
| Voltage range | VCORE Range 1 (required above 26 MHz)                |
| Flash         | 4 WS, prefetch + ICache + DCache enabled             |
| UART VCP      | USART1, PB6 TX (AF7), PB7 RX (AF7)  , 115200 8N1      |
| SW IRQ        | TIM7_IRQn (55) — pended only, TIM7 never clocked     |

## Building

Eclipse workspace: import each `ThreadMetricBenchmarkTaktOS_<test>_STM32L475VG/`
directory as an existing project. The `.project` links pick up the shared
`STM32L475VG/src/` files automatically.

Command-line build (Debug config):
```
cd ThreadMetricBenchmarkTaktOS_BasicProcessing_STM32L475VG/Eclipse
make -f ../Debug/makefile all    # after Eclipse has generated the makefile once
```

## Flashing

Use ST-LINK (on-board) via `openocd -f board/st_b_l475e_iot01a.cfg`, or
STM32CubeProgrammer. ELF is produced at `Debug/<ProjName>.elf`.

## Console

Open the ST-LINK VCP at **115200 8N1** — results print every 30 s during the
benchmark window.
