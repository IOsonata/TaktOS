# TaktOS

**Deterministic kernel for ARM Cortex-M**  *(RISC-V RV32 port planned)*

[![IEC 61508 SIL 2 target](https://img.shields.io/badge/IEC%2061508-SIL%202%20target-blue)](docs/)
[![ISO 26262 ASIL D path](https://img.shields.io/badge/ISO%2026262-ASIL%20D%20path-blue)](docs/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green)](LICENSE)

---

## Design principles

**TaktOS does not own application interrupts.** The kernel installs three system handlers: the context switch handler, the first-task launcher, and the tick source (declared weak so the application may override it). All application IRQ vectors are owned and installed by the application. The application signals the kernel from any IRQ handler using `TaktOSSemGive()` / `sem_post()`.

**Zero dynamic memory.** The kernel never calls `malloc` or any allocator. Every object — threads (TCB + stack in one user-supplied buffer), semaphores, mutexes, queues (backing storage user-supplied) — is statically declared by the application. Memory layout is fully determined at compile time.

### Yield semantics

`TaktOSThreadYield()` is **immediate only in normal Thread mode**: not in Handler mode, interrupts enabled, and at least one peer exists at the same priority. In that case the ready ring is rotated and `PendSV` is requested immediately.

Two special cases are handled safely rather than corrupting scheduler state:

- **Called from an ISR / Handler mode:** the yield is **deferred** by setting an internal flag. The tick handler consumes that flag on the next tick and performs the same round-robin rotation with a correct `pCurrent`.
- **Called from Thread mode with `PRIMASK` already set:** the yield is also **deferred**. This avoids re-enabling interrupts inside the caller's critical section.

So the operational rule is: **immediate yield in normal Thread mode; deferred yield from ISR or while interrupts are masked.**

---

## At a glance

| | ARM Cortex-M |
|---|---|
| **Targets** | Cortex-M0/M0+, M4/M4F, M7, M33, M55 |
| **Context switch** | ~47 cycles ¹ |
| **Interrupt model** | Application owns all IRQ vectors |
| **Memory model** | Static allocation only — zero heap |
| **Scheduler core** | ~590 LOC portable C++23 |
| **Public API** | POSIX PSE51 (`pthread`, `sem_t`, `mq`, `timer`) + native C/C++ |
| **Certification target** | IEC 61508 SIL 2 → ASIL D (SEooC) |
| **Platform integration** | IOsonata Land-layer primitives |

¹ Design target from llvm-mca cycle budget. Measured performance via Thread-Metric on real hardware (nRF52832 M4, nRF54L15 M33).

**RISC-V RV32 port:** planned. The `RISCV/` directory in this repository contains early experimental work only; nothing there is functional or ready for use. No RISC-V Thread-Metric results are published. Do not treat anything under `RISCV/` as a working port.

When the RV32 port is implemented, candidate first validation targets are:

- **CV32E40P on FPGA** (Digilent Arty A7). Open-core IP from OpenHW Group, upstream OpenOCD, `riscv-none-elf-` toolchain — fully open workflow.
- **Renesas R9A02G021** (Andes N22, RV32IMAC). Requires a specific workflow: **both TaktOS and IOsonata must be built inside Renesas e² studio** for source-level debug to work. The E2 Lite probe protocol and the Andes debug-module variant on this part are not supported by mainline OpenOCD, so debug is only available through the Renesas GDB server bundled with e² studio. Integrators who do not need debug can build the application with any `riscv-none-elf-` toolchain of their choice and flash the resulting image using Renesas Flash Programmer — build-and-flash without debug.

---

## On-target benchmark results — Thread-Metric

**Suite:** eclipse-threadx/threadx Thread-Metric (MIT) — steady-state iteration counts, higher = better.  
**Build:** All RTOSes compiled with identical flags on the same MCU. ThreadX tested with source code.

### nRF54L15 · Cortex-M33 · 128 MHz · GCC 15.2.1 · `-Os` · 1 kHz tick

| Test | TaktOS | ThreadX | FreeRTOS | T / TX | T / FR |
|---|---|---|---|---|---|
| TM1  Basic Processing      |    374,404 |    374,403 |    374,303 | 1.00× | 1.00× |
| TM2  Cooperative Scheduling | 39,161,183 | 26,466,010 | 26,474,445 | **1.48×** | **1.48×** |
| TM3  Preemptive Scheduling  | 13,287,261 | 11,757,316 |  6,721,773 | **1.13×** | **1.98×** |
| TM6  Message Processing     | 27,608,741 | 19,092,528 |  6,947,836 | **1.45×** | **3.97×** |
| TM7  Synchronization        | 59,961,406 | 38,375,092 | 11,555,762 | **1.56×** | **5.19×** |
| TM8  Mutex Processing       | 19,679,521 | 10,105,427 |  7,259,902 | **1.95×** | **2.71×** |
| **Geometric mean (TM2–TM8)** | | | | **1.49×** | **2.77×** |

**TM1 note:** All three RTOSes score essentially the same on single-thread compute — no context switches occur during the TM1 window.

**Binary size — TM7 Synchronization `.text`** *(via `arm-none-eabi-size --format=berkeley` on the linked ELF; all three built with GCC 15.2.1, `-Os`, `--gc-sections`, same Thread-Metric harness)*:

| RTOS | .text bytes | vs TaktOS |
|---|---|---|
| TaktOS   |  6,494 | — |
| ThreadX  |  7,239 | +11.5% |
| FreeRTOS |  8,824 | +35.9% |

`.data` and `.bss` are not shown: both are dominated by test-harness static allocations (thread stacks, message-queue buffers, and for FreeRTOS the `configTOTAL_HEAP_SIZE` pool in `heap_4.c`). Those are per-port harness choices, not kernel properties.

---

### nRF52832 · Cortex-M4 · 64 MHz · GCC 15.2.1 · `-Os` · 1 kHz tick

FreeRTOS TM2 ⚠ — determinism error every window, value is informational only.

| Test | TaktOS | ThreadX | FreeRTOS | T / TX | T / FR |
|---|---|---|---|---|---|
| TM1  Basic Processing       |    143,765 |    124,641 |    124,608 | 1.15× | 1.15× |
| TM2  Cooperative Scheduling | 13,823,020 | 10,497,840 |  8,369,345⚠ | 1.32× | 1.65× |
| TM3  Preemptive Scheduling  |  4,793,897 |  4,354,376 |  2,380,390 | **1.10×** | **2.01×** |
| TM6  Message Processing     |  8,952,189 |  6,564,371 |  2,158,116 | **1.36×** | **4.15×** |
| TM7  Synchronization        | 20,381,897 | 14,632,422 |  3,910,892 | **1.39×** | **5.21×** |
| TM8  Mutex Processing       |  6,916,236 |  3,693,281 |  2,421,976 | **1.87×** | **2.86×** |
| **Geometric mean (TM2–TM8)** | | | | **1.39×** | **2.90×** |

**Binary size — TM7 Synchronization `.text`:**

| RTOS | .text bytes | vs TaktOS |
|---|---|---|
| TaktOS   |  6,102 | — |
| ThreadX  |  6,625 | +8.6% |
| FreeRTOS | 10,164 | +66.6% |

**PX5 on nRF52832:** Eclipse projects for PX5 exist under `Benchmark/ThreadMetric/nRF52832/` but the committed `TestResults.txt` does not contain PX5 console logs for this MCU. PX5 numbers will be added once the raw logs are captured alongside the other RTOSes in the same run, so the full table is reproducible from one file.

**TM4 and TM5 are not run.** TM4 requires a hardware timer IRQ owned by the test harness — TaktOS does not own application IRQs by design. TM5 measures dynamic memory allocation — TaktOS has no heap by design.

---

### Effect of `TAKT_INLINE_OPTIMIZATION`

`TAKT_INLINE_OPTIMIZATION` forces `TAKT_ALWAYS_INLINE` on the semaphore, mutex, and queue fast paths. Removing the define reverts those to regular function calls.

| Test | nRF52832 M4  with | without | M4 Δ | nRF54L15 M33  with | without | M33 Δ |
|---|---|---|---|---|---|---|
| TM6 Message         |  8,952,189 |  7,663,774 | −14.4% | 27,608,741 | 22,310,845 | −19.2% |
| TM7 Synchronization | 20,381,897 | 15,203,494 | −25.4% | 59,961,406 | 43,116,795 | −28.1% |

TM7 is affected more than TM6 on both boards — the entire workload is semaphore give/take. The M33 takes a larger penalty than the M4: its instruction cache eliminates flash wait states, making BL/BX call overhead proportionally more expensive. For certification builds where `TAKT_ALWAYS_INLINE` is disabled, the *without* column is the expected performance baseline.

---

## Architecture

### File map

```
TaktOS/
├── include/
│   ├── TaktOS.h              # public API + arch port function declarations
│   ├── TaktKernel.h          # private API (kernel objects only)
│   ├── TaktOSThread.h        # SAFETY BOUNDARY
│   ├── TaktOSSem.h           # SAFETY BOUNDARY — fast path always_inline
│   ├── TaktOSMutex.h         # SAFETY BOUNDARY
│   ├── TaktOSQueue.h         # SAFETY BOUNDARY — fast path always_inline
│   └── posix/                # POSIX PSE51 layer — QM
├── ARM/
│   ├── include/TaktOSCriticalSection.h  # SAFETY BOUNDARY — inline PRIMASK
│   ├── src/systick.h         # IOsonata Land-layer: SysTick MMIO primitives
│   ├── src/TaktKernelCM.cpp  # TaktOSTickInit() + TaktOSStackInit()
│   ├── cm0/PendSV_M0.S       # SAFETY BOUNDARY — M0/M0+
│   ├── cm4/PendSV_M4.S       # SAFETY BOUNDARY — M4/M4F
│   ├── cm7/PendSV_M7.S       # SAFETY BOUNDARY — M7
│   ├── cm33/PendSV_M33.S     # SAFETY BOUNDARY — M33
│   └── cm55/PendSV_M55.S     # SAFETY BOUNDARY — M55
├── RISCV/                    # EXPERIMENTAL — placeholder RV32 work, not functional
│   └── rv32/                 # skeleton files only; do not use
├── src/
│   ├── taktos.cpp            # scheduler, init
│   ├── taktos_sem.cpp        # semaphore slow paths
│   ├── taktos_mutex.cpp      # mutex slow paths
│   ├── taktos_queue.cpp      # queue slow paths
│   ├── taktos_thread.cpp     # thread lifecycle
│   └── posix/                # PSE51 implementation
├── Benchmark/Thread-Metric/  # Thread-Metric Eclipse projects (nRF52832, nRF54L15)
├── examples/                 # basic, mutex, posix, queue
└── test/unit/                # host-native Google Test, no arch dependency
```

### Arch port

Each architecture implements four C functions declared in `TaktOS.h`:

```c
void  TaktOSTickInit  (uint32_t KernClockHz, uint32_t tickHz, TaktOSTickClockSrc_t tickClockSrc);
void  TaktOSCtxSwitch (void);   // request deferred context switch
void  TaktOSStartFirst(void);   // launch first task — never returns
void *TaktOSStackInit (void *stackTop, void (*entry)(void*), void *arg);
```

### Layer model

| Layer | Modules | LOC | ISA-specific? |
|---|---|---|---|
| Land | `TaktOSCriticalSection.h`, `systick.h` | ~80 per arch | Arch files only |
| Roots | scheduler, semaphore, mutex, queue, task | ~760 portable C++23 | None |
| Arch port | `ARM/cm*/PendSV_*.S`, `TaktKernelCM.cpp` | ~200–300 | ARM only (RISC-V planned) |
| Fruit | POSIX PSE51 (pthread, sem_t, mqueue, timer) | ~1,800 | None |

**Safety boundary total: ~1,454 LOC** (ARM + portable C++23).

---

## Certification strategy

TaktOS is supplied as an **IEC 61508 Safety Element out of Context (SEooC)** with supporting evidence artifacts. Assessment and certification of the integrated product are performed by the integrator's own assessor during product safety qualification — I-SYST does not pay for or represent that TaktOS is itself certified.

Evidence artifacts provided with TaktOS are intended to support the integrator's safety case:

- MC/DC coverage reports (portable kernel modules, run on x86 host)
- Branch/line coverage reports for per-variant assembly
- Requirements traceability between engineering specification, source, and tests
- Unit test suite (host-native, Google Test, no arch dependency)
- FMEA worksheets and development process documentation

The small safety boundary (~1,500 LOC) is designed to keep the integrator's MC/DC tractability burden within reach. Static allocation, zero heap, application-owned IRQs, and a single critical-section mechanism are the design choices that make the boundary small — they are correct by design, not trimmed for cost.

---

## Development environment setup

The fastest way to get a working embedded toolchain is **IOcomposer** —
an AI-assisted IDE for embedded development. One script installs the
complete environment: IDE, GCC ARM toolchain, OpenOCD, and
SDK paths. Typical setup time: ~15 minutes.

See [iocomposer.io](https://iocomposer.io) for a 3-minute demo and full documentation.

**macOS**
```sh
curl -fsSL https://iocomposer.io/install_ioc_macos.sh -o /tmp/install_ioc_macos.sh && bash /tmp/install_ioc_macos.sh
```

**Linux**
```sh
curl -fsSL https://iocomposer.io/install_ioc_linux.sh -o /tmp/install_ioc_linux.sh && bash /tmp/install_ioc_linux.sh
```

**Windows (PowerShell as Admin)**
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command "irm https://iocomposer.io/install_ioc_windows.ps1 | iex"
```

After installing, open IOcomposer and load a TaktOS project:
**File → Open Projects from File System…** → browse to
`TaktOS/ARM/cm4/Eclipse/` (or the relevant arch folder).

### Manual toolchain install

If you prefer to manage toolchains yourself:

- **ARM:** [xPack GNU Arm Embedded GCC](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack)
- **Debug:** [xPack OpenOCD](https://github.com/xpack-dev-tools/openocd-xpack) or SEGGER J-Link

---

## Build

### Toolchains

| Target | Prefix | ISA flags |
|---|---|---|
| ARM Cortex-M0/M0+ | `arm-none-eabi-` | `-mcpu=cortex-m0plus` |
| ARM Cortex-M4/M4F | `arm-none-eabi-` | `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M7 | `arm-none-eabi-` | `-mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M33 | `arm-none-eabi-` | `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M55 | `arm-none-eabi-` | `-mcpu=cortex-m55 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |

All targets: `-std=gnu++23 -fno-exceptions -fno-rtti -Os`

### Configuration

TaktOS ships as a precompiled static library per architecture variant.
There is no user config header. All kernel parameters are passed at runtime:

```c
TaktOSInit(64000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);   // tick input Hz, tick rate Hz, tick clock source, handler base
```

Stack overflow detection (paint+check guard word) is always active.
MPU/PMP guard regions are library build options, not application defines.

---

## Product family

| Product | Role |
|---|---|
| **TaktOS** | Deterministic kernel — bare-metal RTOS (this repo) |
| **IOsonata** | Driver/interface framework (`DevIntrf_t` bus injection) |
| **BlueSonata** | Bluetooth connectivity layer |
| **IOcomposer** | System orchestration |

IOsonata architecture (the Land/Roots/Trees/Fruit orchard metaphor and `DevIntrf_t` driver model that TaktOS builds on) is documented in *Beyond Blinky* by Nguyen Hoan Hoang.

---

## Status

- [x] ARM Cortex-M0/M0+ port
- [x] ARM Cortex-M4/M4F port — Thread-Metric validated on nRF52832
- [x] ARM Cortex-M7 port — functional, no Thread-Metric run yet
- [x] ARM Cortex-M33 port — Thread-Metric validated on nRF54L15
- [x] ARM Cortex-M55 port — functional, no Thread-Metric run yet
- [x] POSIX PSE51 layer (pthread, sem, mqueue, timer)
- [x] Thread-Metric TM1/TM2/TM3/TM6/TM7/TM8 — TaktOS, FreeRTOS, ThreadX on nRF52832 and nRF54L15
- [ ] PX5 on nRF52832 — Eclipse projects built; raw console logs pending capture into `TestResults.txt`
- [ ] RISC-V RV32IMAC port — **planned, not implemented.** `RISCV/` directory contains experimental placeholder only.
- [ ] MC/DC coverage run (`test/unit/`)
- [ ] IEC 61508 SIL 2 certification campaign

TM4 and TM5 are not planned: TM4 requires kernel-owned IRQs (TaktOS does not have them by design), TM5 requires dynamic allocation (TaktOS does not have it by design).

---

*TaktOS · Rev 3.3 · April 2026 · I-SYST inc.*
