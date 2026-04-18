# TaktOS Thread-Metric Benchmark Suite

Thread-Metric benchmark projects for TaktOS, FreeRTOS, and ThreadX on
nRF52832 PCA10040 (Cortex-M4F, 64 MHz).

## Test coverage

ThreadMetric test numbers follow the official eclipse-threadx/threadx suite:

| Test | ID | TaktOS | FreeRTOS | ThreadX |
|---|---|---|---|---|
| Basic Processing (calibration) | TM1 | ✓ | ✓ | ✓ |
| Cooperative Scheduling | TM2 | ✓ | ✓ | ✓ |
| Preemptive Scheduling | TM3 | ✓ | ✓ | ✓ |
| Interrupt Processing | TM4 | N/A | ✓ | ✓ |
| Memory Allocation | TM5 | N/A | ✓ | ✓ |
| Message Processing | TM6 | ✓ | ✓ | ✓ |
| Synchronization Processing | TM7 | ✓ | ✓ | ✓ |

**TM4 (Interrupt Processing)** — N/A for TaktOS. TaktOS does not own
application interrupt vectors; the kernel installs only `PendSV_Handler`,
`SVC_Handler`, and `SysTick_Handler`. All application IRQ vectors are owned
and installed by the application. TM4 requires the benchmark harness to own a
hardware timer IRQ, which conflicts with this design principle.

**TM5 (Memory Allocation)** — N/A for TaktOS. TaktOS has no heap allocator.
All memory (TCBs, stacks, queues, semaphores) is statically declared by the
application. Dynamic allocation is an architectural non-goal.

## Measured results — nRF52832 PCA10040 @ 64 MHz

**Platform:** Nordic nRF52832 PCA10040 · Cortex-M4F · 64 MHz ·
arm-none-eabi-gcc 15.2.1 · `-Os` · 1 kHz tick · Release build

Values are 30-second steady-state iteration counts. Higher = better.

| Test | TaktOS | FreeRTOS | ThreadX | T / FR | T / TX |
|---|---|---|---|---|---|
| TM1  Basic Processing | 116,891 | 116,891 | 124,701 | 1.00× | 0.94× |
| TM2  Cooperative Scheduling | 16,297,229 | 8,602,134 ✗ | 10,660,763 | **1.90×** | **1.53×** |
| TM3  Preemptive Scheduling | 4,848,786 | 2,372,066 | 4,396,380 | **2.04×** | **1.10×** |
| TM4  Interrupt Processing | N/A | — | — | — | — |
| TM5  Memory Allocation | N/A | — | — | — | — |
| TM6  Message Processing | 7,398,873 | 2,243,696 | 6,749,360 | **3.30×** | **1.10×** |
| TM7  Synchronization | 20,829,429 | 4,166,025 | 15,334,262 | **5.00×** | **1.36×** |
| **Geometric mean (TM2/3/6/7)** | | | | **2.83×** | **1.26×** |

✗ FreeRTOS TM2: produced repeated "counters more than 1 different from
average" determinism errors. Values from clean windows are used. TaktOS and
ThreadX produced zero determinism errors across all tests.

**TM1 note:** All three RTOSes score within 7% on single-thread compute.
No context switches occur during the TM1 measurement window; the result
reflects compiler output, not RTOS scheduling performance.

### TM2 alignment note

TM2 throughput reflects a known code-alignment artifact. A recent fast-path
optimization commit set added 4 bytes to the TM2 binary, which shifts
`PendSV_Handler` relative to the nRF52832's 128-byte I-code prefetch window.
The handler is 36 bytes (confirmed from `.map`); a boundary crossing adds one
stall cycle per fetch, compounding at 16M+ context switches per 30 s.

Fix in progress: `.balign 64` before `PendSV_Handler` in `PendSV_M4.S`.

## Hardware and port details

| Item | Value |
|---|---|
| Target | Nordic nRF52832 PCA10040 |
| Core | ARM Cortex-M4F |
| Clock | 64 MHz (no PLL, HFCLK from HFXO) |
| UART | UARTE0 EasyDMA, P0.07, 115200 baud |
| Tick source | SysTick @ 1000 Hz |
| Software IRQ | IRQ21 (SWI1/EGU1) via NVIC STIR |
| Stack per thread | 1024 bytes |

Priority mapping: Thread-Metric priorities run 1 (highest) → 31 (lowest).
TaktOS priorities run 1 (lowest) → 31 (highest). The port maps
`TaktOS priority = 32 − TM priority`.

## Development environment

The IOcomposer installer sets up Eclipse Embedded CDT, the xPack ARM and
RISC-V GCC toolchains, OpenOCD, and all IOsonata path variables in a single
step.

**macOS**
```bash
curl -fsSL https://iocomposer.io/install_ioc_macos.sh -o /tmp/install_ioc_macos.sh && bash /tmp/install_ioc_macos.sh
```

**Linux**
```bash
curl -fsSL https://iocomposer.io/install_ioc_linux.sh -o /tmp/install_ioc_linux.sh && bash /tmp/install_ioc_linux.sh
```

**Windows** — requires PowerShell running as Administrator:
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command "irm https://iocomposer.io/install_ioc_windows.ps1 | iex"
```

After installation, open Eclipse and select the pre-configured workspace at
`~/IOcomposer/workspace`. The `IOSONATA_LOC` workspace path variable is
already set; all benchmark projects will resolve their linked source folders
without any manual configuration.

## Building

Each test is a standalone Eclipse CDT managed-build project under its own
subdirectory. Import the projects into Eclipse Embedded CDT.

- Build configuration: **Release** (`-Os`)
- Toolchain: xPack GNU Arm Embedded GCC 15.2.1 (`arm-none-eabi-`)
- `iosonata_loc` workspace path variable must be set (see installer)
- FreeRTOS projects require the FreeRTOS kernel source tree placed at
  `FreeRTOS-Kernel/` under the project root (a placeholder file marks
  the location)
- ThreadX projects require the Eclipse ThreadX repository cloned to
  `~/IOcomposer/external/threadx`:
  ```bash
  git clone https://github.com/eclipse-threadx/threadx ~/IOcomposer/external/threadx
  ```

## Thread-Metric methodology notes

- **TM1 Basic Processing** is a calibration run. Its score verifies the
  test environment is valid; it is not a scheduling performance metric.
- Ratios (T / FR, T / TX) compare RTOSes on the same board with the same
  compiler and flags. Absolute iteration counts are not comparable across
  boards or clock speeds.
- The official Thread-Metric source (`tm_api.h`, `tm_*.c`) is unmodified.
  Only `tm_port.cpp` and `main.cpp` are RTOS-specific.
- Beningo 2024 used a STM32L4 IoT Discovery Node (B-L475E-IOT01A) at
  80 MHz, GCC 12.3. The nRF52832 measurements here use the same test suite
  and methodology; ratios vs FreeRTOS on the same board are valid comparisons.
