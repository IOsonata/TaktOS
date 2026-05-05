# Kernel Validation Benchmark (KVB) - Engineering Design Document

## 1. Purpose

Kernel Validation Benchmark (KVB) is an open, reproducible kernel performance benchmark and validation test suite. It has a dual purpose:

1. **Performance Benchmark** - measure kernel speed, latency, jitter, throughput, scalability, and deterministic timing behavior using reproducible workloads.
2. **Validation Test Suite** - verify that kernel behavior is correct under defined scheduling, synchronization, IPC, interrupt, timing, memory, error-handling, and stress conditions.

KVB is not limited to RTOS kernels. It is designed to support embedded RTOSes, microkernels, monolithic kernels, safety kernels, research kernels, and future desktop/server-class kernels.

KVB must never treat performance and correctness as separate projects. Every performance result must be tied to validation status, and every validation test may optionally expose timing, latency, jitter, or throughput metrics.

A kernel that is fast but incorrect must fail validation. A kernel that is correct but slow must still pass validation while showing its performance limits clearly. KVB must make both outcomes visible.

---

## 2. Scope

### 2.1 In Scope

KVB validates and measures:

- scheduler behavior
- cooperative yield behavior
- preemptive scheduling behavior
- priority scheduling behavior
- context switch cost
- thread creation/deletion, where supported
- thread sleep/wakeup timing
- timeout behavior
- semaphore behavior
- mutex behavior
- mutex ownership rules
- priority inversion and priority inheritance
- queue/message-passing behavior
- event flags/event groups, where supported
- interrupt-to-thread wakeup behavior
- timer behavior
- memory pool behavior
- allocator behavior, where supported
- stack overflow detection, where supported
- invalid API/error path behavior
- long-duration stress behavior
- deterministic behavior under load

### 2.2 Out of Scope for Initial Release

The first release does not need to validate:

- full POSIX compliance
- file systems
- networking stacks
- virtual memory paging policy
- userspace process ABI
- SMP scheduling
- power management
- device driver correctness beyond the small platform support layer required by KVB

These can be added later as optional modules.

---

## 3. Design Principles

### 3.1 Dual-Purpose Test Design

Every KVB test should be designed as either:

1. a **benchmark test** with mandatory correctness guards, or
2. a **validation test** with optional performance metrics.

The suite must serve both purposes at all times. A benchmark without correctness checks is incomplete. A validation test that can expose useful timing, latency, jitter, or throughput information should report those metrics.

### 3.2 Validation-Gated Benchmarking

Every performance benchmark must define expected behavior. The benchmark result is only publishable if the kernel passes the correctness criteria.

Example:

```text
SYNC_MUTEX_OWNERSHIP_001
Expected: A thread that does not own a mutex must not successfully unlock it.
Result: PASS / FAIL / UNSUPPORTED
Metric: optional error-path latency
```

### 3.3 Reproducibility

Every published result must include:

- kernel name and version
- target MCU/CPU
- board name
- clock frequency
- tick rate
- compiler name and version
- compiler flags
- optimization level
- FPU setting
- stack sizes
- heap/pool sizes
- queue sizes
- thread count
- priority layout
- enabled/disabled kernel safety checks
- raw log
- generated report
- source revision

No result is official without reproducibility metadata.

### 3.4 No Hidden Harness Logic

KVB must not hide work inside the abstraction layer. The kernel port layer must be thin and explicit. If a kernel primitive is unavailable, the test must report `UNSUPPORTED`, not emulate the primitive in a way that changes the measurement.

### 3.5 Comparable Defaults

Each kernel should be configured with equivalent functional behavior where possible.

Examples:

- same tick rate
- same number of test threads
- same priority relationships
- same stack size per test role
- same queue depth
- same message size
- same timeout duration
- same compiler optimization level
- same FPU context policy, where applicable
- comparable null pointer / invalid parameter checking policy
- comparable stack overflow checking policy, where available

### 3.6 Open by Default

The suite, ports, scripts, raw logs, report generator, and published result files must be open. Anyone should be able to rebuild the test and reproduce the result.

### 3.7 Neutral Test Identity

KVB is not branded as a TaktOS benchmark. TaktOS is one kernel under test. Other supported kernels should be treated as first-class ports.

Initial kernel targets:

- TaktOS
- FreeRTOS
- ThreadX
- Zephyr

---

## 4. Architecture Overview

KVB is organized into four layers:

```text
+-------------------------------------------------------------+
|                    KVB Test Modules                         |
|  SCHED / SYNC / IPC / IRQ / TIME / MEM / ERR / STRESS       |
+-------------------------------------------------------------+
|                    KVB Core Harness                         |
|  test registry, assertions, timing, logging, result model    |
+-------------------------------------------------------------+
|                    Kernel Port Layer                        |
|  threads, mutexes, semaphores, queues, timers, IRQ hooks     |
+-------------------------------------------------------------+
|                    Platform Layer                           |
|  clock, cycle counter, UART/log output, test trigger, board  |
+-------------------------------------------------------------+
```

