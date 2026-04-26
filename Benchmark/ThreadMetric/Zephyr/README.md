# Thread-Metric Zephyr port (apples-to-apples with TaktOS / FreeRTOS / ThreadX)

Zephyr is a build-config-driven RTOS, not a per-MCU project tree like
Eclipse-based RTOSes. There is **one project per benchmark test**; the target
MCU is selected at build time via `west build -b <board>`.

## Layout

```
ThreadMetric/Zephyr/
├── common/
│   ├── main.c                 # shared Zephyr entry  calls tm_main()
│   └── prj.conf               # strict-parity reference config (copied per project)
├── Zephyr_BasicProcessing/
│   ├── CMakeLists.txt
│   └── prj.conf
├── Zephyr_CooperativeScheduling/
├── Zephyr_PreemptiveScheduling/
├── Zephyr_MessageProcessing/
├── Zephyr_SynchronizationProcessing/
├── Zephyr_MutexProcessing/
└── Zephyr_MutexBargingTest/
```

Each project's `CMakeLists.txt` references:
- `../common/main.c`            shared boot
- `../../src/tm_port_zephyr.c`  shared port (uses `tm_api.h` void-entry)
- `../../src/tm_report_zephyr.c` Zephyr printk reporter
- `../../src/<test>.c`           same shared test file used by all RTOSes

## What's the same as TaktOS / FreeRTOS / ThreadX

- Same shared test sources: `basic_processing.c`, `mutex_barging_test.c`, etc.
- Same `tm_api.h` (23 functions, void-entry thread signature)
- Same `TM_PORT_MAX_THREADS=12`, `TM_PORT_STACK_BYTES=1024`, `TM_PORT_TICK_HZ=1000`
- Same per-window measurement (`TM_TEST_DURATION=30`)
- Same hot-path shape: direct API forward, no defensive checks

## What differs (and why)

- **HAL**: Zephyr uses its own HAL (devicetree, `printk`, UART driver from Zephyr).
  The other RTOSes use IOsonata. This is the documented ecosystem boundary 
  forcing Zephyr onto IOsonata would require a Zephyr-hosted IOsonata layer
  that nobody ships, which would itself bias results.
- **Memory pool**: Zephyr uses native `k_mem_slab` (with pointer validation
  enabled per parity config). TaktOS/FreeRTOS use a hand-rolled linked-list
  pool. ThreadX uses native `tx_block_pool`. This benchmark scope drops
  MemoryAllocation across all RTOSes.

## Strict-parity prj.conf

The `common/prj.conf` is what we use for every Zephyr build. It enables
the safety features that upstream Zephyr's own thread_metric benchmark
config disables:

| Setting                          | Upstream | This config | Why we differ                                  |
|----------------------------------|----------|-------------|------------------------------------------------|
| `CONFIG_HW_STACK_PROTECTION`     | n        | **y**       | production deployments enable this             |
| `CONFIG_MEM_SLAB_POINTER_VALIDATE`| n       | **y**       | production safety check on slab free           |
| `CONFIG_MPU` / `CONFIG_ARM_MPU`  | n        | **y**       | matches what Zephyr ships by default           |
| `CONFIG_TICKLESS_KERNEL`         | y        | **n**       | other RTOSes are tick-driven; match them       |
| `CONFIG_SYS_CLOCK_TICKS_PER_SEC` | 100      | **1000**    | other RTOSes run 1 kHz; 100 Hz gives Zephyr 10× advantage |
| `CONFIG_SPEED_OPTIMIZATIONS`     | y        | **n**       | other RTOSes use `-Os`; match optimisation level |
| `CONFIG_SIZE_OPTIMIZATIONS`      | (n)      | **y**       | ditto                                          |

## Building

```
west build -b nrf52dk/nrf52832          ThreadMetric/Zephyr/Zephyr_BasicProcessing
west build -b nrf54l15dk/nrf54l15/cpuapp ThreadMetric/Zephyr/Zephyr_BasicProcessing
```

The same project tree builds on any Zephyr-supported board with FPU and a
working serial console (Cortex-M4F, Cortex-M33, RV32IMAFC, ).

## Running

UART output is via `printk` over the board's default console. Same line
format as the other ports (`**** Thread-Metric ... ****`, `Time Period
Total: <N>`).
