# Thread-Metric — TaktOS on STM32F0308-DISCO

Cortex-M0 @ 48 MHz, no FPU, soft-float ABI. Bare-metal port, self-contained
startup and USART driver — IOsonata has no STM32F0 port, so this benchmark
set supplies its own vector table, clock init, linker script, and console.

## Tests included

Seven TaktOS projects matching the nRF52832 layout:

- `ThreadMetricBenchmarkTaktOS_BasicProcessing_STM32F0308`
- `ThreadMetricBenchmarkTaktOS_CooperativeScheduling_STM32F0308`
- `ThreadMetricBenchmarkTaktOS_PreemptiveScheduling_STM32F0308`
- `ThreadMetricBenchmarkTaktOS_MessageProcessing_STM32F0308`
- `ThreadMetricBenchmarkTaktOS_SynchronizationProcessing_STM32F0308`
- `ThreadMetricBenchmarkTaktOS_MutexProcessing_STM32F0308`
- `ThreadMetricBenchmarkTaktOS_MutexBargingTest_STM32F0308`

`MemoryAllocation` (TM5) is omitted by design — TaktOS has no heap and this
port intentionally returns `TM_ERROR` from `tm_memory_pool_*`.

## Hardware

| Board          | STM32F0308-DISCO (MB1099)                        |
|----------------|--------------------------------------------------|
| MCU            | STM32F030R8T6 — Cortex-M0 @ 48 MHz               |
| Flash          | 64 KB                                            |
| RAM            | 8 KB                                             |
| UART           | USART1 @ 115200 8N1 on PA9 (TX) / PA10 (RX)      |
| Console access | ST-LINK/V2-1 Virtual COM Port                    |
| User LEDs      | PC9 = LD3 (green), PC8 = LD4 (blue) — not used   |

Open a terminal on the VCP at **115200 8N1** to see benchmark output.

## Eclipse project settings

All seven projects share the same managed-build settings, with two configs
(Debug and Release):

- **MCU**: `-mcpu=cortex-m0 -mthumb`
- **Float ABI**: `-mfloat-abi=soft`, FPU Type = *default* (no FPU)
- **C++ standard**: `-std=gnu++23`
- **Defined symbols**: `STM32F030x8`, `__PROGRAM_START`, `TAKT_INLINE_OPTIMIZATION`
- **Linker script**: `../../../src/gcc_stm32f030r8.ld` (self-contained, 64K/8K)
- **Libraries**: `TaktOS_M0`
- **Library search paths**: `../../../../../../ARM/cm0/Eclipse/<Debug|Release>`
- **Linker flag**: `-specs=nano.specs` (newlib-nano — TaktOS uses none of it,
  but keeps reference builds small)

## Before building

Build the `TaktOS_M0` static library first. The Eclipse project is at
`ARM/cm0/Eclipse/` — import it and build both the Debug and Release configs
so both benchmark configs can link. A `Release_Inl` config is also provided,
matching the cm4 library's layout.

The library pulls in CMSIS Core headers the same way cm4 / cm33 do, at
`${system_property:iosonata_loc}/IOsonata/ARM/CMSIS/Core/Include` or relative
`../../../../../IOsonata/ARM/CMSIS/Core/Include`. No F0-specific CMSIS device
header is required — the port uses bare register access only.

## Port-layer design (`src/tm_port_taktos.cpp`)

- **Scheduler / tick / priorities**: 1000 Hz tick via `TaktOSInit`, PendSV and
  SysTick set to `0xC0` (lowest of the two priority bits the M0 implements),
  software-IRQ set to `0x80` so it preempts tasks and PendSV tail-chains to
  complete scheduling work.
- **Core clock**: 48 MHz from the HSI (8 MHz) × 12 / 2 PLL path. Configured
  by `SystemInit()` in `system_stm32f030.c`. Change `TM_TAKTOS_CORE_CLOCK_HZ`
  in `tm_port_taktos.cpp` if you run at a different speed.
- **Software-IRQ line**: Cortex-M0 has no STIR, so `tm_cause_interrupt()`
  pends an unused peripheral IRQ via `NVIC_ISPR`. Default: TIM17_IRQn (IRQ 22).
  TIM17 is never clocked or configured — only its NVIC pending bit is touched.
  A weak `__attribute__((alias))` wires `TIM17_IRQHandler` to the port's
  `tm_irq_vector_handler`. If you need TIM17 for something else, change
  `TM_F0_SWI_IRQ_N` and the alias target in `tm_port_taktos.cpp`.

## Memory budget — why the sizes are tight

The F030R8 has only 8 KB RAM. With Cortex-M0's 256-byte stack guard-alignment,
each TaktOS thread costs ~622 B of fixed overhead (guard align + guard region
+ init frame + alignment slack) on top of its usable stack. The
`basic_processing` test additionally reserves a 4 KB workload array of its
own.

To keep all seven tests buildable with the same port, this port **remaps**
Thread-Metric thread_ids (0..31) to a compact slot array. Each benchmark
project sets its own `TM_TAKTOS_MAX_SLOTS` via a `-D` define in the
`.cproject`, sized to the test's actual simultaneous-thread count — not to
the test's worst-case thread_id.

Per-project values (from each `.cproject`):

