# Thread-Metric on NUCLEO-H753ZI (TaktOS port)

This directory contains seven Eclipse projects that run the official
Eclipse ThreadX [Thread-Metric][tm] benchmark suite on a
**NUCLEO-H753ZI** board (STM32H753ZI, Arm Cortex-M7 + double-precision FPU
+ I/D cache + MPU, 2 MB Flash, 1 MB+ multi-region SRAM).

The kernel under test is **TaktOS**, linked in from the `TaktOS_M7` static
library produced by `ARM/cm7/Eclipse/`.

[tm]: https://github.com/eclipse-threadx/threadx/tree/master/utility/benchmark_application/thread_metric

---

## Tests included

| Project suffix                                | Threads | TM number |
|-----------------------------------------------|--------:|:---------:|
| `BasicProcessing`                             |    1    |    TM1    |
| `CooperativeScheduling`                       |    5    |    TM2    |
| `PreemptiveScheduling`                        |    5    |    TM3    |
| `MessageProcessing`                           |    1    |    TM6    |
| `SynchronizationProcessing`                   |    1    |    TM7    |
| `MutexProcessing`                             |    3    |    TM8    |
| `MutexBargingTest`                            |  8 + 1  |  ext.     |

TM4 (Interrupt Preemption) is not run — it requires a kernel-owned hardware
timer IRQ, which TaktOS does not have by design. TM5 (Memory Allocation) is
not run — TaktOS has no heap.

---

## Toolchain

- **Compiler:** xPack GNU Arm Embedded GCC, prefix `arm-none-eabi-`.
- **Build system:** Eclipse CDT with GNU MCU Eclipse plugin (same setup as
  the nRF52832 and Nano R4 ports).
- **C++ standard:** `-std=gnu++23`, `-fno-exceptions`, `-fno-rtti`.
- **Optimization:** `-Os` for both Debug and Release.

### Soft-float build

The `ARM/cm7/Eclipse/` kernel library currently has only `Debug` and
`Release` configurations (no `DebugFPU` / `ReleaseFPU`), i.e. it is
compiled **soft-float**. These benchmark projects therefore build the
firmware soft-float too — no `-mfpu`, no `-mfloat-abi`. Thread-Metric is a
pure integer workload, so the FPU plays no role in the numbers.

If a future caller needs the H7 double-precision FPU, add an analogous
`ReleaseFPU` config to `ARM/cm7/Eclipse/.cproject` (FPU unit
`fpv5-d16`, float ABI `hard`) and update these projects' library search
path and `-mfpu` / `-mfloat-abi` options to match. No other change is
required.

---

## Hardware setup

- Board: **STM32 NUCLEO-H753ZI** (ST order code `NUCLEO-H753ZI` — the
  variant with ST-LINK/V3).
- USB: plug the ST-LINK USB port (`CN1`) into the host. The ST-LINK/V3
  exposes both the SWD debug interface and a Virtual COM Port for UART
  output.
- Solder bridges: leave at factory default (`SB12` and `SB19` intact) so
  that USART3 on `PD8` (TX) / `PD9` (RX) routes to the ST-LINK VCP.

### Serial console

USART3 polling-mode, 115200 8N1, no flow control:

| MCU pin  | Signal       | Route            |
|----------|--------------|------------------|
| `PD8`    | USART3_TX    | ST-LINK/V3 VCP → host |
| `PD9`    | USART3_RX    | ST-LINK/V3 VCP ← host |

On the host:

- Linux:   `/dev/ttyACM0` or `/dev/ttyACM1`
- macOS:   `/dev/cu.usbmodem*`
- Windows: COM*n* — check Device Manager under "Ports (COM & LPT)".

Any terminal emulator works (minicom, picocom, screen, PuTTY, Tera Term).

---

## Clock configuration

The port deliberately **does not** touch the clock tree. On reset the
STM32H7 powers up with `HSI` as SYSCLK at 64 MHz and all prescalers at 1,
giving `SYSCLK = HCLK = PCLK1 = PCLK2 = 64 MHz`. `SystemInit()` performs
three actions only:

1. Set `FLASH_ACR.LATENCY = 1` (correct for VOS3 at up to 70 MHz; the
   reset default of 7 wait states is safe but unnecessarily slow).
2. Enable the Cortex-M7 I-cache (`SCB->CCR.IC`).
3. Enable the Cortex-M7 D-cache (`SCB->CCR.DC`), after invalidating by
   set/way using the geometry read from `CCSIDR`.

No PLL bring-up, no voltage-scale change, no FPU CPACR enable.

Running at HSI 64 MHz keeps the setup deterministic and minimal. If you
want the full 480 MHz H7 ceiling for a future deployment, extend
`system_stm32h753.c` with the HSE-sourced PLL1 configuration,
`PWR_D3CR.VOS = 0b11` (VOS1), `FLASH_ACR.LATENCY = 4`,
`WRHIGHFREQ = 0b10` — then update `TM_TAKTOS_CORE_CLOCK_HZ` in
`tm_port_taktos.cpp` and the `SystemCoreClock` global.

---

## Memory layout

| Region      | Base         | Size    | Use                         |
|-------------|--------------|---------|-----------------------------|
| FLASH       | `0x08000000` | 2 MB    | `.text`, `.rodata`, init values |
| DTCM-RAM    | `0x20000000` | 128 KB  | `.data`, `.bss`, MSP         |

