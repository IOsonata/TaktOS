# ThreadMetricBenchmarkFreeRTOS

Official Thread-Metric MutexProcessing comparison project for nRF52832 + FreeRTOS.

## Important
This project is packaged to match the Eclipse style of your TaktOS benchmark project,
but **you must place a FreeRTOS-Kernel checkout inside this project** before building.

Expected layout after extraction:

- ThreadMetricBenchmarkFreeRTOS/
  - Eclipse/
  - src/
  - FreeRTOS-Kernel/
    - include/
    - portable/GCC/ARM_CM4F/
    - tasks.c
    - queue.c
    - list.c

## UART output
This version uses **UARTE0 TX on P0.07** at **115200 baud**.
Connect board TX(P0.07) -> USB/UART RX and GND -> GND.

## Notes
- Same official Thread-Metric MutexProcessing test source as the TaktOS project.
- Same 100 Hz tick assumption.
- The project links against the same external nRF52 support/linker environment as your existing project.
- FreeRTOS sources are referenced as linked resources from `FreeRTOS-Kernel/`.

Selected test source: `synchronization_processing.c`