### 4.1 KVB Test Modules

Test modules contain the actual validation and benchmark logic. They must use only the KVB core API and kernel port API.

### 4.2 KVB Core Harness

The core harness provides:

- test registration
- test selection
- assertion macros
- pass/fail accounting
- metric collection
- timing utilities
- log formatting
- result serialization
- benchmark window control
- deterministic start barriers
- test cleanup checks

### 4.3 Kernel Port Layer

The kernel port layer maps KVB primitives to each kernel under test.

It must expose only the primitives needed by KVB. It must not hide missing kernel behavior.

### 4.4 Platform Layer

The platform layer provides board/CPU support:

- monotonic cycle counter
- wall-clock/tick reference
- UART or semihosting output
- interrupt trigger source
- optional GPIO pulse output for external logic analyzer measurements
- clock-frequency metadata
- CPU/board metadata

---

## 5. Repository Layout

Recommended initial layout:

```text
kernel-validation-benchmark/
├── README.md
├── LICENSE
├── docs/
│   ├── design.md
│   ├── porting-guide.md
│   ├── result-format.md
│   ├── test-catalog.md
│   └── publishing-results.md
├── kvb/
│   ├── core/
│   │   ├── kvb_assert.h
│   │   ├── kvb_config.h
│   │   ├── kvb_log.h
│   │   ├── kvb_metrics.h
│   │   ├── kvb_result.h
│   │   ├── kvb_runner.h
│   │   └── kvb_time.h
│   ├── include/
│   │   ├── kvb.h
│   │   ├── kvb_kernel_port.h
│   │   └── kvb_platform_port.h
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
│       ├── nrf52832/
│       ├── nrf54l15/
│       └── stm32l4/
├── boards/
│   ├── nrf52832_dk/
│   ├── nrf54l15_dk/
│   └── stm32l475_iot01a/
├── configs/
│   ├── common/
│   ├── taktos/
│   ├── freertos/
│   ├── threadx/
│   └── zephyr/
├── tools/
│   ├── parse_log.py
│   ├── generate_report.py
│   ├── compare_results.py
│   └── validate_metadata.py
├── results/
│   ├── raw/
│   ├── parsed/
│   └── reports/
└── examples/
    └── minimal-port/
```

---

## 6. Test Result Model

Every test produces a structured result. The result model must represent both sides of KVB:

1. **validation state** - whether the kernel behavior was correct
2. **benchmark metrics** - how fast, deterministic, or scalable the behavior was

A benchmark metric without validation state is invalid. A validation result without metrics is allowed when the test is purely functional or safety-oriented.

### 6.1 Result States

```text
PASS        Test behavior matched the expected result.
FAIL        Test behavior violated the expected result.
UNSUPPORTED Kernel does not provide the required primitive or behavior.
SKIPPED     Test was intentionally not run due to configuration.
INVALID     Test result is unusable due to harness/configuration error.
```

### 6.2 Metric Types

A test may report one or more metrics:

```text
throughput_ops_per_sec
throughput_ops_per_window
latency_cycles_min
latency_cycles_avg
latency_cycles_max
latency_us_min
latency_us_avg
latency_us_max
jitter_us
p50_us
p90_us
p99_us
p999_us
timeout_error_us
inversion_bound_us
memory_bytes
stack_bytes_used
iterations
failure_count
```

### 6.3 Required Per-Test Fields

```text
test_id
test_name
test_group
kernel_name
kernel_version
platform
board
cpu
clock_hz
tick_hz
compiler
compiler_version
compiler_flags
optimization_level
result_state
validation_message
metrics
raw_log_offset
source_revision
configuration_hash
```

### 6.4 Example Text Log

```text
[KVB] BEGIN test_id=SYNC_MUTEX_OWNERSHIP_001 name="mutex unlock by non-owner"
[KVB] META kernel=TaktOS version=0.1.0 cpu=nRF52832 clock_hz=64000000 tick_hz=1000 compiler=gcc-14.2.1 opt=Os
[KVB] PASS expected_error=KVB_ERR_NOT_OWNER observed_error=KVB_ERR_NOT_OWNER
[KVB] METRIC latency_cycles=184
[KVB] END test_id=SYNC_MUTEX_OWNERSHIP_001 result=PASS
```

