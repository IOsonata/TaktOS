# TaktOS

**Deterministic kernel for ARM Cortex-M (RISC-V RV32 in bring-up)**

[![IEC 61508 SIL 2 target](https://img.shields.io/badge/IEC%2061508-SIL%202%20target-blue)](docs/)
[![ISO 26262 ASIL D path](https://img.shields.io/badge/ISO%2026262-ASIL%20D%20path-blue)](docs/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green)](LICENSE)

---

## Design principles

**TaktOS does not own application interrupts.** The kernel installs three system handlers: the context-switch handler, the first-task launcher, and the tick source (declared weak so the application may override it). All application IRQ vectors are owned and installed by the application. The application signals the kernel from any IRQ handler using `TaktOSSemGive()` / `sem_post()`.

**Zero dynamic memory.** The kernel never calls `malloc` or any allocator. Every object - threads (TCB + stack in one user-supplied buffer), semaphores, mutexes, queues (backing storage user-supplied) - is statically declared by the application. Memory layout is fully determined at compile time.

### Yield semantics

`TaktOSThreadYield()` is **immediate only in normal Thread mode**: not in Handler mode, interrupts enabled, and at least one peer exists at the same priority. In that case the ready ring is rotated and `PendSV` is requested immediately.

Two special cases are handled safely rather than corrupting scheduler state:

- **Called from an ISR / Handler mode:** the yield is **deferred** by setting an internal flag. The tick handler consumes that flag on the next tick and performs the same round-robin rotation with a correct `pCurrent`.
- **Called from Thread mode with `PRIMASK` already set:** the yield is also **deferred**. This avoids re-enabling interrupts inside the caller's critical section.

So the operational rule is: **immediate yield in normal Thread mode; deferred yield from ISR or while interrupts are masked.**

---

## At a glance

| | ARM Cortex-M | RISC-V RV32 |
|---|---|---|
| **Variants** | M0/M0+, M4/M4F, M7, M33, M55 | RV32IMAC (rv32_ilp32) |
| **Hardware validation** | nRF52832 (M4F), nRF54L15 (M33), STM32F0308 (M0) | Compile-tested; on-target bring-up in progress |
| **Context switch** | ~47 cycles ^1 | structural frame + scheduler body ^2 |
| **Interrupt model** | Application owns all IRQ vectors | Application owns all IRQs / traps |
| **Memory model** | Static allocation only - zero heap | Same |
| **Scheduler core** | ~590 LOC portable C++23 | Same portable core |
| **Public API** | POSIX PSE51 (`pthread`, `sem_t`, `mq`, `timer`) + native C/C++ | Same |
| **Certification target** | IEC 61508 SIL 2 -> ASIL D (SEooC) | Same path; ARM is the lead variant |

^1 Design target from llvm-mca cycle budget. Measured throughput via KVB and Thread-Metric on real hardware (nRF52832 M4F, nRF54L15 M33, STM32F0308 M0).

^2 RV32 has no hardware exception frame push, so save/restore of x1-x31 + mepc + mstatus is a structural cost shared by all RTOSes on RV32. See `RISCV/README.md`.

**RISC-V RV32 status (May 2026):** The `RISCV/rv32/` arch lib (`libTaktOS_RV32.a`) is the first deliverable RISC-V port. It builds for `rv32imac_zicsr / ilp32` and ships weak CLINT-based defaults for `TaktOSTickInit` and `TaktHalTrapDispatch` in `RISCV/rv32/src/TaktKernelRiscv.cpp`. Chips outside the standard CLINT layout (ESP32-C3 in particular) override both functions with strong defs. The port is compile-tested and the file structure mirrors the ARM ports; on-target validation (KVB / Thread-Metric on RV32 silicon) is pending. The legacy `RISCV/esp32c3/` tree is design-sketch only and is not built. Treat ARM as the production target and RV32 as bring-up.

---

## On-target benchmark results

Two benchmark suites are run on real hardware. **KVB** (Kernel Validation Benchmark) is the primary measurement framework - it is validation-gated, so a throughput number is publishable only when the corresponding behaviour test also passes. **Thread-Metric** is the de-facto cross-RTOS throughput benchmark and is reported alongside KVB. Every kernel in the comparison is built with the same compiler toolchain (xPack `arm-none-eabi-gcc 12.2.1` for TaktOS/FreeRTOS/ThreadX; Zephyr SDK / NCS v3.3.0 `gcc 12.2.0` - same major.minor) so the comparison is not biased by compiler version.

The full report is `docs/TaktOS_Benchmark_Report.docx` (Rev 3.5, May 2026). The tables below are the headline numbers.

### KVB - nRF54L15 * Cortex-M33 * 128 MHz * GCC 12.2.x * `-Os` * 1 kHz tick

10-second steady-state window. Higher = better.

| KVB Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| SCHED_COOP_001 (yields/10 s) | 10,658,299 | 6,730,284 | 4,034,399 | 4,608,098 | **1.58x** | **2.64x** | **2.31x** |
| SYNC_SEM_FAST_001 (p/s) | 1,468,992 | 390,280 | 1,085,516 | 526,438 | **3.76x** | **1.35x** | **2.79x** |
| SYNC_MUTEX_FAST_001 (p/s) | 1,323,191 | 269,394 | 460,008 | 424,115 | **4.91x** | **2.88x** | **3.12x** |
| SYNC_MUTEX_PCP_FAST_001 (p/s) | 559,345 | 265,790 | 459,687 | 424,115 | **2.10x** | **1.22x** | **1.32x** |
| IPC_QUEUE_FAST_001 (p/s) | 342,282 | 171,693 | 303,617 | 153,958 | **1.99x** | **1.13x** | **2.22x** |
| SYNC_MUTEX_OWNERSHIP_001 | PASS | PASS | PASS | PASS | parity | parity | parity |
| TIME_SLEEP_001 (elapsed us) | PASS @10399 | PASS @9479 | PASS @9467 | PASS @11000 | - | - | - |

`SYNC_MUTEX_PCP_FAST_001` measures TaktOS's Priority Ceiling Protocol path against the other three kernels' Priority Inheritance paths. Both protocols boost holder priority; the bookkeeping shapes differ. The comparison is reported because it is the closest functional pair across kernels, not because the protocols are equivalent.

**Binary size, linked KVB suite ELF** (`arm-none-eabi-size --format=berkeley`):

| Kernel | .text bytes | vs TaktOS |
|---|---:|---|
| TaktOS | 22,111 | - |
| FreeRTOS | 22,363 | +1.1% |
| ThreadX | 26,327 | +19.1% |
| Zephyr | 58,428 (FLASH) | see note |

Zephyr's number is FLASH region usage from its build system; it includes the data segment and a larger startup harness, so it is not byte-for-byte comparable to the other three. The order-of-magnitude difference is informative - Zephyr is the only kernel here that does not fit under 30 KB on the same-feature suite.

### KVB - nRF52832 * Cortex-M4F * 64 MHz * GCC 12.2.x * `-Os` * 1 kHz tick

| KVB Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| SCHED_COOP_001 (yields/10 s) | 4,093,473 | 2,673,007 | 1,929,845 | 1,679,433 | **1.53x** | **2.12x** | **2.44x** |
| SYNC_SEM_FAST_001 (p/s) | 443,662 | 139,804 | 359,094 | 204,934 | **3.17x** | **1.24x** | **2.16x** |
| SYNC_MUTEX_FAST_001 (p/s) | 375,768 | 97,238 | 157,013 | 170,778 | **3.86x** | **2.39x** | **2.20x** |
| SYNC_MUTEX_PCP_FAST_001 (p/s) | 175,466 | 97,239 | 157,404 | 168,060 | **1.80x** | **1.11x** | **1.04x** |
| IPC_QUEUE_FAST_001 (p/s) | 110,874 | 53,586 | 100,786 | 70,166 | **2.07x** | **1.10x** | **1.58x** |
| SYNC_MUTEX_OWNERSHIP_001 | PASS | PASS | PASS | PASS | parity | parity | parity |
| TIME_SLEEP_001 (elapsed us) | PASS @10153 | PASS @9696 | PASS @9429 | PASS @11297 | - | - | - |

**Binary size, linked KVB suite ELF:**

| Kernel | .text bytes | vs TaktOS |
|---|---:|---|
| TaktOS | 22,007 | - |
| FreeRTOS | 23,287 | +5.8% |
| ThreadX | 26,339 | +19.7% |
| Zephyr | 45,692 (FLASH) | see note |

### KVB - STM32F0308-DISCO * Cortex-M0 * 48 MHz * GCC 12.2.x * `-Os` * 1 kHz tick

ARMv6-M has no DWT cycle counter, no CLZ instruction, and no LDREX/STREX exclusive-monitor pair. CLZ is supplied by a software binary-search fallback (~12 cycles). The M0 line is the most architecturally constrained target in the suite.

| KVB Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---|
| SCHED_COOP_001 (yields/10 s) | 2,518,635 | 1,808,448 | 1,196,863 | n/a | **1.39x** | **2.10x** | - |
| SYNC_SEM_FAST_001 (p/s) | 247,054 | 109,762 | 195,543 | n/a | **2.25x** | **1.26x** | - |
| SYNC_MUTEX_FAST_001 (p/s) | 216,834 | 75,125 | 87,875 | n/a | **2.89x** | **2.47x** | - |
| SYNC_MUTEX_PCP_FAST_001 (p/s) | 108,334 | 75,595 | 88,523 | n/a | **1.43x** | **1.22x** | - |
| IPC_QUEUE_FAST_001 (p/s) | 77,975 | 38,593 | 60,849 | n/a | **2.02x** | **1.28x** | - |
| SYNC_MUTEX_OWNERSHIP_001 | PASS | PASS | PASS | PASS | parity | parity | parity |
| TIME_SLEEP_001 (elapsed us) | PASS @10476 | PASS @10005 | PASS @9537 | n/a | - | - | - |

**Binary size, linked KVB suite ELF:**

| Kernel | .text bytes | vs TaktOS |
|---|---:|---|
| TaktOS | 31,784 | - |
| FreeRTOS | 32,208 | +1.3% |
| ThreadX | 35,764 | +12.5% |
| Zephyr | n/a | - |

Zephyr is not run on STM32F0308 in this revision. Zephyr's STM32F030R8 board support exists but does not run at the same parity bar (`CONFIG_HEAP_MEM_POOL_SIZE=0`, MPU off, validation regimes matched) used for M33 and M4. A parity Zephyr build for this target is open work.

### Thread-Metric (cross-RTOS, industry-standard throughput)

Thread-Metric is the de-facto cross-RTOS throughput benchmark (originally from Express Logic / Eclipse ThreadX). Same GCC 12.2.x toolchain pinning as KVB. The numbers below are steady-state windows from the appendix of the benchmark report.

#### nRF54L15 * Cortex-M33 * 128 MHz

| Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| TM1 Basic Processing | 374,397 | 374,335 | 374,394 | 373,427 | 1.00x | 1.00x | 1.00x |
| TM2 Cooperative Sched. | 35,209,422 | 21,557,088 (warn) | 16,541,318 | 13,996,145 | **1.63x** | **2.13x** | **2.52x** |
| TM3 Preemptive Sched. | 13,944,360 | 6,311,100 | 7,912,474 | 8,735,995 | **2.21x** | **1.76x** | **1.60x** |
| TM6 Message Processing | 22,979,552 | 7,251,900 | 19,092,261 | 7,308,438 | **3.17x** | **1.20x** | **3.14x** |
| TM7 Synchronization | 49,838,602 | 12,142,194 | 34,263,828 | 17,619,784 | **4.10x** | **1.45x** | **2.83x** |
| TM8 Mutex Processing | 19,406,089 | 7,075,918 | 7,539,185 | 6,701,746 | **2.74x** | **2.57x** | **2.90x** |
| TM9 Mutex Barging (total) | 19,284,580 | 6,866,504 | 7,373,447 | 6,982,184 | **2.81x** | **2.62x** | **2.76x** |
| TM9 Barge fraction (lower = better) | 0.003 | 0.017 | 0.016 | 0.008 | - | - | - |

#### nRF52832 * Cortex-M4F * 64 MHz

| Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| TM1 Basic Processing | 124,626 | 133,534 | 124,634 | 163,500 | 0.93x | 1.00x | 0.76x |
| TM2 Cooperative Sched. | 14,172,449 | 8,114,857 (warn) | 6,008,402 | 4,980,000 | **1.75x** | **2.36x** | **2.85x** |
| TM3 Preemptive Sched. | 5,133,419 | 2,255,396 | 2,807,117 | 3,154,000 | **2.28x** | **1.83x** | **1.63x** |
| TM6 Message Processing | 7,481,555 | 2,351,340 | 6,027,274 | 2,747,000 | **3.18x** | **1.24x** | **2.72x** |
| TM7 Synchronization | 16,100,253 | 4,355,732 | 11,341,405 | 6,263,000 | **3.70x** | **1.42x** | **2.57x** |
| TM8 Mutex Processing | 6,891,808 | 2,603,737 | 2,691,913 | 2,432,000 | **2.65x** | **2.56x** | **2.83x** |
| TM9 Mutex Barging (total) | 6,800,973 | 2,318,856 | 2,522,960 | 2,405,000 | **2.93x** | **2.70x** | **2.83x** |
| TM9 Barge fraction (lower = better) | 0.008 | 0.051 | 0.047 | 0.024 | - | - | - |

**TM1 note:** All four kernels score within a few percent of each other on M33 single-thread compute. The Zephyr 0.76x value on M4 reflects loop-body code generation only; TM1 does not exercise the scheduler. TaktOS leads every test that does exercise the scheduler.

**TM2 (warn) note:** Both boards report "Invalid counter value(s). Cooperative counters should not be more than 1 different than the average" on the FreeRTOS Cooperative Scheduling test in nearly every 30-second window. Counts are reported as captured.

**TM9 (Mutex Barging)** is a TaktOS addition to the upstream Thread-Metric suite. It puts 4 worker threads at one priority against 4 high-priority interlopers, all contending on a priority-protected mutex. The "barge fraction" = interlopers' ops / total ops measures whether the kernel correctly hands the mutex to the high-priority interloper or whether barging cuts in. TaktOS's PCP path holds barge fraction at or below 0.008 on both boards; PI-based kernels run higher.

**TM4 and TM5 are not run** on any kernel in this suite. TM4 requires a hardware timer IRQ owned by the test harness - TaktOS does not own application IRQs by design. TM5 measures dynamic memory allocation - TaktOS has no heap by design.

### Effect of `TAKT_INLINE_OPTIMIZATION`

`TAKT_INLINE_OPTIMIZATION` forces `TAKT_ALWAYS_INLINE` on the semaphore, mutex, and queue fast paths. Removing the define reverts those to regular function calls. The effect is largest on TM7 (entire workload is semaphore give/take) and grows on M33 because its instruction cache eliminates flash wait states, making BL/BX call overhead proportionally more expensive. The Rev 3.1 GCC 15.2.1 split was -14% to -28% across TM6/TM7 on both boards; the GCC 12.2.x numbers above are with the optimisation on. For certification builds where `TAKT_ALWAYS_INLINE` is disabled, expect a similar shape of degradation.

---

## Architecture

### File map

```
TaktOS/
+-- include/
|   +-- TaktOS.h              # public API + arch port function declarations
|   +-- TaktKernel.h          # private API (kernel objects only)
|   +-- TaktOSThread.h        # SAFETY BOUNDARY
|   +-- TaktOSSem.h           # SAFETY BOUNDARY - fast path always_inline
|   +-- TaktOSMutex.h         # SAFETY BOUNDARY
|   +-- TaktOSQueue.h         # SAFETY BOUNDARY - fast path always_inline
|   +-- posix/                # POSIX PSE51 layer - QM
+-- ARM/
|   +-- include/TaktOSCriticalSection.h  # SAFETY BOUNDARY - inline PRIMASK
|   +-- src/TaktKernelCM.cpp  # TaktOSTickInit() + TaktOSStackInit()
|   +-- cm0/PendSV_M0.S       # SAFETY BOUNDARY - M0/M0+
|   +-- cm4/PendSV_M4.S       # SAFETY BOUNDARY - M4/M4F
|   +-- cm7/PendSV_M7.S       # SAFETY BOUNDARY - M7
|   +-- cm33/PendSV_M33.S     # SAFETY BOUNDARY - M33
|   +-- cm55/PendSV_M55.S     # SAFETY BOUNDARY - M55
+-- RISCV/
|   +-- include/              # shared RV32 headers (CriticalSection, KernelCore, KernelTick)
|   +-- rv32/                 # RV32IMAC arch lib - libTaktOS_RV32.a
|   |   +-- src/ctx_switch.S          # SAFETY BOUNDARY - trap save/restore (rv32imac_zicsr)
|   |   +-- src/TaktKernelRiscv.cpp   # stack init + weak CLINT tick / trap dispatch
|   |   +-- include/port_rv32.h       # 34-word frame layout (x1-x31 + mepc + mstatus)
|   |   +-- Eclipse/                  # Debug + Release configurations
|   +-- esp32c3/              # design sketch for non-CLINT RV32; not built
+-- src/
|   +-- taktos.cpp            # scheduler, init
|   +-- taktos_sem.cpp        # semaphore slow paths
|   +-- taktos_mutex.cpp      # mutex slow paths
|   +-- taktos_queue.cpp      # queue slow paths
|   +-- taktos_thread.cpp     # thread lifecycle
|   +-- posix/                # PSE51 implementation
+-- Benchmark/ThreadMetric/   # Thread-Metric Eclipse + west projects
|   +-- nRF52832/             # TaktOS, FreeRTOS, ThreadX projects
|   +-- nRF54L15/             # TaktOS, FreeRTOS, ThreadX projects
|   +-- Zephyr/               # one west project per TM test, board-selectable
|   +-- STM32F0308/  STM32L432KC/  STM32L475VG/  STM32H753ZI/  STM32G474/
|   +-- SAM4LC8C/  ArduinoNanoR4/  ESP32C/  ESP32C3_RAM/
+-- KVB/                      # Kernel Validation Benchmark - primary on-target test harness
|   +-- Targets/nRF52832/     # TaktOS, FreeRTOS, ThreadX KVB suite projects
|   +-- Targets/nRF54L15/     # same
|   +-- Targets/STM32F0308/   # 3-way (TaktOS, FreeRTOS, ThreadX)
|   +-- Targets/Zephyr/       # 4th-kernel KVB suite (west project)
+-- examples/                 # basic, mutex, posix, queue, std, mpu_guard
+-- test/                     # MPU vector reloc tests (on-target)
```

### Arch port

Each architecture implements four C functions declared in `TaktOS.h`:

```c
void  TaktOSTickInit  (uint32_t KernClockHz, uint32_t TickHz, TaktOSTickClockSrc_t tickClockSrc,
                       uint32_t TickPriority);
void  TaktOSCtxSwitch (void);   // request deferred context switch
void  TaktOSStartFirst(void);   // launch first task - never returns
void *TaktOSStackInit (void *stackTop, void (*entry)(void*), void *arg);
```

`TickPriority` comes straight from `TaktOSCfg_t.TickPriority` on the TaktOS standard priority scale. Each port converts it to the representation required by its interrupt controller.

On Cortex-M, `TaktArchTickPrioField()` converts the TaktOS priority to the raw 8-bit SHPR/IPR priority field used by direct register writes. `TaktArchTickPrioNvic()` converts the same TaktOS priority to the unshifted logical priority value expected by CMSIS `NVIC_SetPriority()`. Do not pass the raw field returned by `TaktArchTickPrioField()` directly to `NVIC_SetPriority()`.

On ESP32-C3, `TaktArchTickPrioMap()` converts the TaktOS priority to the INTMTX priority encoding. RV32 CLINT has no configurable machine-timer interrupt priority and ignores `TickPriority`.

A HAL that replaces the weak `TaktOSTickInit` must use the mapper appropriate to the API it calls. On Cortex-M, PendSV stays pinned at `0xFF` regardless of the tick priority, so a context switch always tail-chains after the tick and after every user ISR.

`TaktOSTickInit` and the trap dispatch entry are declared weak. The kernel ships CLINT-compatible weak defaults for RV32 (`RISCV/rv32/src/TaktKernelRiscv.cpp`) and SysTick-based defaults for ARM (`ARM/src/TaktKernelCM.cpp`). Chips whose interrupt fabric differs from the standard pattern (ESP32-C3 SYSTIMER + INTMTX, GD32VF103 CLINT at a non-standard base) override these with strong defs - in app code, in chip-specific source, or anywhere convenient. TaktOS is HAL-agnostic.

### Layer model

| Layer | Modules | LOC | ISA-specific? |
|---|---|---|---|
| Land | `TaktOSCriticalSection.h`, `TaktKernelCore.h`, tick MMIO | ~80 per arch | Arch files only |
| Roots | scheduler, semaphore, mutex, queue, task | ~760 portable C++23 | None |
| Arch port | `ARM/cm*/PendSV_*.S`, `ARM/src/TaktKernelCM.cpp`; `RISCV/rv32/src/ctx_switch.S`, `TaktKernelRiscv.cpp` | ~200-300 per arch | ARM or RISC-V only |
| Fruit | POSIX PSE51 (pthread, sem_t, mqueue, timer), C11 `<threads.h>` draft | ~1,800 | None |

**Safety boundary total: ~1,500 LOC** (portable kernel + one arch port).

---

## Certification strategy

TaktOS is supplied as an **IEC 61508 Safety Element out of Context (SEooC)** with supporting evidence artifacts. Assessment and certification of the integrated product are performed by the integrator's own assessor during product safety qualification - I-SYST does not pay for or represent that TaktOS is itself certified.

Evidence artifacts provided with TaktOS are intended to support the integrator's safety case:

- MC/DC coverage reports (portable kernel modules, run on x86 host)
- Branch/line coverage reports for per-variant assembly
- Requirements traceability between engineering specification, source, and tests
- Unit test suite (host-native, Google Test, no arch dependency)
- FMEA worksheets and development process documentation

The small safety boundary (~1,500 LOC) is designed to keep the integrator's MC/DC tractability burden within reach. Static allocation, zero heap, application-owned IRQs, and a single critical-section mechanism are the design choices that make the boundary small - they are correct by design, not trimmed for cost.

---

## Development environment setup

The fastest way to get a working embedded toolchain is **IOcomposer** -
an AI-assisted IDE for embedded development. One script installs the
complete environment: IDE, GCC ARM and RISC-V toolchains, OpenOCD, and
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
**File -> Open Projects from File System...** -> browse to
`TaktOS/ARM/cm4/Eclipse/` (or another arch folder, e.g. `TaktOS/RISCV/rv32/Eclipse/`).

### Manual toolchain install

If you prefer to manage toolchains yourself:

- **ARM:** [xPack GNU Arm Embedded GCC](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack)
- **RISC-V:** [xPack GNU RISC-V Embedded GCC](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack) (13.2.0 with picolibc)
- **Debug:** [xPack OpenOCD](https://github.com/xpack-dev-tools/openocd-xpack) or SEGGER J-Link

---

## Build

### How TaktOS integrates with your firmware

The TaktOS kernel **builds to a static library**, one `.a` per architecture variant. Your firmware is a **separate application project** that links against that library. Nothing from the kernel is merged into your source tree - no `taktos_config.h` to edit, no kernel generator, no devicetree.

Kernel library builds live under `ARM/<arch>/Eclipse/<config>/` for ARM and `RISCV/rv32/Eclipse/<config>/` for RV32. Example: `ARM/cm4/Eclipse/ReleaseFPU/` builds `libTaktOS_M4.a` (Cortex-M4 + FPU hard-float, size-optimized). `RISCV/rv32/Eclipse/Release/` builds `libTaktOS_RV32.a` (`rv32imac_zicsr / ilp32`). Build by importing the relevant `Eclipse/` folder as an Eclipse project and hitting build; the `.a` lands in the configuration output folder.

In your firmware's `.cproject` (or Makefile), link as:

    -L .../ARM/cm4/Eclipse/ReleaseFPU
    -lTaktOS_M4
    -I .../include          # public API: TaktOS.h, TaktOSThread.h, ...
    -I .../ARM/include      # arch types: TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD

For RV32 substitute `RISCV/rv32/Eclipse/Release` and `-lTaktOS_RV32` plus `-I .../RISCV/include` and `-I .../RISCV/rv32/include`.

The Thread-Metric and KVB projects under `Benchmark/ThreadMetric/` and `KVB/Targets/` are worked examples of this model on multiple MCUs.

**Why a library rather than source-in-tree:**
- Clean certification boundary - the library is the SEooC; your app is not.
- No config drift across projects - all tunables are runtime arguments to `TaktOSInit()`.
- Same kernel binary on every project targeting the same core - MC/DC coverage runs once, not per firmware.

### Toolchains

| Target | Prefix | ISA flags |
|---|---|---|
| ARM Cortex-M0/M0+ | `arm-none-eabi-` | `-mcpu=cortex-m0plus` |
| ARM Cortex-M4/M4F | `arm-none-eabi-` | `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M7 | `arm-none-eabi-` | `-mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M33 | `arm-none-eabi-` | `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M55 | `arm-none-eabi-` | `-mcpu=cortex-m55 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| RISC-V RV32IMAC | `riscv-none-elf-` | `-march=rv32imac_zicsr -mabi=ilp32` |

All targets: `-std=gnu++23 -fno-exceptions -fno-rtti -Os`.

### Configuration

There is no user config header. All kernel parameters are passed at runtime through a single `TaktOSCfg_t` struct. Any zero field falls back to the documented per-field default, so the standard configuration only needs the chip clock specified:

```c
TaktOSCfg_t cfg = { .KernClockHz = 64000000u };
TaktOSInit(&cfg);
```

Override only the fields that deviate from the default - typical full example for a chip that needs an explicit software-interrupt address:

```c
TaktOSCfg_t cfg = {
    .KernClockHz     = 16000000u,            // tick peripheral input clock (Hz)
    .TickHz          = 1000u,                // default; can be omitted
    .TickClockSrc    = TAKTOS_TICK_CLOCK_PROCESSOR,  // default; can be omitted
    .TickPriority    = TAKTOS_PRIORITY_LOWEST,       // TaktOS priority scale; 0 = port default
    .HandlerBaseAddr = (uintptr_t)gRamVectorTable,   // ARM MPU vector relocation
    .SoftIntAddr     = 0x600C0014u,          // RISC-V ESP32-C3 SYSTEM_CPU_INTR_FROM_CPU_0
};
TaktOSInit(&cfg);
```

`TickPriority` uses the **TaktOS standard priority scale** - the same `TAKTOS_PRIORITY_*` levels used for thread priority and mutex ceilings, `LOWEST(1)` through `CRITICAL(31)`, higher value = more urgent. The port converts the level to the hardware encoding:

| TaktOS level | Cortex-M raw SHPR/IPR field | ESP32-C3 INTMTX | RV32 CLINT |
|---|---|---|---|
| `TAKTOS_PRIORITY_LOWEST` (1) | `0xFF` (lowest) | 1 | n/a |
| `TAKTOS_PRIORITY_LOW` (8) | `0xC3` | 2 | n/a |
| `TAKTOS_PRIORITY_NORMAL` (16) | `0x7F` | 4 | n/a |
| `TAKTOS_PRIORITY_HIGH` (24) | `0x3B` | 5 | n/a |
| `TAKTOS_PRIORITY_CRITICAL` (31) | `0x00` (highest) | 7 | n/a |
| `TAKTOS_TICK_PRIORITY_DEFAULT` (0) | `0xFF` | 2 | n/a |

The Cortex-M column is the raw 8-bit priority field written directly to SHPR/IPR. A Cortex-M part implements only the top `__NVIC_PRIO_BITS` bits of that field, so the effective hardware priority is quantised while remaining monotonic. Code that uses CMSIS `NVIC_SetPriority()` must use `TaktArchTickPrioNvic(TickPriority, __NVIC_PRIO_BITS)` instead of passing the raw field value. RV32 CLINT ignores the field because the machine timer interrupt has no configurable priority.

A tick priority above an application ISR lets the tick pre-empt that ISR. Scheduler state stays consistent because kernel critical sections mask all interrupts; the cost is that the ISR worst-case latency then includes the tick handler execution time.

Per-chip examples for `SoftIntAddr`:

- CLINT-based chips (FE310, BL616, R9A02G021, ESP32-C6/H2): `0x02000000u` standard CLINT MSIP - or omit (default).
- GD32VF103 (CLINT at non-standard base): `0xD1000000u`.
- ESP32-C3 (no CLINT - uses INTMTX-routed SYSTEM register): `0x600C0014u` SYSTEM_CPU_INTR_FROM_CPU_0.

Stack overflow detection (paint+check guard word) is always active. MPU/PMP guard regions are library build options, not application defines.

---

## Product family

| Product | Role |
|---|---|
| **TaktOS** | Deterministic kernel - bare-metal RTOS (this repo) |
| **IOsonata** | Driver/interface framework (`DevIntrf_t` bus injection) |
| **Voci** | C++ communication protocol framework (work in progress). Bluetooth host in refactor from IOsonata; WiFi, cellular, IEEE 802.15.4, LoRaWAN, IP, Ethernet, USB, CAN, Modbus, MQTT, CoAP, HTTP planned. HAL- and OS-agnostic; pairs with IOsonata + TaktOS as the reference integration. |
| **IOcomposer** | AI-assisted embedded IDE / development environment |

IOsonata architecture (the Land/Roots/Trees/Fruit metaphor and `DevIntrf_t` driver model that TaktOS builds on) is documented in *Beyond Blinky* by Nguyen Hoan Hoang.

---

## Status

- [x] ARM Cortex-M0/M0+ port - KVB + Thread-Metric validated on STM32F0308-DISCO (M0 @ 48 MHz)
- [x] ARM Cortex-M4/M4F port - KVB + Thread-Metric validated on nRF52832 (M4F @ 64 MHz)
- [x] ARM Cortex-M7 port - functional, no on-target benchmark run yet
- [x] ARM Cortex-M33 port - KVB + Thread-Metric validated on nRF54L15 (M33 @ 128 MHz)
- [x] ARM Cortex-M55 port - functional, no on-target benchmark run yet
- [x] POSIX PSE51 layer (pthread, sem, mqueue, timer)
- [x] C11/C17 `<threads.h>` draft (`include/threads.h`, `src/std/threads.cpp`)
- [x] KVB suite TaktOS / FreeRTOS / ThreadX / Zephyr on nRF52832 and nRF54L15; 3-way on STM32F0308
- [x] Thread-Metric TM1/TM2/TM3/TM6/TM7/TM8/TM9 - all four kernels on nRF52832 and nRF54L15
- [x] RISC-V RV32IMAC arch lib (`libTaktOS_RV32.a`) - compile-tested, CLINT-based weak defaults, ESP32-C3 strong-def path
- [ ] RISC-V RV32 on-target validation (KVB / Thread-Metric on RV32 silicon)
- [ ] RISC-V FP variants (`ilp32f`, `ilp32d`) - added when a target chip needs them
- [ ] MC/DC coverage run - pending KVB host platform port (gcov-instrumented x86 build of cert-boundary modules running KVB test bodies)
- [ ] POSIX PSE51 functional test suite - pending KVB POSIX test group
- [ ] IEC 61508 SIL 2 certification campaign

TM4 and TM5 are not planned: TM4 requires kernel-owned IRQs (TaktOS does not have them by design), TM5 requires dynamic allocation (TaktOS does not have it by design).

---

Copyright (c) 2026 I-SYST Inc. TaktOS is released under the MIT License - see [LICENSE](LICENSE).
