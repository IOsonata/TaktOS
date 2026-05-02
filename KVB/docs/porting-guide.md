# KVB Porting Guide

## Porting model

A KVB port has two parts:

1. **Kernel port** - maps KVB primitives to native kernel APIs.
2. **Platform port** - provides cycle count, timing, logging, and board metadata.

The kernel port must stay thin. It must not emulate missing primitives unless the test explicitly allows it. If a kernel feature is missing, the port reports `KVB_ERR_UNSUPPORTED` from the relevant API and `false` in the matching field of `KvbKernelFeatures`.

## Required kernel port functions

Kernel ports implement the API declared in:

```text
include/kvb_kernel_port.h
```

The first required primitive set is:

- kernel start
- thread create / yield / sleep / delete
- semaphore create / wait / post / delete
- mutex create / lock / unlock / delete
- queue create / send / receive / delete
- feature descriptor

## Priority

KVB exposes five canonical priority levels:

```c
typedef enum {
    KVB_PRIORITY_LOWEST  = 0,
    KVB_PRIORITY_LOW     = 1,
    KVB_PRIORITY_NORMAL  = 2,
    KVB_PRIORITY_HIGH    = 3,
    KVB_PRIORITY_HIGHEST = 4
} KvbPriority;
```

Each kernel port maps these onto its own native priority space. Tests use the canonical values directly and never assume any particular numeric encoding. The runner thread runs at `KVB_PRIORITY_HIGHEST`; `LOWEST` stays above each kernel's idle/system thread; the three middle levels (`LOW`/`NORMAL`/`HIGH`) are used by priority-inheritance and preemption tests.

A port maps `KvbPriority` in a small table or switch — no math sprinkled across other functions:

```c
static UBaseType_t kvb_freertos_priority(KvbPriority p) {
    const UBaseType_t max = configMAX_PRIORITIES;
    switch (p) {
        case KVB_PRIORITY_LOWEST:  return 1;
        case KVB_PRIORITY_LOW:     return 1 + (max - 2) / 4;
        case KVB_PRIORITY_NORMAL:  return 1 + (max - 2) / 2;
        case KVB_PRIORITY_HIGH:    return 1 + 3 * (max - 2) / 4;
        case KVB_PRIORITY_HIGHEST: return max - 1;
        default:                   return 1 + (max - 2) / 2;
    }
}
```

Kernels with very small priority spaces (e.g. `configMAX_PRIORITIES < 6`) may collapse adjacent levels — that's a configuration constraint of the build, not a KVB design issue. KVB requires a minimum of five distinct preemptive priorities for full test coverage.

## Thread stack memory

The KVB API takes a `(stack_mem, stack_size)` pair. `stack_size` is the **usable** stack size — the same meaning every other RTOS API gives that parameter. Most ports pass `stack_mem` straight into the kernel because the kernel allocates the TCB elsewhere (FreeRTOS `StaticTask_t`, ThreadX `TX_THREAD`, Zephyr `struct k_thread`). On TaktOS the TCB lives inside the caller-provided block, so the test's backing buffer must be sized to include the per-architecture overhead.

KVB exposes that translation as a single macro:

```c
#define KVB_THREAD_BUF_SIZE(usable_bytes)  /* port-defined */
```

For most kernels this is the identity. The TaktOS types header overrides it to `TAKTOS_THREAD_MEM_SIZE(usable_bytes)`. Tests size every per-thread stack array via this macro:

```c
static uint8_t worker_stack[KVB_THREAD_BUF_SIZE(KVB_DEFAULT_STACK_SIZE)];
```

After this, `usable_bytes` of usable stack is identical across all ports.

## Required platform port functions

Platform ports implement the API declared in:

```text
include/kvb_platform_port.h
```

The first required platform set is:

- cycle counter (`kvb_platform_cycle_count`)
- cycle frequency (`kvb_platform_cycle_frequency_hz`)
- microsecond time (`kvb_platform_time_us`)
- log write (`kvb_platform_log_write`, `kvb_platform_log_flush`)
- platform metadata (`kvb_platform_board_name` etc.)

