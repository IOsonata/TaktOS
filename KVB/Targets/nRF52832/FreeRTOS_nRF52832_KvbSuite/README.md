# FreeRTOS_nRF52832_KvbSuite

KVB benchmark + validation suite running on FreeRTOS V11.3 for the
Nordic Semiconductor nRF52-DK (PCA10040) board.

Same shape as the TaktOS counterpart project — see
`../TaktOS_nRF52832_KvbSuite/README.md` for board details.  This
README covers the FreeRTOS-specific bits.

## Configuration

- **`include/FreeRTOSConfig.h`** — kernel config.  Pure static
  allocation (`configSUPPORT_STATIC_ALLOCATION = 1`,
  `configSUPPORT_DYNAMIC_ALLOCATION = 0`, `configTOTAL_HEAP_SIZE = 0`).
  Mirrors the proven `Benchmark/ThreadMetric/nRF52832` FreeRTOS
  reference build.  No heap_4 needed on this target — the
  configSUPPORT_DYNAMIC_ALLOCATION=1 + heap_4 workaround used on
  STM32F0308 is M0-specific and does not apply on Cortex-M4F.
- **`include/kvb_config_nrf52832_freertos.h`** — KVB-side tunings
  matching the TaktOS counterpart.  Same tick rate (1000 Hz), same
  measurement window (10 s), same throughput batch (256), same UART
  FIFO size (256 B), same runner stack (1024 B), same worker stack
  (512 B).  Like-for-like vs the TaktOS variant by construction.

## Dependencies

- **`libfreertos_m4.a`** — FreeRTOS V11.3 kernel built for Cortex-M4F.
  Use the V11.3 ARM_CM4F port from FreeRTOS-Kernel/portable/GCC/ARM_CM4F.
- **`libIOsonata_nRF52832.a`** — same as the TaktOS variant.

Linker script and startup come from IOsonata.  FreeRTOS handlers
`vPortSVCHandler` / `xPortPendSVHandler` / `xPortSysTickHandler` are
aliased to the standard CMSIS names via `FreeRTOSConfig.h` so they
override the weak IOsonata defaults at link.

## Build

1. Build FreeRTOS-Kernel ARM_CM4F port as `libfreertos_m4.a`.
2. Build IOsonata `nRF52832` for the same configuration.
3. Import this Eclipse project, build.

## Test set

Same test list as the TaktOS variant.  Note:

- `SYNC_MUTEX_OWNERSHIP_001`: passes on FreeRTOS V11.3.
- `SYNC_MUTEX_PCP_001` (once written): reports `KVB_ERR_UNSUPPORTED` —
  FreeRTOS implements priority **inheritance**, not priority **ceiling**.
- `TIME_SLEEP_001`: passes the lenient bound (≥9 ms ≥ min_expected for
  vTaskDelay(10) on a 1 ms tick).
