# TaktOS

**Deterministic kernel for ARM Cortex-M**

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

¹ Design target from llvm-mca cycle budget. Measured performance via KVB (validation-gated, primary) and Thread-Metric on real hardware — nRF52832 (Cortex-M4F @ 64 MHz) and nRF54L15 (Cortex-M33 @ 128 MHz). Both benchmark suites use GCC 12.2.x across all four kernels (TaktOS / FreeRTOS / ThreadX / Zephyr) so Zephyr is compared on equal terms.

**RISC-V RV32 port:** placeholder, not in v1.x scope. The `RISCV/`
directory and the `Benchmark/ThreadMetric/ESP32C*/` projects in this
repository contain placeholder code only. No functional RISC-V port is
intended for any v1.x release. No RISC-V Thread-Metric results exist.
Do not treat anything under `RISCV/` as a working port. RISC-V will be
revisited as a separate scoped effort when product demand justifies the
silicon validation campaign and the IOsonata build path. See
*TaktOS Engineering Specification* §11 #6 for the canonical statement.

---

## On-target benchmark results

Two benchmark suites are run on the same hardware with the same toolchain pinning:

- **KVB (Kernel Validation Benchmark)** — primary, validation-gated. Every reported throughput number ships with an accompanying behavioural PASS (mutex ownership rejection, sleep-duration lower bound, parameter validation), so the comparison reflects correct behaviour, not just speed. 10-second measurement window, 1 kHz tick.
- **Thread-Metric** (eclipse-threadx/threadx, MIT) — industry-standard cross-RTOS throughput benchmark, kept as the cross-check against the de-facto baseline. 30-second steady-state windows, 1 kHz tick.

**Toolchain pinning.** All four kernels are built with **GCC 12.2.x** — same major.minor that the Zephyr SDK ships (NCS v3.3.0 → GCC 12.2.0; xPack arm-none-eabi-gcc 12.2.1 for the others). Zephyr cannot substitute its SDK toolchain without breaking SDK guarantees, so the others align down to it. This removes the toolchain as a confounder; remaining differences in the headline tables reflect the kernels themselves.

---

### KVB — nRF54L15 · Cortex-M33 · 128 MHz · GCC 12.2.x · `-Os` · 1 kHz tick

| KVB Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| `SCHED_COOP_001` (yields/10 s)         | 10,658,299 | 6,730,284  | 4,034,399  | 4,608,098  | **1.58×** | **2.64×** | **2.31×** |
| `SYNC_SEM_FAST_001` (pairs/s)          |  1,468,992 |   390,280  | 1,085,516  |   526,438  | **3.76×** | **1.35×** | **2.79×** |
| `SYNC_MUTEX_FAST_001` (pairs/s)        |  1,323,191 |   269,394  |   460,008  |   424,115  | **4.91×** | **2.88×** | **3.12×** |
| `SYNC_MUTEX_PCP_FAST_001` (pairs/s) ¹  |    559,345 |   265,790  |   459,687  |   424,115  | **2.10×** | **1.22×** | **1.32×** |
| `IPC_QUEUE_FAST_001` (pairs/s)         |    342,282 |   171,693  |   303,617  |   153,958  | **1.99×** | **1.13×** | **2.22×** |
| `SYNC_MUTEX_OWNERSHIP_001`             | PASS       | PASS       | PASS       | PASS       | parity | parity | parity |
| `TIME_SLEEP_001` ²                     | PASS @10399 µs | PASS @9479 µs | PASS @9467 µs | PASS @11000 µs | — | — | — |

¹ TaktOS uses **Priority Ceiling Protocol (PCP)**; FreeRTOS / ThreadX / Zephyr use **Priority Inheritance (PI)**. The protocols differ — both boost holder priority, but PCP is a single ceiling assignment while PI walks the wait chain on contention. Numbers are still meaningful as "what does the kernel do when you ask for a priority-boosted mutex," but they do not measure the same algorithm.

