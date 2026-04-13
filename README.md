# TaktOS

**Deterministic kernel for ARM Cortex-M and RISC-V RV32**

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

| | ARM Cortex-M | RISC-V RV32IMAC |
|---|---|---|
| **Targets** | Cortex-M0/M0+, M4/M4F, M33, M55 | RV32IMAC machine mode (Zbb recommended) |
| **Context switch** | ~47 cycles ¹ | ~88 cycles Zbb / ~98 no-Zbb ¹ |
| **Interrupt model** | Application owns all IRQ vectors | same |
| **Memory model** | Static allocation only — zero heap | same |
| **Scheduler core** | ~590 LOC portable C++23 | same |
| **Public API** | POSIX PSE51 (`pthread`, `sem_t`, `mq`, `timer`) + native C/C++ | same |
| **Certification target** | IEC 61508 SIL 2 → ASIL D (SEooC) | separate arch port |
| **Platform integration** | IOsonata Land-layer primitives | same model |

¹ Design targets from llvm-mca cycle budget. DWT (ARM) and mcycle (RISC-V) on-target validation pending.

---

## On-target benchmark results — Thread-Metric

**Platform:** nRF54L15 · Cortex-M33 · 128 MHz · arm-none-eabi-gcc 15.2.1 · `-Os` · 1 kHz tick  
**Suite:** eclipse-threadx/threadx Thread-Metric (MIT) · steady-state 300-second iteration counts (higher = better)  
**Build:** All three RTOSes built with identical flags on the same MCU. ThreadX tested with source code.

| Test | TaktOS | ThreadX | FreeRTOS | T / TX | T / FR |
|---|---|---|---|---|---|
| TM1  Basic Processing | 374,422 | 374,403 | 374,303 | 1.00× | 1.00× |
| TM2  Cooperative Scheduling | 42,176,813 | 26,466,010 | 26,474,445 | **1.59×** | **1.59×** |
| TM3  Preemptive Scheduling | 13,362,618 | 11,757,316 | 6,721,773 | **1.14×** | **1.99×** |
| TM6  Message Processing | 27,609,742 | 19,092,528 | 6,947,836 | **1.45×** | **3.97×** |
| TM7  Synchronization | 59,964,325 | 38,375,092 | 11,555,762 | **1.56×** | **5.19×** |
| **Geometric mean (TM2–TM7)** | | | | **1.42×** | **2.84×** |

TaktOS, ThreadX, and FreeRTOS all produced stable steady-state windows in these runs.

**TM1 note:** All three RTOSes score essentially the same on single-thread compute. No context switches occur during the TM1 window — the result reflects compiler output, not RTOS scheduling performance.

**TM4 and TM5 are not run.** TM4 requires a hardware timer IRQ owned by the test harness — TaktOS does not own application IRQs by design. TM5 measures dynamic memory allocation — TaktOS has no heap by design.

**Binary size (TM7 Synchronization .text):**

| RTOS | .text bytes | vs TaktOS |
|---|---|---|
| TaktOS | 6,446 | — |
| ThreadX | 7,239 | +12% |
| FreeRTOS | 8,824 | +37% |

---

## Why TaktOS is faster

### vs FreeRTOS (2.84× geometric mean, directly measured)

1. **CLZ priority bitmap** — `__builtin_clz` on a 32-bit bitmap gives O(1) highest-priority lookup in the scheduler. Updated at event time; the context switch handler reads one precomputed pointer.
2. **PRIMASK critical sections** — `MRS` + `CPSID` / `MSR`: 2 instructions, no pipeline flush. FreeRTOS `BASEPRI` requires `DSB` + `ISB` after every write (~8 cy penalty per boundary on Cortex-M4).
3. **Inlined semaphore fast path** — `TaktOSSemGive` and `TaktOSSemTake` are `always_inline` static functions in `TaktOSSem.h`. The uncontended fast path executes with zero function-call overhead — the dominant gain on TM7.
4. **Direct-pointer queue** — `writePtr`/`readPtr` are direct buffer addresses. Slot address is one load — no index multiply, no modulo. `TaktQueueFastCopy` uses switch-unrolled word assignments for 1–8 word messages.
5. **Short switch path** — the context-switch hot path stays compact, minimizing front-end overhead and helping the scheduler scale cleanly as event frequency rises.

### vs ThreadX (1.42× geometric mean, directly measured, source-tuned)

