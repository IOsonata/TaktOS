# TaktOS Thread-Metric Benchmark Suite

Thread-Metric benchmark projects for TaktOS, FreeRTOS, ThreadX, and Zephyr on
ARM Cortex-M targets. KVB is the primary measurement framework for the
TaktOS project (see `KVB/README.md` and `docs/TaktOS_Benchmark_Report.docx`
Rev 3.5); Thread-Metric is reported alongside it as the de-facto industry
cross-RTOS throughput benchmark.

## Test coverage

Test numbers follow the official `eclipse-threadx/threadx` Thread-Metric
suite. TM9 (Mutex Barging) is a TaktOS addition not in upstream.

| Test | ID | TaktOS | FreeRTOS | ThreadX | Zephyr |
|---|---|---|---|---|---|
| Basic Processing (calibration) | TM1 | yes | yes | yes | yes |
| Cooperative Scheduling | TM2 | yes | yes | yes | yes |
| Preemptive Scheduling | TM3 | yes | yes | yes | yes |
| Interrupt Processing | TM4 | N/A | not run | not run | not run |
| Memory Allocation | TM5 | N/A | not run | not run | not run |
| Message Processing | TM6 | yes | yes | yes | yes |
| Synchronization Processing | TM7 | yes | yes | yes | yes |
| Mutex Processing | TM8 | yes | yes | yes | yes |
| Mutex Barging (TaktOS-added) | TM9 | yes | yes | yes | yes |

**TM4 (Interrupt Processing)** - N/A for TaktOS. TaktOS does not own
application interrupt vectors; the kernel installs only `PendSV_Handler`,
`SVC_Handler`, and `SysTick_Handler` on ARM (and the machine-trap dispatch
on RV32). TM4 requires the benchmark harness to own a hardware timer IRQ,
which conflicts with this design principle. Not run on the other kernels
either in this suite to keep workloads matched.

**TM5 (Memory Allocation)** - N/A for TaktOS. TaktOS has no heap allocator.
All memory (TCBs, stacks, queues, semaphores) is statically declared by the
application. Not run on the other kernels in this suite.

## Toolchain pinning

Every kernel in the comparison is built with the same GCC major.minor so
the comparison is not biased by compiler version:

| Kernel | Toolchain | Notes |
|---|---|---|
| TaktOS | xPack `arm-none-eabi-gcc 12.2.1` | Same toolchain across KVB and Thread-Metric. |
| FreeRTOS | xPack `arm-none-eabi-gcc 12.2.1` | Same as TaktOS. |
| ThreadX | xPack `arm-none-eabi-gcc 12.2.1` | Same as TaktOS. |
| Zephyr | Zephyr SDK / NCS v3.3.0 - `gcc 12.2.0` | Same major.minor; the `.x` point release is supplied by NCS and cannot be substituted without diverging from a supported Zephyr build. |

The earlier Rev 3.1 dataset used GCC 15.2.1 on TaktOS / FreeRTOS / ThreadX
while Zephyr was on its SDK toolchain. That toolchain mismatch is removed
in the current numbers - all four kernels run on the same GCC 12.2.x family.
The Rev 3.1 GCC 15.2.1 / three-way Thread-Metric data is formally retired.

## Measured results - nRF54L15 * Cortex-M33 * 128 MHz

**Platform:** Nordic nRF54L15-DK * Cortex-M33 @ 128 MHz * 1 kHz tick * `-Os` Release build

Steady-state windows. Higher is better.

| Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T / FR | T / TX | T / Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| TM1 Basic Processing | 374,397 | 374,335 | 374,394 | 373,427 | 1.00x | 1.00x | 1.00x |
| TM2 Cooperative Sched. | 35,209,422 | 21,557,088 (warn) | 16,541,318 | 13,996,145 | **1.63x** | **2.13x** | **2.52x** |
| TM3 Preemptive Sched. | 13,944,360 | 6,311,100 | 7,912,474 | 8,735,995 | **2.21x** | **1.76x** | **1.60x** |
| TM6 Message Processing | 22,979,552 | 7,251,900 | 19,092,261 | 7,308,438 | **3.17x** | **1.20x** | **3.14x** |
| TM7 Synchronization | 49,838,602 | 12,142,194 | 34,263,828 | 17,619,784 | **4.10x** | **1.45x** | **2.83x** |
| TM8 Mutex Processing | 19,406,089 | 7,075,918 | 7,539,185 | 6,701,746 | **2.74x** | **2.57x** | **2.90x** |
| TM9 Mutex Barging (total) | 19,284,580 | 6,866,504 | 7,373,447 | 6,982,184 | **2.81x** | **2.62x** | **2.76x** |
| TM9 Barge fraction | 0.003 | 0.017 | 0.016 | 0.008 | - | - | - |

