# KVB Zephyr — nRF54L15-DK

This target runs the KVB benchmark suite on the **Nordic nRF54L15-DK**
(PCA10156) under **Zephyr** (or nRF Connect SDK), as a fourth kernel
column alongside the existing TaktOS / FreeRTOS / Eclipse ThreadX
results on STM32F0308-DISCO.

| Property        | Value                                    |
| --------------- | ---------------------------------------- |
| MCU             | nRF54L15 (Nordic Semiconductor)          |
| Core            | ARM Cortex-M33 (ARMv8-M Mainline)        |
| Clock           | 128 MHz                                  |
| SRAM            | 256 KB                                   |
| Flash (RRAM)    | 1.5 MB                                   |
| Console         | uart30 → J-Link CDC ACM (IMCU USB port)  |
| Zephyr board    | `nrf54l15dk/nrf54l15/cpuapp`             |

Author: Nguyen Hoan Hoang, I-SYST inc.

---

## Prerequisites

- **nRF Connect SDK ≥ v2.7** (or upstream Zephyr ≥ v4.0 with
  Nordic HAL support for nRF54L15) — the nRF54L15-DK board target
  was added in NCS v2.7 / Zephyr v4.0.
- `west` workspace initialised against either NCS or Zephyr.
- nRF Connect for Desktop (or `nrfjprog` from
  nRF Command Line Tools) for flashing — Zephyr's `west flash` uses
  nrfjprog by default for Nordic targets.
- Zephyr SDK ≥ 0.16.8 (or compatible toolchain, e.g. nRF Connect
  toolchain bundle).

## Build

From this directory:

```bash
west build -p always -b nrf54l15dk/nrf54l15/cpuapp .
```

`-p always` forces a clean configure to pick up any prj.conf or
KVB source changes since the last build.

## Flash

```bash
west flash
```

If the device is locked (factory readback protection), unlock once
with `west flash --recover`, then repeat the flash.

## Capture KVB log output

The board enumerates as a JLink CDC ACM device on the host. Connect
a serial terminal at **115200 8N1** to the corresponding port:

```bash
# Linux example
minicom -D /dev/ttyACM0 -b 115200

# macOS example
screen /dev/cu.usbmodem* 115200
```

Reset the board (or power-cycle). KVB writes 6 tests' worth of log
output: `BEGIN RUN` → 6× `BEGIN/RESULT/METRIC×N/END` → `SUMMARY` →
`END RUN`. With `KVB_MEASUREMENT_MS = 10000`, a full run takes ~60 s.

For a 4-run aggregate (matching the v2.0 KVB report methodology),
cycle the board reset 4 times and capture all four runs into one
log file.

## Files in this target

```
KVB/Targets/nRF54L15-DK/
├── Zephyr_nRF54L15-DK_KvbSuite/      <-- THIS application (Zephyr-style)
│   ├── CMakeLists.txt                Zephyr app build that pulls in KVB sources
│   ├── prj.conf                      Kconfig for parity with TaktOS/FreeRTOS/ThreadX
│   ├── boards/
│   │   └── nrf54l15dk_nrf54l15_cpuapp.overlay   (empty default)
│   ├── src/
│   │   └── main.c                    Calls kvb_zephyr_start_default()
│   └── README.md                     This file
├── include/
│   └── kvb_config_nrf54l15dk_zephyr.h  Per-target KVB knobs (force-included)
└── src/
    └── kvb_platform_nrf54l15dk.c     Microsecond clock + log + ID strings
```

## Feature parity with the other KVB ports

| Safety check                | TaktOS               | FreeRTOS         | ThreadX               | Zephyr (this build)            |
| --------------------------- | -------------------- | ---------------- | --------------------- | ------------------------------ |
| Stack overflow detection    | sentinel (always-on) | configCHECK=2    | TX_ENABLE_STACK_CHECK | CONFIG_STACK_SENTINEL=y        |
| MPU stack guard region      | yes (M0+/M3/.../M33) | no               | no                    | CONFIG_HW_STACK_PROTECTION=y   |
| Null parameter check        | inline (always-on)   | configASSERT     | _txe_* wrappers       | CONFIG_ASSERT=y                |
| Object validity check       | inline (always-on)   | configASSERT     | _txe_* wrappers       | CONFIG_ASSERT=y                |
| Mutex priority inheritance  | yes                  | yes              | yes                   | CONFIG_MUTEX_PRIO_INHERIT=y    |
| Static allocation           | inline-in-handle     | static + heap_4  | port-private pools    | CONFIG_HEAP_MEM_POOL_SIZE=0    |

The HW MPU stack guard is enabled here because nRF54L15 (Cortex-M33)
HAS an MPU, the same way TaktOS production builds on M3/M4/M7/M33/M55
enable MPU stack guard. FreeRTOS and ThreadX do not provide an
equivalent feature on this target — disclosed in the comparison report.

## Open items

1. **First-run bring-up** — needs a board flash and 4-run capture to
   confirm all 6 KVB tests PASS and produce reproducible numbers.
2. **Cross-port comparison** — once captured, the numbers slot into a
   v3.0+ KVB comparison report alongside the existing
   STM32F0308 TaktOS/FreeRTOS/ThreadX columns and any future
   nRF54L15-DK TaktOS/FreeRTOS/ThreadX columns built on the same
   hardware (apples-to-apples requires same target, not just same
   benchmark suite).
3. **`__VERSION__` vs `KERNEL_VERSION_STRING`** — the Zephyr port
   reports `KERNEL_VERSION_STRING` (e.g. `"4.0.0"`) as the kernel
   version. Confirm KVB's log header line renders correctly.
