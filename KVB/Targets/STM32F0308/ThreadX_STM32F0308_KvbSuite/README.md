# ThreadX_STM32F0308_KvbSuite

KVB benchmark suite running on **Eclipse ThreadX** (formerly Azure RTOS,
formerly Express Logic ThreadX) for the STM32F0308-DISCO board.

Sibling project of `TaktOS_STM32F0308_KvbSuite` and
`FreeRTOS_STM32F0308_KvbSuite` — same hardware, same UART console, same
shared platform layer, same KVB framework, same registered test set. The
only difference is the kernel under test: this project links the Eclipse
ThreadX kernel (sources from IOsonata's `external/threadx/`) instead of
the TaktOS_M0 static library or the FreeRTOS-Kernel.

Comparable apples-to-apples throughput numbers come out at the other end —
all three kernels run identical worker-thread counts, identical batch
sizes, identical UART output buffering, and emit `[KVB] METRIC` lines in
the same format consumed by `KVB/tools/compare_runs.py`.

## Configuration

- **Allocation:** ThreadX is naturally static.  Every `tx_*_create` takes
  caller-supplied control-block memory; no `tx_byte_pool` is configured.
  No heap, no kernel-side dynamic allocation anywhere in the test path.

- **Tick rate:** 1000 Hz, matching the TaktOS and FreeRTOS counterparts
  and KVB's default `KVB_TICK_HZ`.  Set in `tx_user.h`
  (`TX_TIMER_TICKS_PER_SECOND = 1000`) and enforced at the SysTick
  reload register by the custom `tx_initialize_low_level.S`.

- **Priorities:** `TX_MAX_PRIORITIES = 32` (ThreadX default).  KVB's
  five canonical priority levels are mapped to ThreadX priorities
  1..30 by `kvb_port_threadx.c`.  Priority 0 is reserved for any
  ISR-deferred-work thread; priority 31 is reserved for an idle-like
  thread.  Note that ThreadX priority 0 is the *highest*, not lowest —
  the inverse of FreeRTOS.

- **Port:** `ports/cortex_m0/gnu/` from upstream eclipse-threadx/threadx,
  latest 6.x release.  Cloned to `IOCOMPOSER_HOME/external/threadx/`
  alongside `external/FreeRTOS-Kernel/`.

- **Custom low-level init:** `tx_initialize_low_level.S` is local to
  this project (in `KVB/Targets/STM32F0308/src/`), not pulled from
  upstream.  Adapted for IOsonata startup — does NOT define its own
  vector table, does NOT alias `__RAM_segment_used_end__`, does NOT
  assume the upstream sample 8 MHz / 100 Hz tick.  Captures system
  stack pointer live via `MRS r0, MSP`.  See file header for the full
  list of differences.

- **PendSV dispatch:** ThreadX's M0/GNU `tx_thread_schedule.S` exports
  `PendSV_Handler` directly as the strong global symbol for the
  context-switch handler.  IOsonata declares `PendSV_Handler` as a
  weak symbol in its vector table, so the linker resolves the
  PendSV exception vector to ThreadX's strong implementation
  automatically.  No alias, wrapper, or trampoline needed —
  context-switch timings on this build are identical to a native
  ThreadX-only environment.

- **Stack overflow detection:** `TX_ENABLE_STACK_CHECKING` defined in
  `tx_user.h`.  ThreadX writes a sentinel pattern at thread create
  time and validates at every context switch.  On detection,
  `kvb_threadx_install_stack_error_handler` (registered from
  `tx_application_define`) logs the offending thread name to UART then
  halts with interrupts disabled.  Same role as
  `vApplicationStackOverflowHook` in the FreeRTOS variant — both
  kernels run with always-on stack guards for parity.

- **Disabled features:** notify callbacks, redundant clearing,
  preemption-threshold, performance counters, trace facility.  Each
  costs code size and adds dead code paths KVB does not exercise.

## Memory budget (8 KB SRAM total)

```
ThreadX kernel infrastructure (timer thread TCB + 768 B stack)  ~= 0.9 KB
Runner thread                 (TX_THREAD + 1024 B stack)        ~= 1.1 KB
Worker tasks (3)              3 * (TX_THREAD + 256 B stack)     ~= 1.2 KB
Mutex-owner task              (TX_THREAD + 256 B stack)         ~= 0.4 KB
IOsonata UART TxFIFO          (UARTFIFOSIZE = 128) + state      ~= 0.3 KB
Queue + sem + mutex storage   (per-test KVB buffers)            ~= 0.3 KB
KVB statics                   (registry + result + log)         ~= 0.4 KB
ISR/main MSP + bss headroom                                     ~= 0.6 KB
----------------------------------------------------------------------------
Total                                                            ~= 5.2 / 8.0 KB
```

Slightly fatter than FreeRTOS (4.4 KB) because of ThreadX's larger timer
thread infrastructure — but well within budget on STM32F030R8.  TaktOS is
slightly fatter again on M0 because of its mandatory 256 B stack-guard
quantum per thread, which neither FreeRTOS nor ThreadX requires.

## Build

1. Clone Eclipse ThreadX:
   ```
   cd $IOCOMPOSER_HOME/external
   git clone https://github.com/eclipse-threadx/threadx.git
   cd threadx
   git checkout <latest-6.x-tag>
   ```
2. Open Eclipse CDT.  File > Import > Existing Projects into Workspace,
   point at `KVB/Targets/STM32F0308/ThreadX_STM32F0308_KvbSuite/Eclipse/`.
3. Build the Release configuration.
4. Flash via ST-LINK.  Connect to the VCOM at 115200 8N1.  Power-cycle
   to capture a clean run.

## Reproducing the cross-kernel comparison

After running 4 separate boots of each variant and saving the UART output
to per-run log files, run:

```
python3 KVB/tools/compare_runs.py \
    --label TaktOS    taktos_run1.log    taktos_run2.log    \
                      taktos_run3.log    taktos_run4.log    \
    --label FreeRTOS  freertos_run1.log  freertos_run2.log  \
                      freertos_run3.log  freertos_run4.log  \
    --label ThreadX   threadx_run1.log   threadx_run2.log   \
                      threadx_run3.log   threadx_run4.log
```

This produces a per-kernel mean-stddev table and a three-way comparison
suitable for the cross-kernel report.

## Author / License

Author: Nguyen Hoan Hoang, I-SYST inc., Brossard, Canada.
KVB framework: MIT (this work).
Eclipse ThreadX: MIT (Eclipse Foundation).
