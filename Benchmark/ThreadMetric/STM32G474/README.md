# STM32G474 Thread-Metric Bundle

NUCLEO-G474RE · STM32G474RE · Cortex-M4 + FPU · 170 MHz

Twenty-one Eclipse projects: **TaktOS, FreeRTOS, and Eclipse ThreadX** side-by-side on the same board, the same compiler, the same settings — seven Thread-Metric tests each.

## Layout

```
STM32G474/
├── README.md                                       (this file)
├── src/                                            shared bring-up + ports
│   ├── startup_stm32g474.S                         vector table, FPU enable
│   ├── gcc_stm32g474.ld                            linker script
│   ├── system_stm32g474.c                          170 MHz PLL, VOS1 Boost, 4 WS
│   ├── tm_console_nucleo_g474.cpp                  LPUART1 console (TaktOS path)
│   ├── tm_port_taktos.cpp                          TaktOS Thread-Metric adapter
│   ├── tm_port_freertos.c                          FreeRTOS Thread-Metric adapter
│   ├── tm_port_threadx.c                           ThreadX Thread-Metric adapter
│   ├── threadx_initialize_low_level.S              ThreadX low-level, 170 MHz tick
│   └── FreeRTOSConfig.h                            canonical FreeRTOSConfig
│
├── ThreadMetricBenchmarkTaktOS_<Test>_STM32G474/   7 TaktOS projects
├── ThreadMetricBenchmarkFreeRTOS_<Test>_STM32G474/ 7 FreeRTOS projects
└── ThreadMetricBenchmarkThreadX_<Test>_STM32G474/  7 ThreadX projects
```

Tests (all three kernels): `BasicProcessing`, `CooperativeScheduling`, `PreemptiveScheduling`, `MessageProcessing`, `SynchronizationProcessing`, `MutexProcessing`, `MutexBargingTest`.

## Board wiring

