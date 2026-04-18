
# nRF54L15 Thread-Metric ports cloned from the working TaktOS suite

This archive mirrors the 6-test subset from the working TaktOS nRF54L15 suite:
- BasicProcessing
- CooperativeScheduling
- MemoryAllocation
- MessageProcessing
- PreemptiveScheduling
- SynchronizationProcessing

What was changed:
- `tm_port.cpp` ported for FreeRTOS, ThreadX, and PX5
- project names renamed from `TaktOS` to the target kernel
- `main.cpp` changed only for PX5 (`px5_pthread_start`)

What is still placeholder / local-environment dependent:
- Eclipse include paths for the RTOS headers
- Eclipse linker library names / search paths
- the actual nRF54L15 RTOS library / kernel integration in your tree

Assumptions:
- FreeRTOS build enables static allocation (`xTaskCreateStatic`, `xQueueCreateStatic`, `xSemaphoreCreateCountingStatic`)
- ThreadX port tick rate is configured to 1000 Hz to match the original TaktOS suite shape
- PX5 provides POSIX pthread/semaphore/message-queue support plus `px5_pthread_start`, `px5_pthread_suspend`, and `px5_pthread_resume`
