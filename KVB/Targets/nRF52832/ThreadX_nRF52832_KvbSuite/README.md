# ThreadX_nRF52832_KvbSuite

KVB benchmark + validation suite running on Eclipse ThreadX 6.x for the
Nordic Semiconductor nRF52-DK (PCA10040) board.

Same shape as the TaktOS counterpart project — see
`../TaktOS_nRF52832_KvbSuite/README.md` for board details.  This
README covers the ThreadX-specific bits.

## Configuration

- **`include/tx_user.h`** — ThreadX feature config.  `TX_ENABLE_STACK_CHECKING`,
  `TX_TIMER_PROCESS_IN_ISR`, error checking ON.  Same settings as the
  STM32F0308 build for cross-target comparability.
- **`include/kvb_config_nrf52832_threadx.h`** — KVB-side tunings.
  Worker stack at 1024 B (vs 512 B on TaktOS / FreeRTOS) — disclosed
  asymmetry from the `_txe_*` parameter-validation wrappers.

## Disclosed asymmetries

Same set as the STM32F0308 build, documented in the config file
header:

1. Worker stack 1024 B vs 512 B — `_txe_*` wrappers add a real C
   call frame to every public API.  M4's larger register file makes
   each frame smaller in cycles than on M0 but the structural overhead
   is the same.
2. Port-private static slot pool for control blocks (TX_THREAD,
   TX_SEMAPHORE, TX_MUTEX, TX_QUEUE) — required because ThreadX hangs
   control blocks on kernel-global linked lists for object lifetime.
3. Pool sizes (`KVB_THREADX_*_POOL_SIZE`) trimmed to test-suite peak.

## Dependencies

- **`libthreadx_m4.a`** — Eclipse ThreadX 6.x kernel built for Cortex-M4F.
  Use the upstream `ports/cortex_m4/gnu` source.
- **`libIOsonata_nRF52832.a`** — same as the TaktOS variant.
- **`src/tx_initialize_low_level.S`** in this folder — replaces the
  ThreadX example_build version, integrates with IOsonata's vector
  table and configures SysTick for 1000 Hz at 64 MHz.
- **`kvb_threadx_glue.c`** in `KVB/ports/kernels/threadx/` — provides
  the `PendSV_Handler -> __tx_PendSVHandler` alias.

## Build

1. Build Eclipse ThreadX `cortex_m4/gnu` port — but **exclude** the
   upstream `tx_initialize_low_level.S` and use this folder's version
   instead.  Same exclusion pattern as the STM32F0308 build (the M4
   tx_initialize_low_level.S override is in `src/`).
2. Build IOsonata `nRF52832` for the same configuration.
3. Import this Eclipse project, build.

## Test set

Same test list as the TaktOS variant.  Note:

- `SYNC_MUTEX_OWNERSHIP_001`: was reported FAIL on STM32F0308 with
  `KVB_THREADX_SEM_POOL_SIZE = 4` due to test sem leaks (see fairness
  patch open items).  This config keeps the same pool size for direct
  comparison; if you want the test to pass either bump the pool to 8
  or fix KVB tests to delete their sems on completion.
- `SYNC_MUTEX_PCP_001` (once written): reports `KVB_ERR_UNSUPPORTED` —
  ThreadX implements priority **inheritance** via `TX_INHERIT`, not
  priority **ceiling**.