Other STM32H7 SRAMs (ITCM 64 KB at `0x00000000`, AXI-SRAM 512 KB at
`0x24000000`, SRAM1-4 in D2/D3 power domains) are intentionally not
mapped — the benchmark's data footprint fits comfortably in DTCM and the
simpler memory map is easier to audit.

---

## Building

1. Open Eclipse CDT with the GNU MCU plugin.
2. File → Import → Existing Projects into Workspace.
3. Select the `STM32H753ZI/` folder and import all seven projects.
4. Also import `ARM/cm7/Eclipse/` (the TaktOS M7 library). Build it
   first — the benchmark projects link its output.
5. Select the `Release` configuration (picks up
   `TAKT_INLINE_OPTIMIZATION`, mandatory for the fast semaphore/mutex/
   queue paths — TM6 would otherwise be −14.4 %, TM7 −25.4 %).
6. Build each benchmark project.

Output: one `.elf` + `.hex` + `.bin` per project under
`<Project>/Eclipse/Release/`.

---

## Flashing

Any of the following tools work with NUCLEO-H753ZI / ST-LINK/V3:

- **STM32CubeProgrammer** (GUI or CLI, official ST tool).
- **OpenOCD** — upstream mainline has H7 support:
  `openocd -f board/st_nucleo_h7.cfg -c "program <file>.elf verify reset exit"`.
- **J-Link** — convert the onboard ST-LINK to J-Link OB firmware
  (free for NUCLEO boards, reversible via Segger's STLinkReflash utility).

---

## What to expect on the console

Each benchmark prints a 30-second rolling counter. Example for
`SynchronizationProcessing` (TM7):

```
**** Thread-Metric Synchronization Processing Test **** Relative Time: 30
Time period total: nnnnnnn
...
```

Higher numbers are better. Copy the final 30-second totals into the
report.

---

## Port layer internals

Files under `src/` in this folder:

| File                              | Role |
|-----------------------------------|------|
| `startup_stm32h753.S`             | Full STM32H753 vector table (Cortex-M sys exceptions + 150 peripheral IRQs per CMSIS `stm32h753xx`). Copies `.data`, zeroes `.bss`, calls `SystemInit`, `__libc_init_array`, `main`. No CPACR enable (soft-float). |
| `gcc_stm32h753.ld`                | Linker script: FLASH 2 MB, DTCM 128 KB, 4 KB MSP reserve. |
| `system_stm32h753.c`              | Caches + flash latency. No clock-tree changes. |
| `tm_console_nucleo_h753.cpp`      | USART3 polling console on `PD8/PD9`, AF7, 115200 8N1. |
| `tm_port_taktos.cpp`              | Thread-Metric ↔ TaktOS adapter. Slot-array thread design, priority mapping (TM 1 → TaktOS 31), TIM7-based software interrupt for `tm_cause_interrupt()`, static allocation for queue/semaphore/mutex. |

### Software-interrupt line

`tm_cause_interrupt()` writes `TIM7_IRQn = 55` into `NVIC_STIR`
(`0xE000EF00`) to pulse the NVIC pending bit. **TIM7 is never clocked**
(`RCC_APB1LENR.TIM7EN` stays 0) — only the NVIC pending bit is touched,
so no TIM7 peripheral event can interfere. `TIM7_IRQHandler` is provided
as a strong symbol in `tm_port_taktos.cpp` via `__attribute__((alias))`,
overriding the weak `Default_Handler` from the startup file.

### Interrupt priorities

Cortex-M7 on STM32H7 implements 4 NVIC priority bits (top 4 of each byte,
valid values `0x00` — `0xF0` in steps of `0x10`):

| Source            | Priority byte | Rationale |
|-------------------|--------------:|:---------|
| PendSV, SysTick   | `0xF0` (lowest) | TaktOS context switch should not preempt any app ISR |
| TIM7 SWI          | `0xC0`        | Fires, then tail-chains into PendSV if a higher-priority task was readied |
| (app ISRs)        | `0x00`–`0xB0` | Left to user code |

### Slot sizing

`tm_port_taktos.cpp` uses a compact slot array whose size is selected
per-project via the `TM_TAKTOS_MAX_SLOTS` macro (defined in the
`.cproject`). Values match the minimum required by each test:

| Test                        | Threads actually used | Slots |
|-----------------------------|----------------------:|------:|
| `BasicProcessing`           | 1                     | 2     |
| `CooperativeScheduling`     | 5                     | 6     |
| `PreemptiveScheduling`      | 5                     | 6     |
| `MessageProcessing`         | 2                     | 2     |
| `SynchronizationProcessing` | 2                     | 2     |
| `MutexProcessing`           | 3                     | 4     |
| `MutexBargingTest`          | 8 workers + 1 reporter| 9     |

Per-thread stack is 2 KB (`TM_TAKTOS_STACK_BYTES = 2048`) — generous for
M7 spill pressure, comfortably under the 128 KB DTCM budget even in the
9-thread case.

---

## Notes on methodology

- All configurations match the other Cortex-M ports in this repo:
  `-Os`, `-std=gnu++23`, `-fno-exceptions`, `-fno-rtti`,
  `TAKT_INLINE_OPTIMIZATION` defined for Release.
- TaktOS links against its own static library, exactly as user firmware
  would — there is no magic integration. See the top-level `README.md`
  section "How TaktOS integrates with your firmware".
- No PX5 comparison is published for this board: the PX5 demo package
  cannot be tuned to match the same optimization level, inlining, and
  kernel options as TaktOS/FreeRTOS/ThreadX. Running it as-is against a
  fully tuned benchmark would not be an apples-to-apples comparison.
  See the repo-top Methodology section.