`kvb_platform_time_us` must return either real microseconds or `0` consistently throughout the run. KVB tests detect a stuck-zero source and report `KVB_RESULT_INVALID` rather than producing meaningless throughput numbers.

### Cortex-M cycle-source caveat

The Cortex-M platform port must not assume that `DWT->CYCCNT` exists. DWT/CYCCNT is available on typical ARMv7-M and ARMv8-M Mainline cores such as Cortex-M3, Cortex-M4, Cortex-M7, Cortex-M33, and Cortex-M55. It is not present on ARMv6-M cores such as Cortex-M0 and Cortex-M0+; STM32F03xx is in this no-DWT class.

For Cortex-M0/M0+ targets, disable DWT in the build or let the port auto-detect no-DWT, then provide a board-specific implementation of:

```c
uint64_t kvb_platform_cortex_m_fallback_time_us(void);
```

The fallback should use a hardware timer or another kernel-independent monotonic microsecond source. If no fallback is provided, timing-dependent tests report `KVB_RESULT_INVALID`.

## Result rule

Every test must produce exactly one result state:

```text
PASS         expected behavior matched
FAIL         behavior violated expected result
UNSUPPORTED  kernel does not provide the required primitive or behavior
SKIPPED      test was intentionally not run
INVALID      harness/configuration error — result is not usable
```

Performance metrics are optional for validation-only tests, but validation status is mandatory for every benchmark test.

## Throughput hot loops

Throughput tests batch their wall-clock reads via `KVB_THROUGHPUT_BATCH` (default 1024 ops). Reading the cycle counter on every operation adds a fixed overhead that disproportionately suppresses fast kernel paths. Batching keeps measurement bias well below 0.1 % even for sub-twenty-cycle fast paths.

## TaktOS private test bed

The private development copy uses TaktOS as the first kernel port:

```text
ports/kernels/taktos/
```

The goal is to validate the KVB design while using TaktOS as the first real kernel under test.

## Current private ports

The private development tree currently includes first-pass ports for:

```text
ports/kernels/taktos/
ports/kernels/freertos/
ports/kernels/threadx/
ports/kernels/zephyr/
```

### FreeRTOS port notes

The FreeRTOS port uses native FreeRTOS primitives:

- `xTaskCreateStatic()` / `xTaskCreate()`
- `vTaskStartScheduler()`, `vTaskDelay()`, `taskYIELD()`
- `xSemaphoreCreateCountingStatic()` / `xSemaphoreCreateCounting()`
- `xSemaphoreCreateMutexStatic()` / `xSemaphoreCreateMutex()`
- `xQueueCreateStatic()` / `xQueueCreate()`

For static allocation, KVB-provided stack memory is passed directly to FreeRTOS. The application build must ensure that KVB worker stacks are properly aligned for `StackType_t` on the target.

If a test requests `KVB_MUTEX_RECURSIVE` on a build without `configUSE_RECURSIVE_MUTEXES`, the port returns `KVB_ERR_UNSUPPORTED` rather than emulating with a non-recursive mutex.

Recommended FreeRTOS configuration for balanced KVB validation:

```text
configUSE_PREEMPTION=1
configMAX_PRIORITIES >= 8
configUSE_MUTEXES=1
configUSE_COUNTING_SEMAPHORES=1
configUSE_RECURSIVE_MUTEXES=1   (optional)
configSUPPORT_STATIC_ALLOCATION=1   (preferred)
configCHECK_FOR_STACK_OVERFLOW=2    (preferred)
configASSERT enabled                (preferred)
```

### ThreadX port notes

The ThreadX port uses native ThreadX primitives:

- `tx_kernel_enter()`, `tx_application_define()`
- `tx_thread_create()`, `tx_thread_sleep()`, `tx_thread_relinquish()`
- `tx_semaphore_create()` / `tx_semaphore_get()` / `tx_semaphore_put()`
- `tx_mutex_create()` / `tx_mutex_get()` / `tx_mutex_put()`
- `tx_queue_create()` / `tx_queue_send()` / `tx_queue_receive()`

The port provides `tx_application_define()`. For benchmark executables, do not define another `tx_application_define()` unless the project intentionally forwards into the KVB hook.

ThreadX queue message size is expressed in `ULONG` words. The port rounds KVB message size up to whole `ULONG` words and rejects invalid sizes. The port returns `KVB_ERR_UNSUPPORTED` when `KVB_MUTEX_RECURSIVE` is requested.

Recommended ThreadX configuration for balanced KVB validation:

```text
TX_DISABLE_ERROR_CHECKING not defined
TX_ENABLE_STACK_CHECKING defined when available
mutexes created with TX_INHERIT
TX_MAX_PRIORITIES >= 8
```

### Zephyr port notes

The Zephyr port uses native Zephyr primitives:

- `k_thread_create()`, `k_yield()`, `k_sleep()`
- `k_sem_init()` / `k_sem_take()` / `k_sem_give()`
- `k_mutex_init()` / `k_mutex_lock()` / `k_mutex_unlock()`
- `k_msgq_init()` / `k_msgq_put()` / `k_msgq_get()`

Zephyr starts the kernel before `main()`, so the KVB Zephyr port runs the test runner from the calling application thread. The port lifts that thread to the canonical `KVB_PRIORITY_HIGHEST` so the runner-vs-worker scheduling relationship matches the other ports.

Recommended Zephyr configuration for balanced KVB validation:

```text
CONFIG_PREEMPT_ENABLED=y
CONFIG_TIMESLICING=n
CONFIG_NUM_PREEMPT_PRIORITIES >= 5
CONFIG_THREAD_STACK_INFO=y
CONFIG_INIT_STACKS=y
CONFIG_RUNTIME_ERROR_CHECKS=y
CONFIG_ASSERT=y
CONFIG_ASSERT_LEVEL=2
CONFIG_MUTEX_PRIORITY_INHERITANCE=y
```

Current Zephyr splits API-entry parameter validation across two mechanisms; both need to be enabled to match what the other KVB-supported kernels carry. `CONFIG_RUNTIME_ERROR_CHECKS=y` selects the runtime-check arm of Zephyr's "Error checking behavior for CHECK macro" Kconfig choice — pinning it explicitly is more robust than relying on `CONFIG_NO_RUNTIME_CHECKS=n`, because the latter is only the absence of one Kconfig and prior build state / menuconfig can leave the choice resolved differently. CHECKIF coverage on the public surface is narrow (`k_sem_init` count/limit, `k_mutex_unlock` non-owner / unlocked-mutex, `k_msgq_cleanup` busy). `CONFIG_ASSERT=y` keeps the `__ASSERT()` layer active, where most of Zephyr's API-entry validation actually lives (`k_sem_take` ISR/timeout, `k_mutex_lock` / `k_mutex_unlock` ISR + lock-count, `k_msgq_put` / `k_msgq_get` ISR/timeout + internal pointer); setting it `n` compiles all of these out and leaves a strictly weaker validation surface. The trade-off is that `CONFIG_ASSERT=y` also enables `__ASSERT`s inside scheduler / spinlock / wait-queue helpers that the other kernels have no equivalent of — there is no Kconfig granularity to split the two, so the build accepts the parity-correct overhead.

### TaktOS port notes

TaktOS stores the TCB inside the caller-provided thread block. The TaktOS types header overrides `KVB_THREAD_BUF_SIZE` to add the per-architecture overhead so that tests obtain the same usable stack across kernels. The runner stack inside the TaktOS port itself is sized the same way.

The port returns `KVB_ERR_UNSUPPORTED` when `KVB_MUTEX_RECURSIVE` is requested. TaktOS provides priority-inheritance mutex with non-recursive semantics.
