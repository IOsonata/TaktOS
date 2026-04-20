# TaktOS Examples

These examples have been updated to the current public API in `include/`.
They are deliberately **board-agnostic**: no BSP layer, no vendor HAL, and no
`printf` dependency. Observable behavior is exposed through `volatile` counters
that you can inspect in a debugger.

## Files

| Path | Language | What it demonstrates |
|---|---|---|
| `basic/basic_c.c` | C | Kernel init, thread creation, semaphore handoff, periodic sleep |
| `basic/basic_cpp.cpp` | C++ | Same as basic C example using `TaktOSThread` and `TaktOSSem` wrappers |
| `queue/producer_consumer_c.c` | C | Fixed-size queue between producer and consumer threads |
| `queue/producer_consumer.cpp` | C++ | Same pattern using the `TaktOSQueue` wrapper |
| `mutex/mutex_pi_c.c` | C | Priority inheritance scenario using `TaktOSMutex_t` |
| `mutex/mutex_pi.cpp` | C++ | Same scenario using the `TaktOSMutex` wrapper |
| `posix/posix_pse51.c` | C | POSIX threads, mutex, condvar, semaphore, timer on top of TaktOS |
| `posix/posix_pse51_cpp.cpp` | C++ | Same POSIX layer from C++ with a small RAII lock helper |
| `mpu_guard.c` | C | MPU stack-guard example; pass a writable handler table base when the target port needs dynamic handler patching |

## Common pattern

Every example follows the same startup model:

```c
#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

int main(void)
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    /* create threads */
    TaktOSStart();
}
```

Change `APP_CORE_CLOCK_HZ` to match your target if it is not 48 MHz.
The examples use a **1 kHz tick** for simple human-readable timing. Pass
`0u` for `HandlerBaseAddr` when using the default statically linked handler
path. Supply an application-owned writable handler/trap table base only on
ports that support dynamic patching.

## Current public APIs used here

### Thread API

```c
static uint8_t mem[TAKTOS_THREAD_MEM_SIZE(512)] __attribute__((aligned(4)));
TaktOSThreadCreate(mem, sizeof(mem), Entry, NULL, TAKTOS_PRIORITY_NORMAL);
TaktOSThreadSleep(TaktOSCurrentThread(), 100u);
TaktOSThreadYield();
```

### Semaphore API

```c
static TaktOSSem_t sem;
TaktOSSemInit(&sem, 0u, 1u);
TaktOSSemGive(&sem, false);
TaktOSSemTake(&sem, true, TAKTOS_WAIT_FOREVER);
```

### Queue API

```c
static uint8_t storage[8u * sizeof(Message_t)] __attribute__((aligned(4)));
static TaktOSQueue_t q;
TaktOSQueueInit(&q, storage, sizeof(Message_t), 8u);
TaktOSQueueSend(&q, &msg, true, TAKTOS_WAIT_FOREVER);
TaktOSQueueReceive(&q, &msg, true, TAKTOS_WAIT_FOREVER);
```

### Mutex API

```c
static TaktOSMutex_t mtx;
TaktOSMutexInit(&mtx);
TaktOSMutexLock(&mtx, true, TAKTOS_WAIT_FOREVER);
TaktOSMutexUnlock(&mtx);
```

### POSIX layer

```c
#include "posix/pthread.h"
#include "posix/semaphore.h"
#include "posix/time.h"
#include "posix/ptimer.h"
```

The POSIX examples run **on top of TaktOS**. They still need native
`TaktOSInit()` / `TaktOSStart()` to bootstrap the kernel.


## Verification harness

From the repository root you can keep the examples checked with either:

```sh
cd examples
make check
```

or:

```sh
./examples/verify_examples.sh
```

Both run a syntax-only pass over every example using the current public
headers. By default they use `clang` / `clang++`, but you can override that:

```sh
CC=arm-none-eabi-gcc CXX=arm-none-eabi-g++ ./examples/verify_examples.sh
```

## Notes

- `TAKTOS_WAIT_FOREVER` blocks indefinitely in thread context.
- `TAKTOS_NO_WAIT` makes the operation non-blocking.
- The C examples now match the current headers. Older names like
  `TaktOS_Init`, `TaktTaskInit`, `TaktMutexAcquire`, and `TaktOSDelay`
  are no longer used.
