# Kernel Validation Benchmark (KVB)

**The open standard for kernel validation and performance evaluation.**

Kernel Validation Benchmark (KVB) is an open, reproducible benchmark and validation test suite for operating-system kernels. It measures how fast a kernel performs while also proving whether the kernel behavior is correct under defined scheduling, synchronization, IPC, interrupt, timing, memory, error-handling, and stress conditions.

KVB is not limited to RTOS kernels. It is designed for embedded RTOSes, microkernels, monolithic kernels, safety kernels, research kernels, and future desktop/server-class kernels.

A kernel benchmark should not only count how fast a primitive runs in a loop. It should also verify that the kernel behaves correctly under real contention, timing pressure, interrupt activity, and invalid-use conditions.

KVB exists to answer two questions:

1. **Does the kernel behave correctly?**
2. **How well does it perform while behaving correctly?**

---

## Why KVB Exists

Kernel benchmarks are often too narrow. Many only measure primitive throughput: context switches per second, queue operations per second, semaphore operations per second, or allocation/free loops. Those numbers can be useful, but they are not enough.

A useful kernel evaluation suite must also test correctness:

- Does a mutex enforce ownership?
- Does priority inheritance actually bound priority inversion?
- Does a timeout expire correctly?
- Does a queue preserve message order?
- Does an ISR wake the correct waiting thread?
- Does the kernel reject invalid API usage safely?
- Does the system remain stable under long-running stress?

KVB combines benchmark and validation in one suite. Performance results are only meaningful when the corresponding behavior has passed validation.

> A fast incorrect kernel is not a win.

---

## Project Goals

KVB is designed to become a neutral, open, reproducible kernel evaluation standard.

Primary goals:

- provide fair cross-kernel performance benchmarks
- provide validation tests for kernel correctness
- expose latency, jitter, throughput, and deterministic behavior
- test real contention instead of only fast paths
- publish raw logs and machine-readable results
- make every result reproducible from source
- support multiple kernels and platforms
- keep kernel ports thin and honest
- avoid hidden emulation of missing kernel behavior

Initial kernel targets:

- TaktOS
- FreeRTOS
- ThreadX
- Zephyr

Initial platform focus:

- Cortex-M
- nRF52832
- nRF54L15

Future platform targets may include Cortex-A, RISC-V, x86/x64, POSIX-hosted kernels, and simulator-based test targets.

---

## What KVB Tests

KVB is organized into test groups.

```text
SCHED   Scheduler behavior and context switching
SYNC    Semaphores, mutexes, ownership, priority inheritance
IPC     Queues, mailboxes, message passing, FIFO correctness
IRQ     Interrupt-to-thread behavior and ISR API legality
TIME    Sleep, timeout, tick, timer accuracy, jitter
MEM     Pools, allocators, stack behavior, memory safety hooks
ERR     Invalid API usage, null arguments, invalid objects
DET     Determinism and latency distribution
STRESS  Long-run stability, race exposure, contention pressure
```

Each test produces a validation state and optional benchmark metrics.

```text
PASS        Test behavior matched the expected result.
FAIL        Test behavior violated the expected result.
UNSUPPORTED Kernel does not provide the required primitive or behavior.
SKIPPED     Test was intentionally not run due to configuration.
INVALID     Test result is unusable due to harness/configuration error.
```

Benchmark metrics may include:

```text
operations per second
cycles per operation
minimum latency
average latency
maximum latency
p50 / p90 / p99 / p999 latency
jitter
timeout error
priority inversion duration
messages per second
context switches per second
stack usage
failure count
```

---

## Validation-Gated Benchmarking

KVB uses validation-gated benchmark results.

A benchmark result is only publishable when the test also proves that the behavior was correct.

Example:

```text
Test: SYNC_MUTEX_FAST_001
Purpose: Measure uncontended mutex lock/unlock throughput.
Validation:
  - lock must succeed
  - unlock must succeed
  - mutex ownership must be correct
Metric:
  - lock/unlock pairs per second
```

