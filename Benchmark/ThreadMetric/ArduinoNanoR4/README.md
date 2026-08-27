# TaktOS Thread-Metric Benchmarks — Arduino Nano R4

Port of the official Thread-Metric benchmark suite to the **Arduino Nano R4**
(Renesas RA4M1, ARM Cortex-M4F @ 48 MHz, 256 KB flash, 32 KB SRAM).

## Target summary

| Item             | Value                                       |
| ---------------- | ------------------------------------------- |
| MCU              | Renesas RA4M1 (R7FA4M1AB3CFM)               |
| Core             | ARM Cortex-M4 with FPU (FPv4-SP-D16) + DSP  |
| Clock            | 48 MHz HOCO                                 |
| Flash / SRAM     | 256 KB / 32 KB                              |
| MPU              | 8 regions                                   |
| TaktOS library   | `TaktOS_M4` ReleaseFPU (hard float ABI)     |
| Toolchain        | xPack `arm-none-eabi-gcc` 15.2.1            |
| C++ standard     | GNU C++23 (`-std=gnu++23`)                  |
| Tick rate        | 1000 Hz                                     |

## Projects

Seven Eclipse CDT managed-build projects, one per Thread-Metric test:

| Project                                                | Slots | Test source              |
| ------------------------------------------------------ | ----- | ------------------------ |
| `ThreadMetricBenchmarkTaktOS_BasicProcessing_Nano_R4`  | 2     | basic_processing.c       |
| `ThreadMetricBenchmarkTaktOS_CooperativeScheduling_Nano_R4` | 6 | cooperative_scheduling.c |
| `ThreadMetricBenchmarkTaktOS_PreemptiveScheduling_Nano_R4`  | 6 | preemptive_scheduling.c  |
| `ThreadMetricBenchmarkTaktOS_MessageProcessing_Nano_R4`     | 2 | message_processing.c     |
| `ThreadMetricBenchmarkTaktOS_SynchronizationProcessing_Nano_R4` | 2 | synchronization_processing.c |
| `ThreadMetricBenchmarkTaktOS_MutexProcessing_Nano_R4`       | 4 | mutex_processing.c       |
| `ThreadMetricBenchmarkTaktOS_MutexBargingTest_Nano_R4`      | 9 | mutex_barging_test.c     |

Thread-Metric tests TM4 (interrupt preemption) and TM5 (memory pool) are not
run — TM4 requires a kernel-owned IRQ that TaktOS does not expose, and TM5
requires a heap which TaktOS does not provide.

## Shared source (`src/`)

| File                        | Purpose                                                |
| --------------------------- | ------------------------------------------------------ |
| `startup_ra4m1.S`           | Cortex-M4 vector table (16 sys + 32 ICU IRQs), Reset_Handler, FPU CPACR enable |
| `gcc_ra4m1.ld`              | 256K flash @ 0x00000000, 32K SRAM @ 0x20000000        |
| `system_ra4m1.c`            | `SystemInit()` — PRCR unlock, OPCCR high-speed, MEMWAIT=1, HOCO select, SCKDIVCR=0, SCKSCR=HOCO |
| `tm_console_nano_r4.cpp`    | SCI2 polling console on P301 / P302 (D1/D0), 115200 8N1 |
| `tm_port_taktos.cpp`        | TaktOS port layer, ICU slot 31 for `tm_cause_interrupt` |

## Console wiring

The Arduino Nano R4's **native USB-C** is driven by the RA4M1's on-chip USBFS
peripheral. Using it from a bare-metal benchmark would require standing up a
full USB-CDC stack from scratch — deliberately out of scope.

Instead, SCI2 is routed to the D1 / D0 header pins (the same "Serial1" mapping
used by the UNO R4 Minima Arduino BSP). Connect an **external USB-UART
adapter** (CP2102, FTDI, CH340, etc.) as follows:

```
  Nano R4                       USB-UART adapter
  ────────                      ────────────────
  D1 (P301, TXD2)   ────────>   RX
  GND               ────────    GND
  D0 (P302, RXD2)                (not used by these benchmarks)
```