- MCU: STM32G474RE — Cortex-M4 + FPv4-SP-D16, 512 KB flash, 128 KB RAM
- SYSCLK: 170 MHz from HSI16 via PLL (M=4, N=85, R=2), VOS1 Boost, 4 WS flash latency
- Console: **LPUART1** on PA2 (TX) / PA3 (RX), alternate function AF12, 115200 8N1 — routed to the ST-LINK/V3E Virtual COM Port on USB
- Soft-IRQ for Thread-Metric: `TIM7_DAC_IRQn` (#55). TIM7 is never clocked; only the NVIC pending bit is pulsed via STIR.

## Workspace prerequisites

This bundle links kernel source trees through an Eclipse workspace path variable named `IOCOMPOSER_HOME`, and a system property `iocomposer_home` used in include paths. Both must be set to the root of your IOcomposer/IOsonata checkout. The expected layout inside:

```
$IOCOMPOSER_HOME/
├── external/
│   ├── FreeRTOS-Kernel/                 FreeRTOS kernel source tree
│   │   ├── list.c, queue.c, tasks.c
│   │   ├── include/
│   │   └── portable/GCC/ARM_CM4F/port.c
│   └── threadx/                         Eclipse ThreadX source tree
│       ├── common/{inc,src}/
│       └── ports/cortex_m4/gnu/{inc,src}/
```

This matches the convention used by your existing nRF52832 bundle — nothing new to set up if that one already builds.

TaktOS projects do not need any external checkout; they link `libTaktOS_M4.a` directly from `../../../../../../ARM/cm4/Eclipse/ReleaseFPU/` (hard-float), so build the TaktOS static library there first.

## Build — TaktOS

1. Build the TaktOS static library once at `ARM/cm4/Eclipse/ReleaseFPU` (and/or `DebugFPU`). The projects link `libTaktOS_M4.a` from that output directory.
2. Import any `ThreadMetricBenchmarkTaktOS_*_STM32G474` project into Eclipse.
3. Build Release or Debug. The output is an ELF in `Eclipse/Release/` or `Eclipse/Debug/`.
4. Flash via ST-LINK. Open the VCP (115200 8N1) before reset to catch the Thread-Metric 30-second-interval report lines.

## Build — FreeRTOS

1. Make sure `IOCOMPOSER_HOME` / `iocomposer_home` point at a tree with `external/FreeRTOS-Kernel/` populated.
2. Import a `ThreadMetricBenchmarkFreeRTOS_*_STM32G474` project.
3. Build. The FreeRTOS kernel source (`list.c`, `queue.c`, `tasks.c`, `portable/GCC/ARM_CM4F/port.c`) is compiled inline with the project.

`FreeRTOSConfig.h` lives in each project's own `src/` — the canonical copy is at `STM32G474/src/FreeRTOSConfig.h`, and the generator copies it into each project. Key settings: `configUSE_PREEMPTION=1`, `configTICK_RATE_HZ=1000`, `configCPU_CLOCK_HZ=170000000`, `configPRIO_BITS=4`, `configMAX_PRIORITIES=32`, assertions disabled.

## Build — ThreadX

1. Make sure `IOCOMPOSER_HOME` / `iocomposer_home` point at a tree with `external/threadx/` populated.
2. Import a `ThreadMetricBenchmarkThreadX_*_STM32G474` project.
3. Build. The full ThreadX common tree (~150 `.c` files) and the Cortex-M4/GNU port (`.S` files) link via `IOCOMPOSER_HOME` resource URIs.

The project also links the board-specific `threadx_initialize_low_level.S` from `STM32G474/src/`, which sets `SYSTEM_CLOCK = 170000000` so ThreadX's SysTick reload math matches the 170 MHz clock. `tx_user.h` (from `ThreadMetric/include/`) carries the `TX_DISABLE_ERROR_CHECKING` configuration already used on the other boards.

## Benchmark methodology

- **Same board**, NUCLEO-G474RE, 170 MHz HSI16/PLL, hard-float, `-O2` Release.
- **Same compiler**, whatever `arm-none-eabi-gcc` is on your path. Tested under xPack GCC 15.
- **Assertions / argument checks disabled** in all three kernels, matching production-typical settings.
- **Per-line UART output** on LPUART1 at 115200 8N1 so every test's report reaches the VCP even across preemption boundaries.
- **Thread-Metric harness** (`ThreadMetric/src/*`) is shared — all three kernels run the identical test source files byte-for-byte.
- **Beningo 2024 reference**: the 2024 RTOS Performance Report tested STM32L4 (B-L475E-IOT01A) at 80 MHz, not G474. At 170 MHz, G474 counts will be roughly 2× higher. If you want strict apples-to-apples against Beningo's published numbers, change `system_stm32g474.c` to run at 80 MHz: PLLM=4, PLLN=40, PLLR=2, flash latency=2 WS, skip the Boost step. The rest of the bundle runs unchanged.
- **PX5 is not included** — per project policy, PX5 cannot be tuned to apples-to-apples settings from its demo package, so publishing a measured PX5 number would not be fair. The Beningo 2024 figures for PX5 remain the reference when that comparison is needed.

## Test slot counts (TaktOS)

TaktOS uses per-project slot allocation, set at build time via `TM_TAKTOS_MAX_SLOTS`:

| Test | Slots |
|---|---|
| BasicProcessing | 2 |
| CooperativeScheduling | 6 |
| PreemptiveScheduling | 6 |
| MessageProcessing | 2 |
| SynchronizationProcessing | 2 |
| MutexProcessing | 4 |
| MutexBargingTest | 9 |

FreeRTOS (`TM_FREERTOS_MAX_THREADS=12`) and ThreadX (`TM_THREADX_MAX_THREADS=15`) size their pools up-front in the port files.

---

Copyright (c) 2026 I-SYST Inc. TaktOS is released under the MIT License. FreeRTOS is a trademark of Amazon Web Services, Inc. Eclipse ThreadX is a trademark of the Eclipse Foundation.