## Measured results - nRF52832 * Cortex-M4F * 64 MHz

**Platform:** Nordic nRF52832 PCA10040 * Cortex-M4F @ 64 MHz * 1 kHz tick * `-Os` Release build

| Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T / FR | T / TX | T / Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| TM1 Basic Processing | 124,626 | 133,534 | 124,634 | 163,500 | 0.93x | 1.00x | 0.76x |
| TM2 Cooperative Sched. | 14,172,449 | 8,114,857 (warn) | 6,008,402 | 4,980,000 | **1.75x** | **2.36x** | **2.85x** |
| TM3 Preemptive Sched. | 5,133,419 | 2,255,396 | 2,807,117 | 3,154,000 | **2.28x** | **1.83x** | **1.63x** |
| TM6 Message Processing | 7,481,555 | 2,351,340 | 6,027,274 | 2,747,000 | **3.18x** | **1.24x** | **2.72x** |
| TM7 Synchronization | 16,100,253 | 4,355,732 | 11,341,405 | 6,263,000 | **3.70x** | **1.42x** | **2.57x** |
| TM8 Mutex Processing | 6,891,808 | 2,603,737 | 2,691,913 | 2,432,000 | **2.65x** | **2.56x** | **2.83x** |
| TM9 Mutex Barging (total) | 6,800,973 | 2,318,856 | 2,522,960 | 2,405,000 | **2.93x** | **2.70x** | **2.83x** |
| TM9 Barge fraction | 0.008 | 0.051 | 0.047 | 0.024 | - | - | - |

### Notes

**TM1.** All four kernels converge on the same iteration count on M33
because the test isolates loop-body cost only. On M4, Zephyr's higher
TM1 reflects loop-body code generation, not scheduling - TM1 does not
exercise the scheduler. TaktOS leads every test that does.

**TM2 (warn).** Both boards report `Invalid counter value(s).
Cooperative counters should not be more than 1 different than the
average` on the FreeRTOS Cooperative Scheduling test in nearly every
30-second window. Counts are reported as captured. Same warning shape as
in the Rev 3.1 dataset.

**TM9 Mutex Barging** is a TaktOS-added test (not in upstream
Thread-Metric). It puts 4 worker threads at one priority against 4
high-priority interlopers, all contending on a priority-protected
mutex. The barge fraction = `interlopers' ops / total ops` measures
whether the kernel correctly hands the mutex to the high-priority
interloper or whether barging cuts in. Lower is better. TaktOS's PCP
path holds the barge fraction at or below 0.008 on both boards; PI-based
kernels run higher.

## Hardware and port details

| Item | nRF54L15-DK | nRF52832 PCA10040 |
|---|---|---|
| Core | ARM Cortex-M33 (FPU, DWT) | ARM Cortex-M4F (FPU, DWT) |
| Clock | 128 MHz | 64 MHz |
| Tick source | SysTick @ 1000 Hz | SysTick @ 1000 Hz |
| Software IRQ | EGU1 / SWI1 via NVIC STIR | EGU1 / SWI1 via NVIC STIR |
| Stack per thread | 1024 bytes | 1024 bytes |

Priority mapping: Thread-Metric priorities run 1 (highest) to 31 (lowest).
TaktOS priorities run 1 (lowest) to 31 (highest). The port maps
`TaktOS priority = 32 - TM priority`.

## Other targets

Project trees exist under `Benchmark/ThreadMetric/` for additional MCUs
that have not yet been published in the headline tables:

- `STM32F0308/` - Cortex-M0 @ 48 MHz - KVB results are published in the
  benchmark report; Thread-Metric run is open work.
- `STM32L432KC/`, `STM32L475VG/`, `STM32H753ZI/`, `STM32G474/` - full
  TaktOS / FreeRTOS / ThreadX Eclipse project sets generated; runs not
  yet captured.
- `SAM4LC8C/` - M4 soft-float port; runs not yet captured.
- `ArduinoNanoR4/` - Renesas RA4M1 M4; UART validation pending FTDI
  adapter.
- `ESP32C/`, `ESP32C3_RAM/` - RV32 experimental; the standard toolchain
  pipeline does not cover ESP32-C3 cleanly. Not on the v1.x ARM-headline
  roadmap.

The PX5 demo is excluded by methodology: its build cannot be configured
the same way TaktOS, FreeRTOS, ThreadX, and Zephyr are tuned (optimisation
level, inlining, kernel options), so a run against them would violate
the like-for-like parity used throughout this work. Published PX5
numbers (Beningo, 2024) are referenced in the engineering benchmark
report where relevant, but are not re-run or presented as our
measurement.

## Zephyr port

Zephyr is a build-config-driven RTOS, not a per-MCU project tree like
the Eclipse-based RTOSes. There is one west project per benchmark test;
the target MCU is selected at build time via `west build -b <board>`.