² `TIME_SLEEP_001` calls `Sleep(10 ticks)` at 1 kHz tick. KVB has two acceptance bars: 90 % floor (≥ 9000 µs — all four kernels PASS) and **strict at-least-N-ticks** (≥ 10000 µs — only **TaktOS and Zephyr** satisfy this). TaktOS satisfies it by design (the kernel adds `+1u` at every timed-wait site since the May 2026 sleep-API split). Zephyr satisfies it because `k_sleep` rounds up similarly. FreeRTOS and ThreadX clear the 90 % floor but undershoot the nominal by ~520 µs and ~530 µs respectively.

### KVB — nRF52832 · Cortex-M4F · 64 MHz · GCC 12.2.x · `-Os` · 1 kHz tick

| KVB Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T/FR | T/TX | T/Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| `SCHED_COOP_001` (yields/10 s)         | 4,093,473 | 2,673,007 | 1,929,845 | 1,679,433 | **1.53×** | **2.12×** | **2.44×** |
| `SYNC_SEM_FAST_001` (pairs/s)          |   443,662 |   139,804 |   359,094 |   204,934 | **3.17×** | **1.24×** | **2.16×** |
| `SYNC_MUTEX_FAST_001` (pairs/s)        |   375,768 |    97,238 |   157,013 |   170,778 | **3.86×** | **2.39×** | **2.20×** |
| `SYNC_MUTEX_PCP_FAST_001` (pairs/s) ¹  |   175,466 |    97,239 |   157,404 |   168,060 | **1.80×** | **1.11×** | **1.04×** |
| `IPC_QUEUE_FAST_001` (pairs/s)         |   110,874 |    53,586 |   100,786 |    70,166 | **2.07×** | **1.10×** | **1.58×** |
| `SYNC_MUTEX_OWNERSHIP_001`             | PASS      | PASS      | PASS      | PASS      | parity | parity | parity |
| `TIME_SLEEP_001` ²                     | PASS @10153 µs | PASS @9696 µs | PASS @9429 µs | PASS @11297 µs | — | — | — |

**KVB suite outcome on both boards: 7 PASS / 0 FAIL / 7 total — every kernel, every run.**

KVB sources and per-kernel ports live under `KVB/`. Per-board run logs:

- `KVB/Targets/nRF52832/KVB_Results_nRF52832.txt`
- `KVB/Targets/nRF52832/nRF52832_KVB_Comparison.md` *(canonical 5-run-aggregate, strict-parity)*
- `KVB/Targets/nRF54L15/KVB_Results_nRF54L15.txt`
- `KVB/Targets/STM32F0308/KVB_Results_STM32F0308.txt`
- `KVB/Targets/STM32F0308/STM32F0308_KVB_Comparison.md`

---

### Thread-Metric — nRF54L15 · Cortex-M33 · 128 MHz · GCC 12.2.x · `-Os` · 1 kHz tick

Steady-state iteration count, higher = better. Source logs: `Benchmark/ThreadMetric/nRF54L15/TestResults_nRF54L15_gcc_12_2.txt`.

| Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T / FR | T / TX | T / Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| TM1  Basic Processing       |    374,397 |    374,335 |    374,394 |    373,427 | 1.00× | 1.00× | 1.00× |
| TM2  Cooperative Scheduling | 35,209,422 | 21,557,088 ⚠ | 16,541,318 | 13,996,145 | **1.63×** | **2.13×** | **2.52×** |
| TM3  Preemptive Scheduling  | 13,944,360 |  6,311,100 |  7,912,474 |  8,735,995 | **2.21×** | **1.76×** | **1.60×** |
| TM6  Message Processing     | 22,979,552 |  7,251,900 | 19,092,261 |  7,308,438 | **3.17×** | **1.20×** | **3.14×** |
| TM7  Synchronization        | 49,838,602 | 12,142,194 | 34,263,828 | 17,619,784 | **4.10×** | **1.45×** | **2.83×** |
| TM8  Mutex Processing       | 19,406,089 |  7,075,918 |  7,539,185 |  6,701,746 | **2.74×** | **2.57×** | **2.90×** |
| TM9  Mutex Barging (Total)  | 19,284,580 |  6,866,504 |  7,373,447 |  6,982,184 | **2.81×** | **2.62×** | **2.76×** |
| TM9  Barge fraction ³       |      0.003 |      0.017 |      0.016 |      0.008 |   —    |   —    |   —    |
| **Geomean (TM2–TM8)**       |            |            |            |            | **2.64×** | **1.76×** | **2.51×** |

