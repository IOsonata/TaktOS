# KVB Zephyr — board-agnostic test suite

This is the **KVB benchmark suite as a Zephyr application**, board-
agnostic.  Build for any Zephyr-supported board by selecting the
target at `west build` time.  No per-board source code; no
per-board CMake.

The same KVB framework, the same Zephyr kernel port, the same test
bodies, the same prj.conf — only the `-b <board>` flag changes.

Author: Nguyen Hoan Hoang, I-SYST inc.

---

## What this measures

KVB exercises 6 kernel primitives via end-to-end iteration throughput
over a 10-second window:

| Test ID                  | What it stresses                                     |
| ------------------------- | ---------------------------------------------------- |
| SCHED_COOP_001            | Cooperative yield — pure scheduler latency loop      |
| SYNC_SEM_FAST_001         | Uncontended semaphore wait/post round-trip           |
| SYNC_MUTEX_FAST_001       | Uncontended mutex lock/unlock round-trip             |
| SYNC_MUTEX_OWNERSHIP_001  | Non-owner unlock rejected (PASS/FAIL behavioural)    |
| IPC_QUEUE_FAST_001        | Same-thread queue send/receive round-trip            |
| TIME_SLEEP_001            | k_sleep accuracy vs requested duration               |

Output is plain printable lines via printk:

```
[KVB] BEGIN RUN
[KVB] VERSION 0.1.0-private-zephyr
[KVB] KERNEL name=Zephyr version=4.3.99
[KVB] FEATURES dyn_threads=1 sem=1 mutex=1 mutex_pi=1 ...
[KVB] PLATFORM board=nrf54l15dk cpu=ARMv8-M Mainline @ ... clock_hz=...
[KVB] BEGIN id=SCHED_COOP_001 ...
... (6 tests) ...
[KVB] SUMMARY pass=6 fail=0 ...
[KVB] END RUN
```

A 4-run aggregate — power-cycle the board four times — is the standard
methodology used in the published KVB v2.0 report.

---

## Prerequisites

- Zephyr ≥ v4.0 (or nRF Connect SDK ≥ v2.7 if targeting Nordic boards
  not yet upstreamed).
- A west workspace with the toolchain installed.
- The board you want to target supported by Zephyr (run
  `west boards` to list them).

---

## Build (any board)

From this directory (`KVB/Targets/Zephyr/KvbSuite/`):

```bash
# Pick whichever board you want to benchmark
west build -p always -b nrf54l15dk/nrf54l15/cpuapp .
west build -p always -b nrf52840dk/nrf52840 .
west build -p always -b stm32l4r9i_iot01a .
west build -p always -b qemu_cortex_m3 .
```

`-p always` forces a clean configure to pick up any prj.conf or KVB
source changes since the last build.

---

## Flash

```bash
west flash
```

For Nordic boards via J-Link this uses nrfjprog or nrfutil under the
hood.  For STM32 it uses OpenOCD or J-Link.  For QEMU targets, run
`west build -t run` instead of `west flash`.

---

## Capture KVB log output

The host serial port depends on the board:

| Board                | Typical host enumeration                   |
| -------------------- | ------------------------------------------ |
| nRF54L15-DK          | `/dev/ttyACM0` (J-Link CDC ACM)            |
| nRF52840-DK          | `/dev/ttyACM0` (J-Link CDC ACM)            |
| STM32L4 IoT Disco    | `/dev/ttyACM0` (ST-LINK V2.1 VCP)          |
| QEMU                 | stdout of `west build -t run`              |

Connect at **115200 8N1** (or whatever the board's `chosen,zephyr,console`
defaults to — usually 115200) and reset the board.

For a 4-run aggregate, reset four times and capture all four runs into
one log file.

---

## Files

```
KVB/Targets/Zephyr/KvbSuite/
├── CMakeLists.txt          Generic Zephyr application
├── prj.conf                Generic Kconfig — feature parity with other ports
├── README.md               This file
├── boards/
│   └── README.md           When and how to add per-board overrides
├── src/
│   ├── main.c              Calls kvb_zephyr_start_default()
│   └── kvb_platform_zephyr.c  Generic platform layer using only Zephyr APIs
└── include/
    └── kvb_config_zephyr.h Generic KVB knobs (10 s window, 1024 B stacks)
```

The KVB application sources, kernel-port, and test bodies live in their
canonical KVB tree locations:

```
KVB/
├── include/                    KVB framework headers
├── src/core/                   Framework core (logger, runner, registry)
├── src/tests/                  Test bodies
└── ports/kernels/zephyr/       Zephyr kernel port (kvb_port_zephyr.c)
```

The generic CMakeLists.txt in this directory pulls those in via
`${KVB_ROOT}` relative paths.

---

## Adding a board

If the board's defaults are sane, **no extra files needed** — just
`west build -b <board> .` and you're done.

If the board needs tweaks (different tick rate, smaller stacks, custom
console pinout), drop a per-board `.conf` and/or `.overlay` file in
`boards/`.  See `boards/README.md` for the conventions.

---

## Comparing against other kernels

The KVB tree includes ports for TaktOS, FreeRTOS V11.1+, and Eclipse
ThreadX 6.x in addition to Zephyr.  Those ports build natively
(Eclipse + IOsonata) for specific MCUs (currently STM32F0308-DISCO);
the published v2.0 report shows TaktOS vs FreeRTOS vs ThreadX numbers
on that target.

The Zephyr column adds Zephyr to the same comparison on whichever
board you select for the Zephyr build.  Like-for-like requires the
same MCU across all four kernels — for the nRF54L15 family that means
also bringing up TaktOS / FreeRTOS / ThreadX native targets on the
same board, which is a separate effort.
