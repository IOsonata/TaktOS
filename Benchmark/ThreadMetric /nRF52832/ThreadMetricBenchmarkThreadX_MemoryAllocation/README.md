# ThreadMetricBenchmarkThreadX

TM1 cooperative scheduling comparison project for nRF52832 + Eclipse ThreadX.

## Important
This project expects a ThreadX source tree under `threadx/` before building.
Expected layout:

- threadx/
  - common/inc/
  - common/src/
  - ports/cortex_m4/gnu/inc/
  - ports/cortex_m4/gnu/src/

## UART output
UARTE0 TX on **P0.07** at **115200 baud**.

## Notes
- This is a best-effort project skeleton for comparison work.
- It uses the official Thread-Metric TM1 cooperative scheduling test.
- For comparison with Beningo, use **Release / optimize for speed**.

Be sure the GNU Cortex-M4 port file `threadx/ports/cortex_m4/gnu/src/tx_initialize_low_level.S` is present and built.

Selected benchmark source: `memory_allocation.c`