```
Benchmark/ThreadMetric/Zephyr/
+-- common/
|   +-- main.c                 # shared Zephyr entry; calls tm_main()
|   +-- prj.conf               # strict-parity reference config (copied per project)
+-- Zephyr_BasicProcessing/    # one CMakeLists.txt per test
+-- Zephyr_CooperativeScheduling/
+-- Zephyr_PreemptiveScheduling/
+-- Zephyr_MessageProcessing/
+-- Zephyr_SynchronizationProcessing/
+-- Zephyr_MutexProcessing/
+-- Zephyr_MutexBargingTest/
```

The single `common/prj.conf` is the parity baseline against
FreeRTOS / TaktOS / ThreadX:

- `CONFIG_TICKLESS_KERNEL=n` and `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000`
  (other kernels are tick-driven at 1 kHz).
- `CONFIG_SIZE_OPTIMIZATIONS=y` and `CONFIG_SPEED_OPTIMIZATIONS=n`
  (match `-Os` everywhere).
- `CONFIG_STACK_SENTINEL=y` (canary parity with FreeRTOS
  `configCHECK_FOR_STACK_OVERFLOW=2`, ThreadX `TX_ENABLE_STACK_CHECKING`,
  TaktOS always-on guard-word check).
- `CONFIG_RUNTIME_ERROR_CHECKS=y`, `CONFIG_ASSERT=y`,
  `CONFIG_ASSERT_LEVEL=2` (parity with the API-entry validation regimes
  each other kernel ships - see the bench report 3.2 for the
  per-kernel breakdown).
- `CONFIG_MPU=n` / `CONFIG_ARM_MPU=n` (parity with FreeRTOS ARM_CM4F,
  ThreadX cortex_m4/gnu, and TaktOS MPU path unbound).
- `CONFIG_HEAP_MEM_POOL_SIZE=0` (all Thread-Metric storage is static).

See `Benchmark/ThreadMetric/Zephyr/README.md` for the full parity table
and west build commands.

## Development environment

The IOcomposer installer sets up Eclipse Embedded CDT, the xPack ARM and
RISC-V GCC toolchains, OpenOCD, and all IOsonata path variables in a
single step.

**macOS**
```bash
curl -fsSL https://iocomposer.io/install_ioc_macos.sh -o /tmp/install_ioc_macos.sh && bash /tmp/install_ioc_macos.sh
```

**Linux**
```bash
curl -fsSL https://iocomposer.io/install_ioc_linux.sh -o /tmp/install_ioc_linux.sh && bash /tmp/install_ioc_linux.sh
```

**Windows** (PowerShell as Administrator):
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command "irm https://iocomposer.io/install_ioc_windows.ps1 | iex"
```

After installation, open Eclipse and select the pre-configured workspace
at `~/IOcomposer/workspace`. The `IOSONATA_LOC` workspace path variable
is already set; all Eclipse-based benchmark projects will resolve their
linked source folders without any manual configuration. Zephyr west
projects are built from a separate Zephyr workspace - see the Zephyr
port README.

## Building

Each Eclipse-based test is a standalone Eclipse CDT managed-build project
under its own subdirectory. Import the projects into Eclipse Embedded CDT.

- Build configuration: **Release** (`-Os`)
- Toolchain: xPack GNU Arm Embedded GCC 12.2.1 (`arm-none-eabi-`)
- `iosonata_loc` workspace path variable must be set (see installer)
- FreeRTOS projects require the FreeRTOS kernel source tree placed at
  `FreeRTOS-Kernel/` under the project root (a placeholder file marks
  the location)
- ThreadX projects require the Eclipse ThreadX repository cloned to
  `~/IOcomposer/external/threadx`:
  ```bash
  git clone https://github.com/eclipse-threadx/threadx ~/IOcomposer/external/threadx
  ```
- Zephyr projects build via west from the Zephyr workspace (see the
  Zephyr port README for board-specific commands)

## Methodology notes

- **TM1 Basic Processing** is a calibration run. Its score verifies the
  test environment is valid; it is not a scheduling performance metric.
- Ratios (T / FR, T / TX, T / Z) compare RTOSes on the same board with
  the same compiler and flags. Absolute iteration counts are not
  comparable across boards or clock speeds.
- The official Thread-Metric source (`tm_api.h`, `tm_*.c`) is unmodified.
  Only `tm_port_*.{c,cpp}` and the boot wrapper are RTOS-specific.
- Beningo 2024 used a STM32L4 IoT Discovery Node (B-L475E-IOT01A) at
  80 MHz, GCC 12.3. The nRF measurements here use the same test suite
  and methodology; ratios vs other kernels on the same board are valid
  comparisons.
