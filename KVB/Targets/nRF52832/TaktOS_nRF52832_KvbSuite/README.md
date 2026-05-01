# TaktOS_nRF52832_KvbSuite

KVB benchmark + validation suite running on TaktOS for the
Nordic Semiconductor nRF52-DK (PCA10040) board.

## Board

| Item | Value |
| --- | --- |
| MCU | nRF52832 (QFAA) |
| Core | ARM Cortex-M4F (ARMv7E-M) |
| Clock | 64 MHz (HFINT / HFXO) |
| Flash | 512 KB |
| SRAM | 64 KB |
| FPU | single-precision (FPv4-SP) — disabled in KVB build |
| MPU | 8 regions (ARMv7-M) — not used by KVB |
| DWT | present (CYCCNT used by KVB platform layer) |
| Console | UART0 P0.06/P0.08 → J-Link OB VCOM, 115200 8N1 |

## Layout

This project follows the same shape as the STM32F0308 KVB suite, with
the nRF52832-specific shared code one level up:

```
KVB/                                              framework root
├── src/                                          KVB tests + core
├── ports/                                        kernel + platform ports
├── include/                                      public KVB headers
└── Targets/
    ├── src/                                      shared across all MCUs
    │   ├── main.cpp                              IOsonata UART init, calls kvb_kernel_start
    │   └── kvb_platform_iosonata.cpp             kvb_platform_log_write -> g_Uart.Tx
    └── nRF52832/                                 per-MCU shared
        ├── include/
        │   ├── board.h                           UART pins, core clock
        │   └── kvb_config_nrf52832.h             board-specific KVB tunings
        ├── src/
        │   └── kvb_platform_nrf52832.cpp         board/CPU strings (DWT default
        │                                         provides the time source)
        └── TaktOS_nRF52832_KvbSuite/
            ├── README.md                         this file
            └── Eclipse/
                ├── .project                      links into all the above + KVB framework
                └── .cproject                     build config (xPack GNU Arm)
```

Eclipse linked-resource path variables resolve relative to the project's
Eclipse folder:

- `PARENT-2-PROJECT_LOC` → `KVB/Targets/nRF52832/`
- `PARENT-3-PROJECT_LOC` → `KVB/Targets/`
- `PARENT-4-PROJECT_LOC` → `KVB/` (framework root)

## Dependencies

This project links against two pre-built static libraries:

- **`libTaktOS_M4.a`** — built from `TaktOS_Dev/ARM/cm4/Eclipse/`.
- **`libIOsonata_nRF52832.a`** — built from
  `IOsonata/ARM/Nordic/nRF52/nRF52832/lib/Eclipse/`.

Both must be built (in the same Debug/Release configuration as this
project) before this project will link.

The startup code, vector table, and SystemInit come from
`libIOsonata_nRF52832.a`. The linker script
`gcc_nrf52832.ld` (or `nrf52832_xxaa.ld`) comes from
`${iosonata_loc}/IOsonata/ARM/Nordic/nRF52/ldscript/`.

Eclipse system properties `iosonata_loc` and `iocomposer_home` are
referenced as in the reference projects.

## Build

1. Build `TaktOS_Dev/ARM/cm4/` for Debug and/or Release.
2. Build IOsonata `nRF52832` static library for the same configuration.
3. Open this project in Eclipse:
   File → Import → Existing Projects into Workspace, root directory this
   folder.  Eclipse imports both `.project` and `.cproject`.
4. Project → Build Project.

Output ELF/HEX appears in `Eclipse/Debug/` or `Eclipse/Release/`. Flash
via the on-board J-Link OB.

## Console output

KVB results emit through `kvb_platform_log_write()`. The shared
`KVB/Targets/src/kvb_platform_iosonata.cpp` routes that to the
IOsonata `UART` C++ object `g_Uart`, which is configured in `main.cpp`
using the pin definitions from `board.h`:

- UART0 (`UART_DEVNO=0`)
- TX = P0.06, RX = P0.08, RTS = P0.05, CTS = P0.07
- 115200 8N1, DMA + interrupt mode (EasyDMA on nRF52832 UARTE)

Output appears on the host as `/dev/ttyACM*` (Linux), `/dev/cu.usb*`
(macOS), or `COMn` (Windows) at 115200 8N1.

## What runs

The runner brings up TaktOS and executes the registered KVB test set:

| Test | Purpose |
| --- | --- |
| `SCHED_COOP_001` | cooperative yield throughput |
| `SYNC_SEM_FAST_001` | uncontended semaphore wait/post throughput |
| `SYNC_MUTEX_FAST_001` | uncontended mutex lock/unlock throughput |
| `SYNC_MUTEX_OWNERSHIP_001` | mutex unlock by non-owner (validation) |
| `IPC_QUEUE_FAST_001` | queue send/receive same-thread throughput |
| `TIME_SLEEP_001` | thread sleep duration accuracy |

If the IPCP mutex patch (`ipcp_mutex_patch/`) is applied in the
TaktOS source tree, the `SYNC_MUTEX_PCP_001` test (once written) can
also run here — TaktOS is the only kernel of the three with
priority-ceiling support, so other variants will report it as
`KVB_ERR_UNSUPPORTED` and skip.

Results print as ASCII over UART0, then the runner halts.

## Memory budget

64 KB SRAM is comfortable.  See the comment block at the top of
`include/kvb_config_nrf52832.h` for the per-region budget — total usage
sits around 7 KB, leaving roughly 57 KB free for any user code added on
top of the benchmark suite.

## Status

Cortex-M4 / 64 MHz reference target.  Numbers go in this README and in
the KVB benchmark report once a measurement run is captured on hardware.