Baud rate: **115200 8N1**. At 48 MHz PCLKB and `BRR = 12` the actual baud is
115384 (+0.16 % — well inside UART tolerance).

## Flashing

These projects link at **flash origin `0x00000000`**, which is the right
choice when flashing over **SWD** (e.g. J-Link Mini EDU or CMSIS-DAP on the
Nano R4's SWD pads). If you want to upload over the RA4M1's native USB-DFU
bootloader instead, edit `src/gcc_ra4m1.ld`:

```ld
FLASH (rx)  : ORIGIN = 0x00004000, LENGTH = 240K
```

The 16 KB at the bottom of flash is then reserved for the Arduino DFU
bootloader.

## Build prerequisites

- xPack GNU Arm Embedded GCC on `$PATH` (`arm-none-eabi-gcc` 15.x).
- Eclipse CDT with the Embedded CDT (GNU MCU Eclipse) plug-in.
- The **`TaktOS_M4 / ReleaseFPU`** static library already built in
  `ARM/cm4/Eclipse/ReleaseFPU/libTaktOS_M4.a`. The projects' linker path
  `../../../../../../ARM/cm4/Eclipse/ReleaseFPU` resolves there from the
  project's `Eclipse/` directory.

Unlike the nRF52832 projects, the Nano R4 port does **not** link IOsonata —
IOsonata has no Renesas RA4M1 support at the time of writing, and everything
the benchmark needs (startup, clock, UART, interrupts) is provided locally
in `Arduino_Nano_R4/src/`.

## RA4M1-specific design notes

### Clock bring-up
`SystemInit()` unlocks PRCR, sets OPCCR to high-speed mode (required above
24 MHz), sets `MEMWAIT = 1` (required above 32 MHz), enables HOCO (idempotent
if OFS1 already started it), waits for `OSCSF.HOCOSF = 1`, sets
`SCKDIVCR = 0` (all peripheral clocks `/1`), then switches `SCKSCR` to HOCO
(source 0b000). The Arduino factory OFS1 configures HOCO for 48 MHz; if a
user reprograms OFS1 to a different HOCO frequency, `SystemCoreClock` and
the BRR divisor must be recomputed.

### ICU IRQ for `tm_cause_interrupt()`
The RA4M1 ICU multiplexes up to 32 peripheral events onto NVIC IRQs 0..31
via `IELSR0..IELSR31`. This port uses **slot 31** with `IELSR31.IELS = 0`
(no peripheral event assigned) — so the only way slot 31 ever fires is
through a software trigger. `tm_cause_interrupt()` writes NVIC STIR with IRQ
number 31, which is a one-instruction software trigger on Cortex-M4.
`IEL31_IRQHandler` is provided as a strong alias to `tm_irq_vector_handler`
in the port, overriding the weak alias from the startup file.

### FPU enablement
CPACR bits CP10 and CP11 are set to `0b11` (full access) in `Reset_Handler`
**before any C code runs**, because the `TaktOS_M4 / ReleaseFPU` library
and any `-mfloat-abi=hard` code will execute VFP instructions as soon as
the first C function runs.

Context-switch FPU handling (lazy stacking for S0–S15+FPSCR, explicit save
of S16–S31 gated on `EXC_RETURN.FPU`) is already correct in
`ARM/cm4/PendSV_M4.S` and needs nothing extra here.

### Priority bits
RA4M1 implements 4 NVIC priority bits (vs. 2 on the STM32F0308). PendSV and
SysTick are set to `0xF0` (lowest priority); the ICU slot-31 software
interrupt is set to `0xC0` so it runs ahead of PendSV and the scheduler
tail-chains in afterwards if a higher-priority task was readied.

## Open items

- **On-target validation not yet done.** The port compiles and follows the
  same pattern as the nRF52832 / STM32F0308 ports, but Thread-Metric runs on
  real Nano R4 hardware are pending.
- No analogous FreeRTOS / ThreadX Nano R4 projects are generated here — add
  them separately if a head-to-head set is wanted on this target.