**TM1 note:** all four kernels score essentially the same — TM1 measures only the loop-body code generation, no scheduling occurs.

### Thread-Metric — nRF52832 · Cortex-M4F · 64 MHz · GCC 12.2.x · `-Os` · 1 kHz tick

FreeRTOS TM2 ⚠ — produces "counters more than 1 different from average" determinism warnings on every 30-second window; counts are reported as captured. Source logs: `Benchmark/ThreadMetric/nRF52832/TestResults_nRF52832_gcc_12_2.txt`.

| Test | TaktOS | FreeRTOS V11.3 | ThreadX V6.4.2 | Zephyr 4.3.99 | T / FR | T / TX | T / Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| TM1  Basic Processing       |    124,626 |    133,534 |    124,634 |    163,500 | 0.93× | 1.00× | 0.76× |
| TM2  Cooperative Scheduling | 14,172,449 |  8,114,857 ⚠ |  6,008,402 |  4,980,000 | **1.75×** | **2.36×** | **2.85×** |
| TM3  Preemptive Scheduling  |  5,133,419 |  2,255,396 |  2,807,117 |  3,154,000 | **2.28×** | **1.83×** | **1.63×** |
| TM6  Message Processing     |  7,481,555 |  2,351,340 |  6,027,274 |  2,747,000 | **3.18×** | **1.24×** | **2.72×** |
| TM7  Synchronization        | 16,100,253 |  4,355,732 | 11,341,405 |  6,263,000 | **3.70×** | **1.42×** | **2.57×** |
| TM8  Mutex Processing       |  6,891,808 |  2,603,737 |  2,691,913 |  2,432,000 | **2.65×** | **2.56×** | **2.83×** |
| TM9  Mutex Barging (Total)  |  6,800,973 |  2,318,856 |  2,522,960 |  2,405,000 | **2.93×** | **2.70×** | **2.83×** |
| TM9  Barge fraction ³       |      0.008 |      0.051 |      0.047 |      0.024 |   —    |   —    |   —    |
| **Geomean (TM2–TM8)**       |            |            |            |            | **2.62×** | **1.81×** | **2.47×** |

**TM1 note (nRF52832):** the loop-body code generation favours FreeRTOS and Zephyr slightly on this M4 build; TM1 does not exercise the scheduler so the ranking has no bearing on actual scheduling/IPC workloads.

³ **TM9 Mutex Barging** — TaktOS-added test (not in upstream Thread-Metric). Four worker threads at priority 10 contend with four high-priority interlopers at priority 3 on a priority-protected mutex. The `barge fraction` = interloper ops / total ops; lower is closer to the ideal of high-priority threads always winning. TaktOS PCP holds barging ≤ 0.008 on both boards; Zephyr / ThreadX / FreeRTOS use PI and run higher (0.016–0.051). Different protocol shapes, not implementation defects.

**TM4 and TM5 are not run.** TM4 requires kernel-owned hardware-timer IRQs — TaktOS does not own application IRQs by design. TM5 measures dynamic memory allocation — TaktOS has no heap by design.

---

### Effect of `TAKT_INLINE_OPTIMIZATION`

`TAKT_INLINE_OPTIMIZATION` forces `TAKT_ALWAYS_INLINE` on the semaphore, mutex, and queue fast paths. Removing the define reverts those to regular function calls.