Example:

```text
Test: SYNC_MUTEX_OWNERSHIP_001
Purpose: Verify that a non-owner cannot unlock a mutex.
Validation:
  - thread A locks mutex
  - thread B attempts unlock
  - unlock by thread B must fail
Metric:
  - optional error-path latency
```

This prevents meaningless results such as a high mutex throughput number from a kernel configuration that does not enforce mutex ownership.

---

## Repository Layout

```text
KVB/
├── README.md
├── docs/
│   ├── design.md
│   ├── porting-guide.md
│   ├── result-format.md
│   └── test-catalog.md
├── include/
│   ├── kvb.h
│   ├── kvb_config.h
│   ├── kvb_kernel_port.h
│   └── kvb_platform_port.h
├── src/
│   ├── core/
│   └── tests/
│       ├── sched/
│       ├── sync/
│       ├── ipc/
│       ├── irq/
│       ├── time/
│       ├── mem/
│       ├── err/
│       └── stress/
├── ports/
│   ├── kernels/
│   │   ├── taktos/
│   │   ├── freertos/
│   │   ├── threadx/
│   │   └── zephyr/
│   └── platforms/
│       └── cortex_m/
├── configs/
│   ├── taktos/
│   ├── freertos/
│   ├── threadx/
│   └── zephyr/
├── examples/
│   ├── taktos_minimal/
│   ├── freertos_minimal/
│   ├── threadx_minimal/
│   └── zephyr_minimal/
└── tools/
    ├── parse_log.py
    ├── generate_report.py
    └── compare_results.py
```

---

## Architecture

KVB is split into four layers.

```text
+-------------------------------------------------------------+
|                    KVB Test Modules                         |
|  SCHED / SYNC / IPC / IRQ / TIME / MEM / ERR / STRESS       |
+-------------------------------------------------------------+
|                    KVB Core Harness                         |
|  runner, assertions, metrics, logging, result model          |
+-------------------------------------------------------------+
|                    Kernel Port Layer                        |
|  threads, mutexes, semaphores, queues, timers                |
+-------------------------------------------------------------+
|                    Platform Port Layer                      |
|  cycle counter, clock, UART/log output, IRQ trigger, board   |
+-------------------------------------------------------------+
```

### Kernel Port Layer

The kernel port maps KVB primitives to a specific kernel:

- thread create/start/yield/sleep/delete
- semaphore create/wait/post/delete
- mutex create/lock/unlock/delete
- queue create/send/receive/delete
- timer create/start/stop/delete
- kernel start behavior
- kernel feature descriptor

Examples:

```text
ports/kernels/taktos/
ports/kernels/freertos/
ports/kernels/threadx/
ports/kernels/zephyr/
```

### Platform Port Layer

The platform port handles hardware-specific measurement and board support:

- cycle counter
- clock frequency
- time source
- interrupt test source
- UART or logging output
- CPU/board metadata

Examples:

```text
ports/platforms/cortex_m/
ports/platforms/x86_64/
ports/platforms/posix_host/
```

The platform port is separate from the kernel port so multiple kernels can be tested on the same hardware using the same measurement mechanism.

Cortex-M note: `DWT->CYCCNT` is used only when the target core provides it. ARMv6-M devices such as Cortex-M0, Cortex-M0+, and STM32F03xx do not have DWT/CYCCNT, so they need a board-specific timer fallback for microsecond timing.

---

## Current Status

KVB is currently in early implementation.

Implemented foundation:

- core runner
- result model
- validation-gated benchmark logging
- TaktOS kernel port
- FreeRTOS kernel port
- ThreadX kernel port
- Zephyr kernel port
- Cortex-M platform helper
- initial parser/report tools
- minimal examples

Initial test coverage:

```text
SCHED_COOP_001
SYNC_SEM_FAST_001
SYNC_MUTEX_FAST_001
SYNC_MUTEX_OWNERSHIP_001
IPC_QUEUE_FAST_001
TIME_SLEEP_001
```

Near-term tests:

```text
SCHED_PREEMPT_001
SYNC_SEM_CONTEND_001
SYNC_MUTEX_CONTEND_001
SYNC_MUTEX_PI_001
SYNC_MUTEX_TIMEOUT_001
IPC_QUEUE_CONTEND_001
IPC_QUEUE_FULL_001
IPC_QUEUE_EMPTY_001
IRQ_WAKE_001
TIME_TIMEOUT_001
STRESS_TIMEOUT_RACE_001
```

---

## Kernel Feature Descriptor

Each kernel port must publish a feature descriptor so results can be interpreted correctly.

Example fields:

```text
kernel name
kernel version
supports dynamic threads
supports thread delete
supports semaphore
supports mutex
supports mutex timeout
supports mutex priority inheritance
supports recursive mutex
supports queue
supports timer
supports fixed memory pool
supports general allocator
supports ISR kernel API
supports stack overflow detection
validates null arguments
validates invalid objects
```

This prevents unfair or misleading comparisons. If a kernel does not support a primitive or behavior, the test must report `UNSUPPORTED`. KVB should not silently emulate missing kernel behavior in the benchmark harness.

---

## Fair Comparison Rules

Published comparative results must disclose the configuration used for every kernel.

Required equivalence where possible:

- same CPU frequency
- same compiler family and version
- same optimization level
- same FPU ABI and FPU context setting
- same tick rate
- same number of test threads
- same queue depth
- same message size
- same stack size unless a kernel requires a larger stack
- same interrupt priority for IRQ tests
- same measurement duration
- same warmup duration

Required disclosure:

- kernel safety checks enabled/disabled
- null pointer validation policy
- stack overflow policy
- mutex priority inheritance setting
- dynamic allocation enabled/disabled
- tickless mode enabled/disabled
- logging backend
- debug assertions enabled/disabled
- MPU/MMU enabled/disabled
- cache enabled/disabled, where applicable

A result should be marked invalid if the comparison is not configured fairly or the measurement source is wrong.

---

## Result Format

KVB produces raw text logs and machine-readable results.

Example text log:

```text
[KVB] BEGIN test_id=SYNC_MUTEX_OWNERSHIP_001 name="mutex unlock by non-owner"
[KVB] META kernel=TaktOS version=0.1.0 cpu=nRF52832 clock_hz=64000000 tick_hz=1000 compiler=gcc-14.2.1 opt=Os
[KVB] PASS expected_error=KVB_ERR_NOT_OWNER observed_error=KVB_ERR_NOT_OWNER
[KVB] METRIC latency_cycles=184
[KVB] END test_id=SYNC_MUTEX_OWNERSHIP_001 result=PASS
```

Example JSON result:

```json
{
  "test_id": "SYNC_MUTEX_OWNERSHIP_001",
  "test_name": "mutex unlock by non-owner",
  "test_group": "SYNC",
  "kernel": {
    "name": "TaktOS",
    "version": "0.1.0"
  },
  "platform": {
    "board": "nrf52832_dk",
    "cpu": "nRF52832",
    "clock_hz": 64000000,
    "tick_hz": 1000
  },
  "result": {
    "state": "PASS",
    "message": "non-owner unlock rejected"
  },
  "metrics": {
    "latency_cycles": 184
  }
}
```

---

## Build and Run

Build integration is currently kernel/platform specific.

At this stage, KVB is intended to be integrated into each kernel's existing example or benchmark project.

Typical integration steps:

1. Add `KVB/include` to the include path.
2. Add `KVB/src/core` sources.
3. Add the selected `KVB/src/tests` sources.
4. Add one kernel port from `KVB/ports/kernels/<kernel>`.
5. Add one platform port from `KVB/ports/platforms/<platform>`.
6. Add the selected kernel config from `KVB/configs/<kernel>`.
7. Call the KVB runner from the kernel-specific startup entry.