### 6.5 Example JSON Result

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
  "toolchain": {
    "compiler": "arm-none-eabi-gcc",
    "version": "14.2.1",
    "flags": "-Os -ffunction-sections -fdata-sections"
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

## 7. Kernel Port API

The kernel port API must be intentionally small. Each kernel implementation must provide this API using native kernel primitives.

### 7.1 Common Types

```c
typedef struct KvbThread KvbThread;
typedef struct KvbSemaphore KvbSemaphore;
typedef struct KvbMutex KvbMutex;
typedef struct KvbQueue KvbQueue;
typedef struct KvbTimer KvbTimer;

typedef enum {
    KVB_OK = 0,
    KVB_ERR_TIMEOUT,
    KVB_ERR_INVALID_ARG,
    KVB_ERR_NOT_OWNER,
    KVB_ERR_WOULD_BLOCK,
    KVB_ERR_UNSUPPORTED,
    KVB_ERR_KERNEL
} KvbStatus;
```

### 7.2 Thread API

```c
KvbStatus kvb_thread_create(
    KvbThread *thread,
    const char *name,
    void (*entry)(void *arg),
    void *arg,
    void *stack_mem,
    size_t stack_size,
    int priority);

KvbStatus kvb_thread_start(KvbThread *thread);
KvbStatus kvb_thread_sleep_ticks(uint32_t ticks);
KvbStatus kvb_thread_yield(void);
KvbStatus kvb_thread_suspend(KvbThread *thread);
KvbStatus kvb_thread_resume(KvbThread *thread);
KvbStatus kvb_thread_join(KvbThread *thread, uint32_t timeout_ticks);
KvbStatus kvb_thread_delete(KvbThread *thread);
```

Notes:

- If a kernel does not support dynamic thread creation, the port may statically bind test threads.
- The test metadata must state whether thread objects are static or dynamic.
- If `join` is unsupported, tests requiring join must use a completion semaphore or return `UNSUPPORTED`.

### 7.3 Semaphore API

```c
KvbStatus kvb_sem_create(KvbSemaphore *sem, uint32_t initial_count, uint32_t max_count);
KvbStatus kvb_sem_wait(KvbSemaphore *sem, uint32_t timeout_ticks);
KvbStatus kvb_sem_post(KvbSemaphore *sem);
KvbStatus kvb_sem_delete(KvbSemaphore *sem);
```

### 7.4 Mutex API

```c
KvbStatus kvb_mutex_create(KvbMutex *mutex, uint32_t flags);
KvbStatus kvb_mutex_lock(KvbMutex *mutex, uint32_t timeout_ticks);
KvbStatus kvb_mutex_unlock(KvbMutex *mutex);
KvbStatus kvb_mutex_delete(KvbMutex *mutex);
```

Mutex flags:

```c
#define KVB_MUTEX_NORMAL       0x00000000u
#define KVB_MUTEX_RECURSIVE    0x00000001u
#define KVB_MUTEX_PI_REQUIRED  0x00000002u
```

Notes:

- If priority inheritance is optional, the port must expose whether it is enabled.
- If recursive mutex is unsupported, recursive tests return `UNSUPPORTED`.
- Unlock by non-owner must return an error if the kernel supports ownership validation.
- If the kernel intentionally does not validate ownership, the result should be `FAIL` for ownership validation tests unless the test is marked as compatibility-only.
- KVB validation tests should not silently accept unsafe behavior.

### 7.5 Queue API

```c
KvbStatus kvb_queue_create(
    KvbQueue *queue,
    void *storage,
    size_t message_size,
    uint32_t message_count);

KvbStatus kvb_queue_send(KvbQueue *queue, const void *msg, uint32_t timeout_ticks);
KvbStatus kvb_queue_recv(KvbQueue *queue, void *msg, uint32_t timeout_ticks);
KvbStatus kvb_queue_delete(KvbQueue *queue);
```

### 7.6 Timer API

```c
KvbStatus kvb_timer_create(
    KvbTimer *timer,
    void (*callback)(void *arg),
    void *arg,
    uint32_t period_ticks,
    bool periodic);

KvbStatus kvb_timer_start(KvbTimer *timer);
KvbStatus kvb_timer_stop(KvbTimer *timer);
KvbStatus kvb_timer_delete(KvbTimer *timer);
```

### 7.7 Interrupt API

```c
typedef void (*KvbPlatformIrqHandler)(void *arg);

KvbStatus kvb_platform_irq_probe_init(KvbPlatformIrqHandler handler, void *arg);
KvbStatus kvb_platform_irq_probe_trigger(void);
KvbStatus kvb_platform_irq_probe_disable(void);
const char *kvb_platform_irq_probe_name(void);
```

Notes:

- On Cortex-M, this can use a software interrupt, EGU/SWI, PendSV variant, or a timer interrupt depending on platform.
- The platform must document the interrupt priority used.
- ISR-to-thread tests must state whether the interrupt priority is allowed to call kernel APIs.

---

## 8. Platform Port API

### 8.1 Time and Cycle Counter

```c
uint64_t kvb_platform_cycle_count(void);
uint32_t kvb_platform_cycle_frequency_hz(void);
uint64_t kvb_platform_time_us(void);
```

Requirements:

- Cycle counter must be monotonic during the test window.
- If the hardware cycle counter wraps, the platform layer must extend it or document the maximum safe measurement window.
- On Cortex-M, DWT CYCCNT is preferred when available.
- DWT/CYCCNT must not be assumed on ARMv6-M targets such as Cortex-M0, Cortex-M0+, and STM32F03xx. Those targets require a hardware-timer fallback for `kvb_platform_time_us()`, or timing-dependent tests must report `INVALID`.

### 8.2 Logging

```c
void kvb_platform_log_write(const char *data, size_t len);
void kvb_platform_log_flush(void);
```

Requirements:

- Logging must not occur inside the hot measurement loop unless the test explicitly measures logging.
- Logs should be buffered where possible.

### 8.3 Board Metadata

```c
const char *kvb_platform_board_name(void);
const char *kvb_platform_cpu_name(void);
const char *kvb_platform_compiler_name(void);
const char *kvb_platform_compiler_version(void);
```

---

## 9. Test Groups

### 9.1 SCHED - Scheduler Tests

#### SCHED_COOP_001 - Cooperative Yield Throughput

Purpose:

Measure cooperative yield cost across a fixed set of same-priority threads.

Setup:

- create N worker threads
- same priority
- each thread increments its own counter
- each thread yields after increment
- run for fixed duration

Validation:

- every thread must run
- no thread counter may remain zero
- fairness ratio must stay within configured threshold unless kernel does not guarantee fairness

Metrics:

- total yields per second
- per-thread counters
- fairness min/max ratio

#### SCHED_PREEMPT_001 - Preemptive Scheduling Throughput

Purpose:

Measure scheduler cost when higher-priority threads wake and preempt lower-priority threads.

Setup:

- N threads across defined priorities
- controlled wake chain
- each wake causes a context switch

Validation:

- execution order must match expected priority order
- no missed wakeups

Metrics:

- switches per second
- average cycles per switch

#### SCHED_PRIORITY_001 - Priority Ordering

Purpose:

Validate that the highest ready priority runs before lower priorities.

Validation:

- ready high-priority thread must run before ready lower-priority thread
- failure is correctness failure, not performance degradation

---

### 9.2 SYNC - Synchronization Tests

#### SYNC_SEM_FAST_001 - Uncontended Semaphore Wait/Post

Purpose:

Measure semaphore fast path.

Validation:

- wait succeeds when count is available
- post restores count

Metrics:

- wait/post pairs per second
- cycles per pair

#### SYNC_SEM_CONTEND_001 - Contended Semaphore Wake

Purpose:

Validate and measure semaphore wakeup behavior when one or more threads are blocked.

Setup:

- consumer thread blocks on semaphore
- producer releases semaphore

Validation:

- blocked thread wakes
- no lost wakeup
- wake order is checked if kernel documents ordering

Metrics:

- post-to-thread-run latency
- wake jitter

#### SYNC_MUTEX_FAST_001 - Uncontended Mutex Lock/Unlock

Purpose:

Measure mutex fast path.

Validation:

- lock succeeds
- unlock succeeds
- ownership state is correct

Metrics:

- lock/unlock pairs per second
- cycles per pair

#### SYNC_MUTEX_CONTEND_001 - Contended Mutex Transfer

Purpose:

Measure mutex behavior under real contention.

Setup:

- owner thread locks mutex
- waiter thread blocks on mutex
- owner releases mutex

Validation:

- waiter blocks while owner holds mutex
- waiter acquires mutex after release
- no double ownership

Metrics:

- unlock-to-waiter-run latency
- acquisition latency

#### SYNC_MUTEX_OWNERSHIP_001 - Unlock By Non-Owner

Purpose:

Validate mutex ownership enforcement.

Setup:

- thread A locks mutex
- thread B attempts unlock

Expected:

- thread B must not successfully unlock the mutex

Result:

- `PASS` if non-owner unlock is rejected
- `FAIL` if non-owner unlock succeeds or silently corrupts ownership
- `UNSUPPORTED` only if the kernel has no mutex primitive

#### SYNC_MUTEX_PI_001 - Priority Inversion Bound

Purpose:

Validate priority inheritance or equivalent priority inversion mitigation.

Setup:

- low-priority thread locks mutex
- medium-priority CPU-bound thread becomes ready
- high-priority thread blocks on mutex held by low-priority thread

Expected:

- low-priority owner must be allowed to run and release mutex despite medium-priority pressure
- high-priority thread must acquire mutex within defined bound

Metrics:

- inversion duration in cycles/us
- high-priority block duration
- low-priority owner boost observed, where observable

Result:

- `PASS` if bounded inversion behavior matches kernel configuration
- `FAIL` if medium-priority thread can indefinitely delay release
- `UNSUPPORTED` if kernel explicitly lacks mutex priority inheritance and the test is configured to require PI

#### SYNC_MUTEX_TIMEOUT_001 - Mutex Timeout

Purpose:

Validate timeout behavior while waiting on a locked mutex.

Expected:

- wait must return timeout at expected time
- mutex ownership must remain unchanged

Metrics:

- timeout error latency
- timeout jitter

#### SYNC_MUTEX_DELETE_WAITERS_001 - Delete Mutex With Waiters

Purpose:

Validate behavior when a mutex is deleted while threads are waiting.

Expected:

- behavior must match documented kernel semantics
- waiting threads must not remain permanently blocked
- no memory corruption

Result:

- `PASS`, `FAIL`, or `UNSUPPORTED` depending on kernel capability and documented behavior

---

### 9.3 IPC - Message Passing Tests

#### IPC_QUEUE_FAST_001 - Queue Send/Receive Same Thread

Purpose:

Measure queue fast path without blocking.

Validation:

- sent message equals received message
- FIFO order preserved for multiple messages

Metrics:

- send/receive pairs per second
- cycles per pair

#### IPC_QUEUE_CONTEND_001 - Producer/Consumer Queue

Purpose:

Measure queue throughput between two threads.

Setup:

- producer thread sends fixed-size messages
- consumer thread receives messages
- optional blocking on full/empty

Validation:

- no lost messages
- no duplicate messages
- sequence numbers strictly ordered

Metrics:

- messages per second
- producer block count
- consumer block count
- latency distribution if timestamped messages are enabled

#### IPC_QUEUE_FULL_001 - Queue Full Behavior

Purpose:

Validate behavior when sending to a full queue.

Expected:

- nonblocking send returns would-block/full error
- timed send returns timeout if no space appears
- queue contents remain intact

#### IPC_QUEUE_EMPTY_001 - Queue Empty Behavior

Purpose:

Validate behavior when receiving from an empty queue.

Expected:

- nonblocking receive returns would-block/empty error
- timed receive returns timeout if no message appears

---

### 9.4 IRQ - Interrupt Interaction Tests

#### RT_IRQ_MASK_001 - IRQ Response and ISR-to-Thread Latency

Purpose:

Measure latency from interrupt event to awakened thread executing.

Setup:

- thread blocks on semaphore/queue/event
- interrupt handler posts semaphore/queue/event
- awakened thread records timestamp

Validation:

- exactly one wake per interrupt
- no missed events

Metrics:

- trigger-to-IRQ entry latency min/avg/max
- IRQ-entry-to-thread-run latency min/avg/max
- trigger-to-thread-run latency min/avg/max

#### IRQ_API_LEGALITY_001 - ISR API Rule Validation

Purpose:

Validate that illegal APIs from ISR context are rejected or documented.

Expected:

- blocking APIs must not block inside ISR
- illegal ISR calls must fail safely where kernel supports validation

---

### 9.5 TIME - Timing Tests

#### TIME_SLEEP_001 - Sleep Duration Accuracy

Purpose:

Validate that sleep duration is not shorter than requested and within tolerance.

Metrics:

- requested ticks/us
- actual ticks/us
- early wake count
- late wake jitter

#### TIME_TIMEOUT_001 - Timeout Accuracy

Purpose:

Validate timeout behavior for blocking primitives.

Primitives:

- semaphore wait timeout
- mutex lock timeout
- queue receive timeout
- queue send timeout

#### TIME_TIMER_001 - One-Shot Timer Accuracy

Purpose:

Validate one-shot timer expiry time.

Metrics:

- expiry latency
- jitter

#### TIME_TIMER_PERIODIC_001 - Periodic Timer Jitter

Purpose:

Measure periodic timer stability.

Metrics:

- min/avg/max period
- p99 jitter
- missed periods

---

### 9.6 MEM - Memory Tests

#### MEM_POOL_FAST_001 - Fixed Block Allocate/Free

Purpose:

Measure fixed-block memory pool allocate/free cost.

Validation:

- returned block is valid
- freed block can be reused
- no duplicate allocation of same block while still allocated

#### MEM_POOL_EXHAUST_001 - Pool Exhaustion

Purpose:

Validate behavior when memory pool is exhausted.

Expected:

- allocation fails safely
- no memory corruption
- later free restores availability

#### MEM_ALLOC_FAST_001 - General Allocator Allocate/Free

Purpose:

Measure general allocator behavior where supported.

Result:

- `UNSUPPORTED` for kernels that intentionally do not provide a general allocator

#### MEM_STACK_GUARD_001 - Stack Overflow Detection

Purpose:

Validate stack overflow detection where supported.

Expected:

- overflow must be detected by configured mechanism
- test must be isolated because it may intentionally fault

Notes:

- This test may require a special build or fault-capture mode.
- Result may be `UNSUPPORTED` if the kernel or platform has no stack overflow detection.

---

### 9.7 ERR - Error Behavior Tests

#### ERR_NULL_ARG_001 - Null Argument Validation

Purpose:

Validate null pointer handling for kernel APIs.

Expected:

- kernel returns error, asserts, or traps according to documented configuration
- behavior must be documented in metadata

#### ERR_INVALID_OBJECT_001 - Invalid Object Handle

Purpose:

Validate behavior when an invalid kernel object is passed.

Expected:

- reject safely, assert, or document unsupported validation

#### ERR_DOUBLE_DELETE_001 - Double Delete

Purpose:

Validate behavior when an object is deleted twice.

Expected:

- safe failure, assert, or documented behavior

---

### 9.8 STRESS - Stress Tests

#### STRESS_QUEUE_CHURN_001 - Long-Run Queue Churn

Purpose:

Expose lost messages, race conditions, and long-run instability.

Setup:

- multiple producers
- multiple consumers
- sequence-numbered messages
- long duration

Validation:

- no lost messages
- no duplicate messages
- no corrupted payloads
- no deadlock

#### STRESS_MUTEX_CONTENTION_001 - Multi-Priority Mutex Contention

Purpose:

Expose mutex ownership, fairness, timeout, and priority inversion issues.

Setup:

- multiple threads of different priorities contend for shared mutex
- each critical section updates invariant-protected state

Validation:

- invariant never violated
- no double ownership
- no starvation beyond configured bound

#### STRESS_TIMEOUT_RACE_001 - Timeout/Release Race

Purpose:

Stress the race boundary where an object is released near timeout expiry.

Validation:

- no thread receives both success and timeout for the same wait
- no lost wakeup
- no corrupted wait list

---

## 10. Timing and Measurement Rules

### 10.1 Measurement Window

Performance tests may use either:

- fixed-duration window, e.g. 30 seconds
- fixed-iteration loop, e.g. 1,000,000 operations

Each test must state which mode it uses.

### 10.2 Warmup

Each performance test should support a warmup phase.

```text
warmup_ms = 1000
measurement_ms = 30000
```

Warmup counters must not be included in final metrics.

### 10.3 Logging Exclusion

Logging must not occur inside hot loops unless the test is explicitly measuring logging.

### 10.4 Counter Width

All operation counters should be at least 64-bit.

### 10.5 Tail Latency

Latency tests should report more than average.

Required latency fields where practical:

```text
min
avg
max
p50
p90
p99
p999
sample_count
```

---

## 11. Configuration Model

KVB should use a common configuration header plus kernel-specific configuration files.

### 11.1 Common Configuration

```c
#define KVB_TICK_HZ                  1000u
#define KVB_MEASUREMENT_MS           30000u
#define KVB_WARMUP_MS                1000u
#define KVB_DEFAULT_STACK_SIZE       1024u
#define KVB_QUEUE_DEPTH              16u
#define KVB_QUEUE_MESSAGE_SIZE       16u
#define KVB_WORKER_THREAD_COUNT      5u
#define KVB_ENABLE_LATENCY_HISTOGRAM 1u
#define KVB_ENABLE_STRESS_TESTS      1u
#define KVB_ENABLE_ERROR_TESTS       1u
```

### 11.2 Kernel Feature Descriptor

Each kernel port must publish a feature descriptor:

```c
typedef struct {
    const char *kernel_name;
    const char *kernel_version;
    bool supports_dynamic_threads;
    bool supports_thread_delete;
    bool supports_semaphore;
    bool supports_mutex;
    bool supports_mutex_timeout;
    bool supports_mutex_priority_inheritance;
    bool supports_recursive_mutex;
    bool supports_queue;
    bool supports_timer;
    bool supports_fixed_pool;
    bool supports_general_allocator;
    bool supports_isr_kernel_api;
    bool supports_stack_overflow_detection;
    bool validates_null_args;
    bool validates_invalid_objects;
} KvbKernelFeatures;
```

The feature descriptor is part of the final report.

---

## 12. Fair Comparison Rules

To publish comparative results, the following must be equivalent or explicitly stated.

### 12.1 Required Equivalence

- same CPU frequency
- same compiler family and version where possible
- same optimization level
- same FPU ABI and FPU context setting
- same tick rate
- same number of test threads
- same queue depth
- same message size
- same stack size unless a kernel requires larger stack
- same interrupt priority for IRQ tests
- same measurement duration
- same warmup duration

### 12.2 Required to Be Reported

- kernel safety checks enabled/disabled
- null pointer validation policy
- stack overflow policy
- mutex priority inheritance setting
- dynamic allocation enabled/disabled
- tickless mode enabled/disabled
- logging backend
- debug assertions enabled/disabled
- MPU/MMU enabled/disabled
- cache enabled/disabled, for cacheable CPUs

### 12.3 Invalid Comparison Conditions

A comparative result must be marked invalid if:

- one kernel uses a different CPU clock without normalization and a stated note
- one kernel uses different optimization flags without a stated note
- one kernel has a different test thread count
- one kernel has a different queue depth/message size for queue tests
- one kernel disables safety/error checking while another enables it, unless the report explicitly compares those modes
- logging is inside the hot path for one kernel but not another
- the test duration is not equal
- the measurement timer is wrong or derived from the wrong clock source

---

## 13. Report Format

Reports should have three levels.

### 13.1 Summary Table

```text
Group   Test                         TaktOS   FreeRTOS   ThreadX   Zephyr
SCHED   Cooperative Yield            PASS     PASS       PASS      PASS
SYNC    Mutex Ownership              PASS     PASS*      PASS      PASS
SYNC    Priority Inheritance         PASS     PASS       PASS      PASS
IPC     Queue FIFO Correctness       PASS     PASS       PASS      PASS
IRQ     ISR-to-Thread Wake           PASS     PASS       PASS      PASS
TIME    Sleep Accuracy               PASS     PASS       PASS      PASS
STRESS  Timeout/Release Race         PASS     PASS       PASS      PASS
```

Footnotes must explain configuration-dependent behavior.

### 13.2 Performance Table

```text
Test                         Metric                 TaktOS   FreeRTOS   ThreadX   Zephyr
SCHED_COOP_001               yields/sec             ...      ...        ...       ...
SYNC_MUTEX_FAST_001          lock+unlock/sec        ...      ...        ...       ...
IRQ_WAKE_001                 p99 latency us         ...      ...        ...       ...
IPC_QUEUE_CONTEND_001        messages/sec           ...      ...        ...       ...
TIME_TIMER_PERIODIC_001      p99 jitter us          ...      ...        ...       ...
```

### 13.3 Full Validation Log

Every report must link to raw logs and machine-readable JSON.

---

## 14. Implementation Plan

### Phase 1 - Core Harness and Minimal Ports

Deliverables:

- core test runner
- assertion system
- log format
- result model
- common config
- platform cycle counter
- UART log output
- TaktOS port
- FreeRTOS port
- ThreadX port
- Zephyr port

Initial tests:

- SCHED_COOP_001
- SCHED_PREEMPT_001
- SYNC_SEM_FAST_001
- SYNC_MUTEX_FAST_001
- SYNC_MUTEX_OWNERSHIP_001
- IPC_QUEUE_FAST_001
- TIME_SLEEP_001

### Phase 2 - Contention and Latency

Deliverables:

- latency histogram support
- deterministic start barrier
- IRQ test source
- ISR-to-thread latency tests
- contended semaphore test
- contended mutex test
- producer/consumer queue test

Initial tests:

- SYNC_SEM_CONTEND_001
- SYNC_MUTEX_CONTEND_001
- IPC_QUEUE_CONTEND_001
- IRQ_WAKE_001
- TIME_TIMEOUT_001

### Phase 3 - Priority Inversion and Error Behavior

Deliverables:

- priority inversion harness
- error behavior tests
- metadata reporting for safety settings
- report generator v1

Initial tests:

- SYNC_MUTEX_PI_001
- SYNC_MUTEX_TIMEOUT_001
- ERR_NULL_ARG_001
- ERR_INVALID_OBJECT_001
- IPC_QUEUE_FULL_001
- IPC_QUEUE_EMPTY_001

### Phase 4 - Stress and Long-Run QA

Deliverables:

- stress runner
- randomized test sequencing
- invariant checking
- long-run result format
- failure snapshot log

Initial tests:

- STRESS_QUEUE_CHURN_001
- STRESS_MUTEX_CONTENTION_001
- STRESS_TIMEOUT_RACE_001

### Phase 5 - Memory and Protection

Deliverables:

- memory pool tests
- allocator tests
- stack overflow detection tests
- optional MPU/MMU validation hooks
- Partitura integration path

Initial tests:

- MEM_POOL_FAST_001
- MEM_POOL_EXHAUST_001
- MEM_ALLOC_FAST_001
- MEM_STACK_GUARD_001

---

## 15. Coding Rules

### 15.1 Test Code

- Tests must be deterministic where possible.
- Tests must not depend on undefined scheduling behavior unless the result explicitly validates that behavior.
- Hot loops must avoid logging.
- Shared counters must use correct atomic or kernel-protected access.
- Each test must clean up or reset all objects it creates.
- Each test must produce a PASS/FAIL/UNSUPPORTED/SKIPPED/INVALID result.

### 15.2 Port Code

- Port code must use native kernel primitives.
- Port code must not emulate missing kernel primitives unless the test explicitly permits emulation.
- Port code must not hide kernel errors.
- Port code must document every configuration assumption.
- Port code must expose kernel feature flags accurately.

### 15.3 Platform Code

- Platform code must document clock source.
- Platform code must document interrupt source.
- Platform code must document cycle counter behavior and wraparound.
- Platform code must avoid changing test semantics.

---

## 16. Naming Convention

Test IDs use this format:

```text
<GROUP>_<FEATURE>_<NUMBER>
```

Examples:

```text
SCHED_COOP_001
SCHED_PREEMPT_001
SYNC_MUTEX_OWNERSHIP_001
SYNC_MUTEX_PI_001
IPC_QUEUE_CONTEND_001
IRQ_WAKE_001
TIME_TIMEOUT_001
MEM_POOL_EXHAUST_001
ERR_NULL_ARG_001
STRESS_TIMEOUT_RACE_001
```

Groups:

```text
SCHED   Scheduler behavior
SYNC    Synchronization primitives
IPC     Message passing and inter-thread communication
IRQ     Interrupt/kernel interaction
TIME    Sleep, timeout, and timer behavior
MEM     Memory allocation, pools, protection, and stack behavior
ERR     Invalid API and safety behavior
DET     Determinism and jitter-focused tests
STRESS  Long-run stress and race exposure
```

---

## 17. Minimal First Target

The first useful public KVB release should support:

- nRF52832 or nRF54L15 target
- GCC toolchain
- TaktOS
- FreeRTOS
- ThreadX
- Zephyr
- common UART log output
- identical test configuration across kernels where possible
- generated JSON report
- generated Markdown report

Minimum first test list:

```text
SCHED_COOP_001
SCHED_PREEMPT_001
SYNC_SEM_FAST_001
SYNC_SEM_CONTEND_001
SYNC_MUTEX_FAST_001
SYNC_MUTEX_CONTEND_001
SYNC_MUTEX_OWNERSHIP_001
SYNC_MUTEX_PI_001
IPC_QUEUE_FAST_001
IPC_QUEUE_CONTEND_001
IPC_QUEUE_FULL_001
IPC_QUEUE_EMPTY_001
IRQ_WAKE_001
TIME_SLEEP_001
TIME_TIMEOUT_001
STRESS_TIMEOUT_RACE_001
```

This first release is enough to prove that KVB is not merely a throughput loop benchmark. It validates correctness, contention, priority inversion, queue behavior, ISR wakeup, timing, and race behavior.

---

## 18. Engineering Risks

### 18.1 Kernel Feature Mismatch

Different kernels expose different APIs and semantics.

Mitigation:

- use explicit feature descriptor
- use `UNSUPPORTED` when the kernel does not provide the feature
- do not emulate missing behavior silently
- document configuration-dependent behavior

### 18.2 Timing Source Errors

Incorrect CPU clock or timer source can invalidate results.

Mitigation:

- require clock metadata
- validate measurement duration using an independent tick or hardware timer
- support GPIO pulse validation with external tools

### 18.3 Hidden Logging Cost

UART output can dominate runtime.

Mitigation:

- no logging inside hot loops
- buffer logs
- emit only final test summaries during benchmark windows

### 18.4 Unfair Kernel Configuration

Different safety settings can distort comparisons.

Mitigation:

- list all settings
- require comparable safety profiles
- support named profiles such as `performance`, `balanced`, and `safety`

### 18.5 Test Harness Bias

A poorly designed abstraction can favor one kernel.

Mitigation:

- keep port layer thin
- review generated assembly for hot tests
- publish all source
- allow external contributors to review and improve ports

---

## 19. Recommended Configuration Profiles

### 19.1 Performance Profile

Purpose:

Measure best-case kernel primitive cost.

Typical settings:

- assertions disabled where safe
- stack overflow checks disabled or minimal
- null pointer checks disabled only if all kernels are configured equivalently
- maximum compiler optimization
- no debug logging in hot path

### 19.2 Balanced Profile

Purpose:

Measure realistic production configuration.

Typical settings:

- null pointer checks enabled where supported
- stack overflow checks enabled where supported
- priority inheritance enabled where supported
- release optimization
- no debug logging in hot path

### 19.3 Safety Profile

Purpose:

Validate stronger defensive behavior.

Typical settings:

- assertions enabled
- object validation enabled
- stack overflow checks enabled
- memory protection enabled where supported
- invalid API tests enabled

Each published result must identify the profile.

---

## 20. Definition of Done

A KVB test is complete when:

- test purpose is documented
- expected behavior is documented
- pass/fail criteria are implemented
- metrics are defined
- raw log output is implemented
- JSON result output is implemented
- test runs on at least one kernel port
- unsupported behavior is handled explicitly
- test does not log inside hot loop
- test cleanup is verified
- result is reproducible from source

A KVB kernel port is complete when:

- feature descriptor is implemented
- all required port APIs are implemented or explicitly unsupported
- kernel configuration is documented
- metadata output is complete
- at least the minimal first-release tests run
- raw logs and parsed reports can be generated

A KVB public result is complete when:

- raw logs are published
- parsed JSON is published
- generated report is published
- source revision is identified
- toolchain version is identified
- kernel configuration files are published
- board/platform details are published
- invalid or unsupported tests are clearly marked

---

## 21. Summary

KVB is both a performance benchmark and a validation test suite. Its value comes from combining reproducible performance measurements with correctness tests, latency measurements, deterministic behavior checks, and stress tests.

KVB must answer two questions for every kernel:

1. **Does it behave correctly?**
2. **How well does it perform while behaving correctly?**

The first implementation should focus on the areas most obviously missing from primitive-loop benchmarks:

- mutex ownership
- mutex contention
- priority inversion
- queue correctness
- timeout correctness
- ISR-to-thread wake latency
- stress/race behavior

Once these are implemented across TaktOS, FreeRTOS, ThreadX, and Zephyr, KVB can credibly become the open standard for kernel evaluation.