| Test                        | MAX_SLOTS | Thread IDs used | BSS    | Flash |
|-----------------------------|-----------|-----------------|--------|-------|
| BasicProcessing             | 2         | 0, 5            | 6616 B | 5956  |
| CooperativeScheduling       | 6         | 0..5            | 5352 B | 7680  |
| PreemptiveScheduling        | 6         | 0..5            | 5352 B | 7424  |
| MessageProcessing           | 2         | 0..1            | 2656 B | 7900  |
| SynchronizationProcessing   | 2         | 0..1            | 2528 B | 6640  |
| MutexProcessing             | 4         | 0..3            | 3960 B | 8508  |
| MutexBargingTest            | 9         | 0..7 + reporter | 7496 B | 9800  |

Measured with `-mcpu=cortex-m0 -mfloat-abi=soft -O2 -specs=nano.specs` on
arm-none-eabi-gcc 13.2.1. Add the 256 B MSP reserve from the linker script
to get total RAM. Tightest fit is MutexBargingTest at 7752 B / 8192.

`TM_TAKTOS_STACK_BYTES` defaults to 64 in the port source but is also
overridable per-project via the same define mechanism. 64 B is tight — the
reporter thread's `tm_printf` path was measured to fit, but if you add
instrumentation or use a different UART driver with deeper call nesting,
raise to 96 or 128 and reduce `TM_TAKTOS_MAX_SLOTS` to compensate.

## Boot-time SysTick/clock diagnostic

The port includes a boot-time diagnostic gated by `TM_PORT_BOOT_DIAGNOSE`,
defined in each `.cproject` by default. Before `TaktOSInit` runs, the port
configures SysTick itself for a 100 ms period at 48 MHz (`LOAD = 4,799,999`),
then polls the `COUNTFLAG` bit ten times and prints one `.` per wrap.

Expected output at startup:
```
SYS:..........OK
**** Thread-Metric ... Relative Time: 30
...
```

The ten dots should print over **approximately one second** of wall-clock
time. If they don't, your core is not running at the expected 48 MHz:

| What you see                          | Diagnosis                                                         |
|---------------------------------------|-------------------------------------------------------------------|
| `SYS:..........OK` in ~1 s            | Clock is 48 MHz, SysTick counts. Good — issue is elsewhere.       |
| Dots print in much less than 1 s      | Core clock is higher than 48 MHz (overclocked or wrong LOAD).     |
| Dots print in ~6 s                    | PLL did not engage; running on HSI (8 MHz). Check `SystemInit()`. |
| No dots — just `SYS:` then hang       | SysTick is not counting. Check `CLKSOURCE` bit / PLL lock.        |
| `SYS:` never prints, benchmark output | Console is buffered; `tm_putchar` isn't flushing per-character.   |

**If the diagnostic reports ~1 s but benchmark output still appears
immediately**, the issue is not SysTick — it's that `tm_thread_sleep` is
returning without blocking. Check that your linked `TaktOS_M0` library
resolves `SysTick_Handler` to `TaktKernelTickHandler` (verify with
`arm-none-eabi-nm your.elf | grep SysTick_Handler` — the two should map to
the same address).

To remove the diagnostic once everything works, delete the
`TM_PORT_BOOT_DIAGNOSE` entry from each project's `.cproject` defines (or
remove it from the managed-build C++ compiler symbol list via Eclipse
project properties).

## Overriding the console driver

`tm_console_stm32f0308.cpp` provides strong symbols for `tm_hw_console_init()`
and `tm_putchar()` that drive USART1 directly. `tm_port_taktos.cpp` declares
weak fallbacks, so if you want to use a different UART (or route output over
SWO, or to a ring buffer for post-run capture), drop your own file into the
project's local `src/` with the two strong-symbol definitions — it will win
at link time.

## File layout

```
Benchmark/ThreadMetric/STM32F0308/
├── README.md                              — this file
├── src/                                   — board-shared BSP + port
│   ├── startup_stm32f030r8.S              — vector table + Reset_Handler
│   ├── system_stm32f030.c                 — HSI→PLL 48 MHz
│   ├── gcc_stm32f030r8.ld                 — 64K flash / 8K RAM
│   ├── tm_console_stm32f0308.cpp          — USART1 polling console
│   └── tm_port_taktos.cpp                 — TaktOS porting layer
├── ThreadMetricBenchmarkTaktOS_BasicProcessing_STM32F0308/
│   ├── src/main.cpp
│   └── Eclipse/{.project, .cproject, .settings/, .gitignore}
├── ThreadMetricBenchmarkTaktOS_CooperativeScheduling_STM32F0308/…
├── ThreadMetricBenchmarkTaktOS_PreemptiveScheduling_STM32F0308/…
├── ThreadMetricBenchmarkTaktOS_MessageProcessing_STM32F0308/…
├── ThreadMetricBenchmarkTaktOS_SynchronizationProcessing_STM32F0308/…
├── ThreadMetricBenchmarkTaktOS_MutexProcessing_STM32F0308/…
└── ThreadMetricBenchmarkTaktOS_MutexBargingTest_STM32F0308/…
```

The test source files themselves (`basic_processing.c`, `mutex_barging_test.c`,
etc.) and `tm_report.c` / `tm_api.h` live in the shared
`Benchmark/ThreadMetric/{src,include}/` trees — each Eclipse project brings
them in via `PARENT-3-PROJECT_LOC` linked resources, same as the nRF52832
and SAM4LC8C ports.
