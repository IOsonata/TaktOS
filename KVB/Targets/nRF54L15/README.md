# KVB Targets/nRF54L15

Cross-kernel KVB benchmark + validation tree for the Nordic
Semiconductor **nRF54L15-DK** board, based on the **nRF54L15**
(Cortex-M33 @ 128 MHz, 256 KB SRAM, 1.5 MB flash).

## Why this target

Adds a Cortex-M33 reference point to the KVB benchmark portfolio
alongside the STM32F0308 (Cortex-M0) target.  The two together cover
the most common embedded-RTOS performance regimes:

- **STM32F0308**: tight 8 KB SRAM, no DWT, ARMv6-M baseline — stresses
  static-allocation strategies and exposes the cost of every byte of
  TCB / control-block storage.
- **nRF54L15**: comfortable 256 KB SRAM, DWT cycle counter, ARMv8-M Mainline
  with FPU and MPU — exercises the kernels at higher absolute speed
  and with cycle-accurate timing throughout.

Same six KVB tests run on both targets, identical workloads, so per-
target throughput numbers can be cross-compared MCU by MCU.

## Layout

Standard KVB target shape:

```
KVB/Targets/nRF54L15/
├── README.md                            (this file)
├── include/
│   ├── board.h                          UART pin map + core clock
│   ├── kvb_config_nrf54l15.h            TaktOS variant
│   ├── kvb_config_nrf54l15_freertos.h   FreeRTOS variant
│   ├── kvb_config_nrf54l15_threadx.h    ThreadX variant
│   └── FreeRTOSConfig.h                 FreeRTOS kernel config
│       (ThreadX feature config lives in KVB/include/tx_user.h, shared
│        across every KVB target)
├── src/
│   ├── kvb_platform_nrf54l15.cpp        board / CPU strings only
│   └── tx_initialize_low_level.S        ThreadX SysTick + priority init
├── TaktOS_nRF54L15_KvbSuite/
│   └── README.md                        TaktOS suite
├── FreeRTOS_nRF54L15_KvbSuite/
│   └── README.md                        FreeRTOS suite
└── ThreadX_nRF54L15_KvbSuite/
    └── README.md                        ThreadX suite
```

Per-suite Eclipse `.project` and `.cproject` files are NOT shipped
here — copy them from the existing nRF54L15 reference projects in
your tree (Thread-Metric or other) and update the linked-resource
paths to point at this directory.  The path-variable convention is
documented in `TaktOS_nRF54L15_KvbSuite/README.md`.

## What this target does NOT need

Cortex-M33 has DWT/CYCCNT in the base implementation, so the cortex_m
default platform port (`KVB/ports/platforms/cortex_m/kvb_platform_cortex_m.c`)
provides cycle-accurate microsecond timing out of the box.

That means the per-MCU `kvb_platform_nrf54l15.cpp` is much smaller than
its STM32F0308 sibling — only board/CPU identification strings.  No
strong override of `kvb_platform_cortex_m_fallback_time_us()` is
needed; the DWT path runs at one-cycle resolution (~15.6 ns @ 128 MHz)
without any per-MCU plumbing.

## Cross-kernel parity (across the three suites)

| Item | TaktOS | FreeRTOS | ThreadX |
| --- | --- | --- | --- |
| Tick rate | 1000 Hz | 1000 Hz | 1000 Hz |
| Measurement window | 10 s | 10 s | 10 s |
| Throughput batch | 256 | 256 | 256 |
| Warmup | 100 ms | 100 ms | 100 ms |
| UART FIFO size | 256 B | 256 B | 256 B |
| Runner stack | 1024 B | 1024 B | 1024 B |
| Worker stack | 512 B | 512 B | 1024 B (asymmetry) |
| Stack overflow check | always-on | check-level 2 | TX_ENABLE_STACK_CHECKING |
| Null-arg / object-validity check | always-on | configASSERT | _txe_* wrappers |
| Allocation strategy | inline-in-handle | static (no heap_4 needed) | port-private slot pool |
| Optimisation | -Os | -Os | -Os |

The ThreadX worker-stack asymmetry is the one documented difference,
same reason as on STM32F0308 (`_txe_*` wrappers add a C call frame to
every public API).  Everything else is byte-identical across the three
kernels, including the IOsonata UART, the KVB framework version, and
the KVB test set.

## Apply order

1. Drop this directory tree into `KVB/Targets/` in your repo.
2. Copy your existing nRF54L15 Eclipse project's `.cproject` /
   `.project` files into each per-kernel `Eclipse/` subdirectory and
   update the linked-resource paths so they resolve through
   `PARENT-2-PROJECT_LOC` / `PARENT-3-PROJECT_LOC` /
   `PARENT-4-PROJECT_LOC` against the layout shown above.
3. For each suite, set the Eclipse project's `Pre-include files` to
   the appropriate `kvb_config_nrf54l15*.h` so the tunings land before
   `kvb_config.h` defaults.
4. Build the dependency libraries (TaktOS M4, FreeRTOS M4F, ThreadX
   cortex_m33, IOsonata nRF54L15) and build each suite.
5. Flash via J-Link OB, capture log over `/dev/ttyACM*` at 115200 8N1.

## Companion patches

If you have applied the **`kvb_fairness_patch/`** series, no further
changes to KVB framework code are required — this target just adds new
files under `KVB/Targets/nRF54L15/`.

If you have applied the **`ipcp_mutex_patch/`** series, the new
`kvb_mutex_create_protect()` API and `KVB_MUTEX_PROTECT` flag will be
available across all three kernels (TaktOS supports it, FreeRTOS and
ThreadX return `KVB_ERR_UNSUPPORTED`).  This makes the nRF54L15 target
the ideal place to add the `SYNC_MUTEX_PCP_001` test once it is
written, since Cortex-M33 has the cycle resolution to capture the IPCP
boost / unboost timings precisely.

## Status

New target.  Numbers go in the per-suite READMEs and in the KVB
benchmark report once a measurement run is captured on hardware.

## Author / date

Nguyen Hoan Hoang, I-SYST inc. — April 2026