ThreadX uses a precomputed `execute_ptr` updated on every ready/block operation — the same approach as TaktOS. The gap is smaller than vs FreeRTOS, but still material on nRF54L15. TaktOS leads most strongly on TM2 cooperative (1.59×) and TM7 synchronization (1.56×), where the tighter semaphore and yield paths show the clearest advantage.

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
│   ├── cm4/PendSV_M4.S       # SAFETY BOUNDARY — M4/M7
│   ├── cm33/PendSV_M33.S     # SAFETY BOUNDARY — M33
│   └── cm55/PendSV_M55.S     # SAFETY BOUNDARY — M55
├── RISCV/
│   ├── rv32/                 # GD32VF103 / FE310 (CLINT)
│   │   ├── arch_hal_riscv.h  # SAFETY BOUNDARY — RV32 arch-specific
│   │   ├── ctx_switch.S      # SAFETY BOUNDARY — ~60 LOC
│   │   └── clint.cpp         # TaktOSTickInit() — CLINT mtime/mtimecmp/MSIP
│   └── esp32c3/              # ESP32-C3 / ESP32-C6 (SYSTIMER + interrupt matrix)
│       ├── ctx_switch_rv32.S # SAFETY BOUNDARY
│       └── src/TaktKernelRV32_esp32c3.cpp
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
| Arch port | `ARM/cm*/PendSV_*.S`, `TaktKernelCM.cpp`; `RISCV/*/ctx_switch*.S`, port cpps | ~200–300 per arch | Yes |
| Fruit | POSIX PSE51 (pthread, sem_t, mqueue, timer) | ~1,800 | None |

**Safety boundary total: ~1,454 LOC** (ARM + portable C++23).

---

## Certification strategy

| Phase | Timeline | Cost estimate |
|---|---|---|
| IEC 61508 SIL 2 (portable kernel) | ~6 months | |
| IEC 61508 SIL 2 (ARM arch port) | ~3 months parallel | |
| IEC 61508 SIL 2 (RISC-V arch port) | ~3 months parallel | |
| **Total SIL 2 (dual-arch)** | **~9–12 months** | **$600K–$1.0M** |
| ASIL D uplift | +6 months | +$300K–$500K |

~590 LOC scheduler core = minimal MC/DC burden. Static allocation, zero heap, clean safety boundary. Certifying company owns the IP.

---

## Development environment setup

The fastest way to get a working embedded toolchain is **IOcomposer** —
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
**File → Open Projects from File System…** → browse to
`TaktOS/ARM/cm4/Eclipse/` (or the relevant arch folder).

### Manual toolchain install

If you prefer to manage toolchains yourself:

- **ARM:** [xPack GNU Arm Embedded GCC](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack)
- **RISC-V:** [xPack GNU RISC-V Embedded GCC](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack)
- **Debug:** [xPack OpenOCD](https://github.com/xpack-dev-tools/openocd-xpack) or SEGGER J-Link

---

## Build

### Toolchains

| Target | Prefix | ISA flags |
|---|---|---|
| ARM Cortex-M4/M7 | `arm-none-eabi-` | `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M33/M55 | `arm-none-eabi-` | `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard` |
| ARM Cortex-M0/M0+ | `arm-none-eabi-` | `-mcpu=cortex-m0plus` |
| RISC-V GD32VF103 | `riscv32-unknown-elf-` | `-march=rv32imac_zbb -mabi=ilp32` |
| RISC-V ESP32-C3 | `riscv32-unknown-elf-` | `-march=rv32imc -mabi=ilp32` |
| RISC-V ESP32-C6 | `riscv32-unknown-elf-` | `-march=rv32imafc_zbb -mabi=ilp32f` |

All targets: `-std=gnu++23 -fno-exceptions -fno-rtti -Os`

### Configuration

TaktOS ships as a precompiled static library per architecture variant.
There is no user config header. All kernel parameters are passed at runtime:

```c
TaktOSInit(64000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR);   // tick input Hz, tick rate Hz, tick clock source
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

Documented in *Beyond Blinky* by Nguyen Hoan Hoang.

---

## Status

- [x] ARM Cortex-M0/M0+ port
- [x] ARM Cortex-M4/M7/M33/M55 port — Thread-Metric validated on nRF54L15
- [x] RISC-V RV32IMAC port (GD32VF103 / CLINT)
- [x] RISC-V ESP32-C3 / ESP32-C6 port (SYSTIMER + interrupt matrix)
- [x] POSIX PSE51 layer (pthread, sem, mqueue, timer)
- [x] Thread-Metric TM1/TM2/TM3/TM6/TM7 — TaktOS, FreeRTOS, ThreadX measured on nRF54L15
- [ ] DWT validation on STM32F407 — **required before cycle-count publication**
- [ ] mcycle validation on GD32VF103
- [ ] MC/DC coverage run (`test/unit/`)
- [ ] IEC 61508 SIL 2 certification campaign

TM4 and TM5 are not planned: TM4 requires kernel-owned IRQs (TaktOS does not have them by design), TM5 requires dynamic allocation (TaktOS does not have it by design).

---

*TaktOS · Rev 3.2 · April 2026 · I-SYST inc.*
