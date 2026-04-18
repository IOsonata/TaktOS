# TaktOS MPU / RAM vector relocation test — nRF54L15

This Eclipse managed project validates the ARMv8-M MPU handler path on
Cortex-M33 (nRF54L15 @ 128 MHz).  It is the sister project to
`TaktOSMpuVectorRelocTest_nRF52832` (Cortex-M4 @ 64 MHz) and exercises the
same flow against the new `takt_mpu_v8m.h` / `PendSV_Handler_MPU` /
`SVC_Handler_MPU` implementation.

## What it tests

- The application relocates the active vector table to RAM.
- `TaktOSInit(..., HandlerBaseAddr)` receives the RAM vector base.
- The ARM port patches only the kernel-reserved slots in RAM:
  - SVCall
  - PendSV
  - SysTick
- `SVC_Handler_MPU` programs PSPLIM and MPU region 7 before the first task
  runs, and `PendSV_Handler_MPU` re-programs both on every context switch.
- The scheduler runs normally after the relocation.
- A deliberate indexed write into the current thread's 32-byte guard
  region triggers `MemoryManagement_Handler`.

## Why this is a stronger test on M33 than on M4

The fault probe is an indexed write at `pStackBottom` while the task's SP
is well above the limit.  On ARMv8-M this case is only caught by MPU
region 7 — PSPLIM cannot see it because SP does not descend.  A passing
result proves that the new `takt_mpu_v8m.h` RBAR + RLAR encoding is
correctly installed and that AP = 10 (RO-priv, XN = 1) is faulting writes
as intended.

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
- `gMemManageMmar` should match the fault thread guard base (the
  pStackBottom address of the running thread)
- `gMemManageCfsr` should have the MMFARVALID and DACCVIOL bits set
  (CFSR bit 7 and bit 1 of the MMFSR byte)

## ARMv8-M-specific expectations

- Encoding in MPU region 7 after the first context switch:
  - `MPU_RBAR` = `pStackBottom | 0x05`   (SH = 00, AP = 10 RO-priv, XN = 1)
  - `MPU_RLAR` = `pStackBottom | 0x01`   (AttrIndx = 0 Normal-WB, EN = 1)
- `MPU_CTRL` = `PRIVDEFENA | ENABLE` (`0x05`)
- `MPU_MAIR0[7:0]` = `0xFF` (Normal Inner/Outer WB non-transient,
  Read + Write allocate)
- `MPU_RNR` = `7`

A handy sanity check in the debugger before triggering the fault:
read `MPU_RNR` and then `MPU_RBAR`/`MPU_RLAR`; the RBAR low nibble
should be `0x5` and the RLAR low bit should be `1`.  If either shows 0,
the MPU handler variants were not bound.

No UART or board support code is required.