Example conceptual source set:

```text
KVB/src/core/*.c
KVB/src/tests/sched/*.c
KVB/src/tests/sync/*.c
KVB/src/tests/ipc/*.c
KVB/src/tests/time/*.c
KVB/ports/kernels/taktos/*.c
KVB/ports/platforms/cortex_m/*.c
```

More complete build examples will be added as the suite stabilizes.

---

## Configuration Profiles

KVB supports named comparison profiles.

### Performance Profile

Measures best-case kernel primitive cost.

Typical settings:

- maximum compiler optimization
- no debug logging in hot paths
- minimal safety checks where all kernels are configured equivalently

### Balanced Profile

Measures realistic production behavior.

Typical settings:

- null pointer checks enabled where supported
- stack overflow checks enabled where supported
- priority inheritance enabled where supported
- release optimization
- no debug logging in hot paths

### Safety Profile

Validates stronger defensive behavior.

Typical settings:

- assertions enabled
- object validation enabled
- stack overflow checks enabled
- memory protection enabled where supported
- invalid API tests enabled

Each published result must identify the profile.

---

## Roadmap

### Phase 1 — Foundation

- core runner
- result model
- TaktOS, FreeRTOS, ThreadX, and Zephyr kernel ports
- Cortex-M platform port
- initial scheduler, semaphore, mutex, queue, and sleep tests

### Phase 2 — Contention and Latency

- contended semaphore test
- contended mutex test
- producer/consumer queue test
- latency histogram
- deterministic start barrier
- ISR-to-thread wake latency test

### Phase 3 — Priority Inversion and Error Behavior

- priority inversion validation
- mutex timeout validation
- null argument tests
- invalid object tests
- queue full/empty tests
- report generator v1

### Phase 4 — Stress and Long-Run QA

- queue churn stress
- multi-priority mutex contention stress
- timeout/release race stress
- invariant checking
- long-run failure snapshot logs

### Phase 5 — Memory and Protection

- fixed pool tests
- allocator tests
- pool exhaustion tests
- stack overflow detection tests
- optional MPU/MMU validation hooks

---

## Contributing

KVB needs contributors in several areas:

- kernel ports
- platform ports
- benchmark tests
- validation tests
- report tooling
- build-system examples
- result review and reproduction
- documentation

Contribution rules:

- keep kernel ports thin
- use native kernel primitives
- do not hide missing behavior
- report unsupported features honestly
- do not log inside hot benchmark loops
- document all configuration assumptions
- include raw logs for result submissions
- prefer correctness over impressive numbers

---


### Zephyr target

Zephyr is handled as a special build-config target instead of one project per
MCU.  The single Zephyr KVB application lives at:

```text
KVB/Targets/Zephyr/KVB_ZephyrSuite/
```

Build it by selecting the board/MCU through `west`:

```sh
west build -b stm32f0308_disco KVB/Targets/Zephyr/KVB_ZephyrSuite
west build -b nrf52dk/nrf52832 KVB/Targets/Zephyr/KVB_ZephyrSuite
west build -b nrf54l15dk/nrf54l15/cpuapp KVB/Targets/Zephyr/KVB_ZephyrSuite
```

The shared Zephyr configuration is:

```text
KVB/Targets/Zephyr/common/prj.conf
```


## License

License to be finalized before public release.

Recommended license: MIT, Apache-2.0, or BSD-3-Clause.

The license should allow broad industry adoption, including use by commercial embedded vendors, consultants, silicon vendors, and OS projects.

---

## Project Position

KVB is not a vendor benchmark and not a single-kernel demo. It is a kernel validation benchmark.

Its purpose is to make kernel evaluation open, reproducible, and technically meaningful.

**KVB is the standard for proving both kernel correctness and kernel performance.**