The A/B below is the **GCC 15.2.1** capture from the previous benchmark cycle. Re-running at the new GCC 12.2.x toolchain pinning is open work; the optimisation mechanism is structural (call-site inlining of the fast paths into the caller's loop body) and the qualitative impact carries to GCC 12.2.x.

| Test | nRF52832 M4  with | without | M4 Δ | nRF54L15 M33  with | without | M33 Δ |
|---|---:|---:|---:|---:|---:|---:|
| TM6 Message         |  8,952,189 |  7,663,774 | −14.4 % | 27,608,741 | 22,310,845 | −19.2 % |
| TM7 Synchronization | 20,381,897 | 15,203,494 | −25.4 % | 59,961,406 | 43,116,795 | −28.1 % |

TM7 is affected more than TM6 on both boards — the entire workload is semaphore give/take. The M33 takes a larger penalty than the M4: its instruction cache eliminates flash wait states, making `BL`/`BX` call overhead proportionally more expensive. For certification builds where `TAKT_ALWAYS_INLINE` is disabled, the *without* column is the expected performance baseline.

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
├── RISCV/                    # PLACEHOLDER — not in v1.x scope, not functional
│   └── rv32/                 # design sketch only; do not use
├── src/
│   ├── taktos.cpp            # scheduler, init
│   ├── taktos_sem.cpp        # semaphore slow paths
│   ├── taktos_mutex.cpp      # mutex slow paths
│   ├── taktos_queue.cpp      # queue slow paths
│   ├── taktos_thread.cpp     # thread lifecycle
│   └── posix/                # PSE51 implementation
├── Benchmark/ThreadMetric/  # Thread-Metric Eclipse projects + per-board run logs
│   ├── nRF52832/            #   four-way runs at GCC 12.2.x
│   └── nRF54L15/            #   four-way runs at GCC 12.2.x
├── KVB/                     # Kernel Validation Benchmark — primary on-target test harness
│   ├── docs/                #   design.md, porting-guide.md
│   ├── ports/               #   per-platform timing/console + per-kernel adapter
│   └── Targets/             #   per-board KVB suites (TaktOS / FreeRTOS / ThreadX / Zephyr)
│       ├── nRF52832/        #     5-run-aggregate canonical comparison MD
│       ├── nRF54L15/        #     run logs (Zephyr 4.3.99 / NCS v3.3.0)
│       └── STM32F0308/      #     Cortex-M0 reference target
├── examples/                # basic, mutex, posix, queue
└── test/                    # MPU vector reloc tests (on-target)
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
| Arch port | `ARM/cm*/PendSV_*.S`, `TaktKernelCM.cpp` | ~200–300 | ARM only |
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

### How TaktOS integrates with your firmware

The TaktOS kernel **builds to a static library**, one `.a` per architecture
variant. Your firmware is a **separate application project** that links
against that library. Nothing from the kernel is merged into your source
tree — no `taktos_config.h` to edit, no kernel generator, no devicetree.

Kernel library builds live under `ARM/<arch>/Eclipse/<config>/`. Example:
`ARM/cm4/Eclipse/ReleaseFPU/` builds to `libTaktOS_M4.a` — the Cortex-M4
+ FPU hard-float, size-optimized kernel. Build it by importing
`ARM/cm4/Eclipse/` as an Eclipse project and hitting build; the `.a`
lands in the `ReleaseFPU/` output folder.

In your firmware's `.cproject` (or Makefile), link as:

    -L .../ARM/cm4/Eclipse/ReleaseFPU
    -lTaktOS_M4
    -I .../include          # public API: TaktOS.h, TaktOSThread.h, ...
    -I .../ARM/include      # arch headers: TaktKernelCore.h, TaktOSCriticalSection.h

The Thread-Metric projects under `Benchmark/ThreadMetric/` are worked
examples of this model on five different MCUs.

**Why a library rather than source-in-tree:**
- Clean certification boundary — the library is the SEooC; your app is not.
- No config drift across projects — all tunables are runtime arguments to
  `TaktOSInit()`.
- Same kernel binary on every project targeting the same core — MC/DC
  coverage runs once, not per firmware.

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

There is no user config header. All kernel parameters are passed at runtime:

```c
TaktOSInit(64000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
//         tick input Hz, tick rate Hz, tick clock source, handler base
```

Thread memory is statically declared by the application using one helper:

```c
static uint8_t g_MyThreadMem[TAKTOS_THREAD_MEM_SIZE(512)] TAKT_ALIGNED(4);
//                           ^^^ usable stack in bytes; macro adds the per-arch TCB + guard overhead
```

Stack overflow detection (paint+check guard word) is always active.
MPU guard regions are library build options, not application defines.

---

## Product family

| Product | Role |
|---|---|
| **TaktOS** | Deterministic kernel — bare-metal RTOS (this repo) |
| **IOsonata** | Driver/interface framework (`DevIntrf_t` bus injection) |
| **BlueSonata** | Bluetooth connectivity layer |
| **IOcomposer** | AI-assisted embedded IDE / development environment |

IOsonata architecture (the Land/Roots/Trees/Fruit orchard metaphor and `DevIntrf_t` driver model that TaktOS builds on) is documented in *Beyond Blinky* by Nguyen Hoan Hoang.

---

## Status

- [x] ARM Cortex-M0/M0+ port — KVB validated on STM32F0308
- [x] ARM Cortex-M4/M4F port — KVB and Thread-Metric validated on nRF52832
- [x] ARM Cortex-M7 port — functional, no benchmark run yet
- [x] ARM Cortex-M33 port — KVB and Thread-Metric validated on nRF54L15
- [x] ARM Cortex-M55 port — functional, no benchmark run yet
- [x] POSIX PSE51 layer (pthread, sem, mqueue, timer) — replaces the earlier FreeRTOS-shim concept
- [x] IPCP mutex — `TaktOSMutexInitProtect(Ceiling)` / `pthread_mutexattr_setprotocol(PTHREAD_PRIO_PROTECT)`. Plain mutex + IPCP variant exposed through both native and POSIX APIs.
- [x] Sleep API split — `TaktOSThreadSleep(ms)` / `TaktOSThreadSleepTicks(ticks)` / `TaktOSGetTickHz()`; strict at-least-N-ticks bound on every timed wait
- [x] KVB suite — TaktOS, FreeRTOS V11.3, Eclipse ThreadX V6.4.2, Zephyr 4.3.99 — 7/7 PASS on every run, every board (nRF52832, nRF54L15, STM32F0308)
- [x] Thread-Metric TM1/TM2/TM3/TM6/TM7/TM8/TM9 — same four kernels, GCC 12.2.x, on nRF52832 and nRF54L15
- [ ] RISC-V RV32IMAC port — **placeholder, not in v1.x scope.** `RISCV/` and `Benchmark/ThreadMetric/ESP32C*/` contain placeholder code only; not on the v1.x roadmap.
- [ ] MC/DC coverage run — pending KVB host platform port (gcov-instrumented x86 build of cert-boundary modules running KVB test bodies)
- [ ] Contended SYNC and IPC tests in KVB (`SYNC_SEM_CONTEND_001`, `IPC_QUEUE_CONTEND_001`, etc.) — Phase 2 roadmap
- [ ] `TAKT_INLINE_OPTIMIZATION` A/B re-run at GCC 12.2.x — qualitative effect carries from the GCC 15.2.1 capture; quantitative re-run pending
- [ ] IEC 61508 SIL 2 certification campaign

TM4 and TM5 are not planned: TM4 requires kernel-owned IRQs (TaktOS does not have them by design), TM5 requires dynamic allocation (TaktOS does not have it by design).

---

Copyright (c) 2026 I-SYST Inc. TaktOS is released under the MIT License — see [LICENSE](LICENSE).
