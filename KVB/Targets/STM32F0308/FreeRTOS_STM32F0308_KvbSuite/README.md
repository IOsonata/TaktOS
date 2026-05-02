# FreeRTOS_STM32F0308_KvbSuite

KVB benchmark suite running on **FreeRTOS** for the STM32F0308-DISCO board.

Sibling project of `TaktOS_STM32F0308_KvbSuite` — same hardware, same UART
console, same shared platform layer, same KVB framework, same registered
test set. The only difference is the kernel under test: this project
links the FreeRTOS kernel (sources from IOsonata's `external/FreeRTOS-Kernel/`)
instead of the TaktOS_M0 static library.

Comparable like-for-like cycle counts come out at the other end —
both kernels run identical worker thread counts, identical batch sizes,
identical UART output buffering, and emit `[KVB] METRIC` lines in the
same format consumed by `KVB/tools/parse_log.py`.

## Configuration

- **Allocation:** static only.  `configSUPPORT_STATIC_ALLOCATION = 1`,
  `configSUPPORT_DYNAMIC_ALLOCATION = 0`, `configTOTAL_HEAP_SIZE = 0`.
  No heap, no `pvPortMalloc` calls anywhere in the test path.  Eliminates
  any "comparison clouded by heap allocator" concern.

- **Tick rate:** 1000 Hz, matching the TaktOS counterpart and KVB's
  default `KVB_TICK_HZ`.

- **Priorities:** `configMAX_PRIORITIES = 8`.  KVB's five canonical
  priority levels (LOWEST/LOW/NORMAL/HIGH/HIGHEST) are mapped to
  FreeRTOS priorities 1..7 by `kvb_port_freertos.c`.  Priority 0 is
  reserved for `tskIDLE_PRIORITY` per FreeRTOS convention.

- **Port:** `portable/GCC/ARM_CM0` from IOsonata's bundled FreeRTOS-Kernel.
  No `configUSE_PORT_OPTIMISED_TASK_SELECTION` — that path depends on a
  CLZ instruction the M0 base ISA does not have.

- **Disabled features:** software timers, run-time stats, trace facility,
  malloc-failed hook, stack-overflow check, idle hook, tick hook.  Each
  costs SRAM and adds dead code paths — KVB's static config does not
  need any of them.

## Memory budget

| Component | Bytes |
|---|---:|
| FreeRTOS kernel state + idle TCB + 256 B idle stack  | ~600 |
| Runner task (StaticTask_t + 512 B stack)             | ~600 |
| 3 worker tasks (StaticTask_t + 256 B stack)          | ~1024 |
| Mutex-owner task (StaticTask_t + 256 B stack)        | ~344 |
| IOsonata UART TxFIFO (128 B) + driver state          | ~300 |
| Queue + sem + mutex storage (StaticQueue_t etc)      | ~400 |
| KVB statics (registry 16 * 4 B + result + log)       | ~400 |
| ISR/main MSP + bss headroom                          | ~700 |
| **Total**                                            | **~4.4 KB / 8 KB** |

The TaktOS counterpart sits at ~7.6 KB / 8 KB on the same board with the
same KVB test set.  The 3 KB difference is **not** a TaktOS bug — TaktOS
reserves a 256 B stack guard region per thread on M0 to catch overflow
without hardware MPU.  FreeRTOS does not provide that protection at the
static-config level.  Both produce correct programs at the configured
stack sizes.

## Files

This project links no source files of its own — every source is shared
or comes from the KVB framework.  Two project-specific files in
`KVB/Targets/STM32F0308/include/`:

- `FreeRTOSConfig.h` — kernel configuration (M0, tick=1000Hz, static).
- `kvb_config_stm32f0308_freertos.h` — KVB build tunings (forced via
  `-include`, lands before the framework defaults).

One project-specific file in `KVB/Targets/src/`:

- `kvb_freertos_hooks.c` — `vApplicationGetIdleTaskMemory` static
  buffer provider.  Reusable across all FreeRTOS-static-only KVB
  targets (future M4/M33 ports pick this up unchanged).

The shared per-MCU files (`board.h`, `kvb_platform_stm32f0308.cpp`)
are linked from `KVB/Targets/STM32F0308/`.  The shared cross-MCU
files (`main.cpp`, `kvb_platform_iosonata.cpp`) come from
`KVB/Targets/src/`.  KVB framework, tests, FreeRTOS port, and Cortex-M
platform layer come from the framework tree.

The FreeRTOS kernel itself is linked as four source files from
`${IOCOMPOSER_HOME}/external/FreeRTOS-Kernel/`:

- `list.c`
- `queue.c`
- `tasks.c`
- `portable/GCC/ARM_CM0/port.c`

No `MemMang/heap_*.c` is linked — heap allocation is impossible at
build time (`configTOTAL_HEAP_SIZE = 0`).

## Running

Same procedure as `TaktOS_STM32F0308_KvbSuite`:

1. Open the workspace in Eclipse with the IOsonata project tree set up.
2. Build Debug or Release.
3. Flash via OpenOCD or ST-LINK GDB Server.
4. Open a terminal on the ST-LINK VCOM at 115200 8N1.
5. Reset the board.  KVB log streams the same `[KVB] BEGIN RUN ...
   [KVB] END RUN` sequence as the TaktOS variant.

Pipe the captured log through `KVB/tools/parse_log.py` and
`KVB/tools/generate_report.py` to produce the comparable Markdown
report side-by-side with the TaktOS run.

## Direct comparison checklist

For the comparison to be fair, both projects must emit:

- Same `KVB_DEFAULT_STACK_SIZE` (256 B usable) and same
  `KVB_RUNNER_STACK_SIZE` (512 B usable) — verified in the per-board
  config headers.
- Same `KVB_THROUGHPUT_BATCH` (256) — verified.
- Same `KVB_MEASUREMENT_MS` (1000) — verified.
- Same `KVB_WORKER_THREAD_COUNT` (3) — verified.
- Same UART rate and FIFO size (115200, 128 B) — verified.
- Same compiler, same optimization level (`-Os` Release / `-O0` Debug
  for both).

Anything that differs structurally between the kernels (TaktOS's M0
guard region, FreeRTOS's idle task) is documented above and cannot be
made equal — they are inherent design choices, not benchmark artifacts.
