# TaktOS_STM32F0308_KvbSuite

KVB benchmark + validation suite running on TaktOS for the
STMicroelectronics STM32F0308-DISCO board.

## Board

| Item | Value |
| --- | --- |
| MCU | STM32F030R8T6 |
| Core | ARM Cortex-M0 (ARMv6-M) |
| Clock | 48 MHz (HSI + PLL) |
| Flash | 64 KB |
| SRAM | 8 KB |
| FPU | none |
| MPU | none |
| DWT | not present |
| Console | USART1 PA9/PA10 → ST-LINK VCOM, 115200 8N1 |

## Layout

This project follows the same shape as the nRF52832 / nRF54L15 reference
projects. Per-project Eclipse folder contains only `.project` and
`.cproject`; all sources are linked from upstream:

```
KVB/                                              framework root
├── src/                                          KVB tests + core
├── ports/                                        kernel + platform ports
├── include/                                      public KVB headers
└── Targets/
    ├── src/                                      shared across all MCUs
    │   ├── main.cpp                              IOsonata UART init, calls kvb_kernel_start
    │   └── kvb_platform_iosonata.cpp             kvb_platform_log_write -> g_Uart.Tx
    ├── include/                                  shared headers (none yet)
    └── STM32F0308/                               per-MCU shared
        ├── include/
        │   ├── board.h                           USART pins, core clock
        │   └── kvb_config_stm32f0308.h           board-specific KVB tunings
        ├── src/
        │   └── kvb_platform_stm32f0308.cpp       no-DWT time source, board/cpu strings
        └── TaktOS_STM32F0308_KvbSuite/
            ├── README.md                         this file
            └── Eclipse/
                ├── .project                      links into all the above + KVB framework
                └── .cproject                     build config (xPack GNU MCU plugin)
```

Eclipse linked-resource path variables resolve relative to the project's
Eclipse folder:

- `PARENT-2-PROJECT_LOC` → `KVB/Targets/STM32F0308/`
- `PARENT-3-PROJECT_LOC` → `KVB/Targets/`
- `PARENT-4-PROJECT_LOC` → `KVB/` (framework root)

No system-property setup is required.

## Dependencies

This project links against two pre-built static libraries:

- **`libTaktOS_M0.a`** — built from `TaktOS_Dev/ARM/cm0/Eclipse/`.
- **`libIOsonata_STM32F030x8.a`** — built from
  `IOsonata/ARM/ST/STM32F0xx/STM32F030x8/lib/Eclipse/`.

Both must be built (in the same Debug/Release configuration as this
project) before this project will link.

The startup code, vector table, and SystemInit come from
`libIOsonata_STM32F030x8.a`. The linker script
`gcc_stm32f030x.ld` comes from
`${iosonata_loc}/IOsonata/ARM/ST/STM32F0xx/ldscript/`.

Eclipse system properties `iosonata_loc` and `iocomposer_home` are
referenced as in the reference projects.

## Build

1. Build `TaktOS_Dev/ARM/cm0/` for Debug and/or Release.
2. Build IOsonata `STM32F030x8` static library for the same configuration.
3. Open this project in Eclipse:
   File → Import → Existing Projects into Workspace, root directory this
   folder. Eclipse imports both `.project` and `.cproject`.
4. Project → Build Project.

Output ELF/HEX appears in `Eclipse/Debug/` or `Eclipse/Release/`. Flash
via the on-board ST-LINK.

## Console output

KVB results emit through `kvb_platform_log_write()`. The shared
`KVB/Targets/src/kvb_platform_iosonata.cpp` routes that to the
IOsonata `UART` C++ object `g_Uart`, which is configured in `main.cpp`
using the pin definitions from `board.h`:

- USART1 (`UART_DEVNO=0`)
- TX = PA9 (AF1), RX = PA10 (AF1)
- 115200 8N1, DMA + interrupt mode

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

Results print as ASCII over USART1, then the runner halts.

## Memory budget

8 KB SRAM is the binding constraint. See the comment block at the top
of `include/kvb_config_stm32f0308.h` for the per-region budget and the
levers if more RAM is needed.

## Status

First KVB benchmark project. Numbers go in this README and in the
KVB benchmark report once a measurement run is captured on hardware.
