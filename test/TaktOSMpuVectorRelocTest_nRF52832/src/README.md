# TaktOS MPU / RAM vector relocation test — nRF52832

This Eclipse managed project validates the new handler-base path on Cortex-M4.

## What it tests

- The application relocates the active vector table to RAM.
- `TaktOSInit(..., HandlerBaseAddr)` receives the RAM vector base.
- The ARM port patches only the kernel-reserved slots in RAM:
  - SVCall
  - PendSV
  - SysTick
- The scheduler runs normally after the relocation.
- A deliberate write to the current thread guard base triggers `MemoryManagement_Handler`.

## Debug checklist

Run under the debugger and inspect these globals:

- `gVectorRelocated == 1`
- `gMpuHandlersBound == 1`
- `gProducedCount` and `gConsumedCount` keep increasing

After about 10 fault-thread wake cycles, the test auto-sets:

- `gTriggerMpuFault = 1`

Expected result:

- `gMemManageFaultCount == 1`
- execution halts in `MemoryManagement_Handler`
- `gMemManageMmar` should match the fault thread guard base

No UART or board support code is required.
