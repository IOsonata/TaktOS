# Thread-Metric — TaktOS on ATSAM4LC8C

Cortex-M4 @ 48 MHz, no FPU, soft-float ABI. Mirrors the nRF52832 / nRF54L15
benchmark suites in structure; uses the same Thread-Metric sources under
`Benchmark/ThreadMetric/src/` and the same shared `tm_report.c`.

## Tests included

Same six TaktOS tests as the nRF52832 port — MemoryAllocation is intentionally
excluded (TaktOS has no heap; `tm_memory_pool_*` runs on a static free-list
inside `tm_port_taktos`):

- `ThreadMetricBenchmarkTaktOS_BasicProcessing_SAM4LC8C`
- `ThreadMetricBenchmarkTaktOS_CooperativeScheduling_SAM4LC8C`
- `ThreadMetricBenchmarkTaktOS_MessageProcessing_SAM4LC8C`
- `ThreadMetricBenchmarkTaktOS_MutexProcessing_SAM4LC8C`
- `ThreadMetricBenchmarkTaktOS_PreemptiveScheduling_SAM4LC8C`
- `ThreadMetricBenchmarkTaktOS_SynchronizationProcessing_SAM4LC8C`

## Eclipse project settings (both Debug and Release configs)

- **MCU**: `-mcpu=cortex-m4 -mthumb`
- **FPU / float ABI**: `-mfloat-abi=soft`, FPU Type = *none* — SAM4L has no FPU
- **C++ standard**: `-std=gnu++23`
- **Defined symbols**: `__SAM4LC8C__`, `__PROGRAM_START`, `TAKT_INLINE_OPTIMIZATION`
- **Linker script**: `${iosonata_loc}/IOsonata/ARM/Microchip/SAM4L/ldscript/gcc_sam4lx8.ld`
- **Libraries**: `TaktOS_M4`, `IOsonata_SAM4LCxC`
- **Library search paths**: `${iosonata_loc}/IOsonata/ARM/Microchip/SAM4L/SAM4LCxC/lib/Eclipse/<Debug|Release>`
  plus the local `ARM/cm4/Eclipse/<config>` for `libTaktOS_M4.a`

If your IOsonata tree places the compiled SAM4L library at a different path,
edit that single `listOptionValue` in `.cproject`.

## Port-layer design (`src/tm_port_taktos.cpp`)

- **Scheduler / tick / priorities**: 1000 Hz tick via TaktOS, PendSV and SysTick
  set to 0xFF (lowest priority), SW IRQ set to 0xC0 so it preempts tasks and
  PendSV tail-chains to complete any scheduling work.
- **Core clock**: 48 MHz expected after IOsonata's `SystemInit`.
  Change `TM_TAKTOS_CORE_CLOCK_HZ` if different.
- **`tm_cause_interrupt`**: SAM4L has no STIR and no dedicated SWI peripheral.
  The port borrows the TRNG NVIC line (IRQ **73**) and pends it via `NVIC_ISPR`.
  The TRNG peripheral itself is never clocked or configured — only its NVIC
  slot is used. The handler is installed by weak-aliasing `TRNG_Handler` to
  `tm_irq_vector_handler`, so it drops into the IOsonata CMSIS startup vector
  table without edits.

## Console output: you must provide tm_putchar + tm_hw_console_init

**This is the one piece of glue you need to add to each project.** The port
layer ships with weak no-op fallbacks for `tm_putchar()` and
`tm_hw_console_init()`, so the benchmark itself will run to completion on any
SAM4L board regardless of UART wiring — but the Thread-Metric report text
needs a working UART to actually appear on your terminal.

**Why it's not hardwired:** the SAM4L peripheral memory map and PM clock
unlock sequence differ from SAM3/4S/4N/4E. Accessing a clock-disabled
peripheral on SAM4L hangs the core hard. IOsonata's `UARTDev_t` driver
already handles clock gating, pin-muxing, baud calculation, and polling
correctly — reusing that is much safer than duplicating the register-level
work in the port layer.

### Drop-in template (SAM4L8 Xplained Pro)

A ready-made example is in `tm_console_iosonata_sam4l8xpro.cpp` at the top of
this folder, pre-configured for the **SAM4L8 Xplained Pro** EDBG Virtual COM
Port wiring:

- **USART1**
- **PC27** -> USART1 TXD  (SAM4L8 TX line out of the MCU)
- **PC26** -> USART1 RXD  (SAM4L8 RX line into the MCU)

Source: *Atmel SAM4L8 Xplained Pro User Guide* (Atmel-42103B), Section 4.3.2.
Note this is different from the SAM4L-EK wiring (USART2 on PC11/PC12).

**Known caveat — peripheral mux function letter:** the `PINOP` field selects
which of the 8 peripheral functions (A-H = 0-7) maps the pin to USART1. The
template defaults to `0` (function A), which is the primary mapping for most
SAM4L pins. If output still does not appear after wiring the right USART and
pins, try `UART_CFG_TX_PINOP = 1` (B), then 2 (C), etc. The definitive value
is in Table 3-1 "GPIO Controller Function Multiplexing - 100-pin Package" of
the SAM4L datasheet, or in the ASF board file
`sam/boards/sam4l8_xplained_pro/sam4l8_xplained_pro.h` as `COM_PORT_PIN_TX_MUX`.

To use the template:

1. Copy that file into one project's `src/` folder (next to `main.cpp`).
2. Edit the `UART_CFG_*` defines at the top to match your board's UART and
   pin wiring.
3. Rebuild. The strong symbols defined in the override file automatically
   take precedence over the weak no-op fallbacks at link time.

Repeat per project, or add it as a linked resource across all six projects
the same way `tm_port_taktos.cpp` is linked.

### Advanced: direct register access (not recommended)

If you really want the port to drive the USART directly without IOsonata,
define `TM_SAM4L_ENABLE_DIRECT_UART=1` as a project symbol. The register
addresses, PBA clock mask bit, and pin-mux selectors in `tm_port_taktos.cpp`
have **NOT** been verified against the SAM4L datasheet — they were reasoned
from SAM3/4S conventions and may not be correct. Expect to step through with
a debugger and fix any mismatched base addresses, offsets, or bit positions
against the SAM4L datasheet memory map before this path will work.

The included `tm_putchar` under this path has a 1,000,000-iteration safety
timeout so it won't hang the whole benchmark if the USART doesn't come up.

## File layout

```
Benchmark/ThreadMetric/SAM4LC8C/
├── README.md
├── tm_console_iosonata_sam4l8xpro.cpp    (copy into each project's src/)
├── src/
│   ├── tm_port_taktos.c                (excluded from build, kept for reference)
│   └── tm_port_taktos.cpp              (the active port)
└── ThreadMetricBenchmarkTaktOS_<test>_SAM4LC8C/
    ├── src/main.cpp                    (tiny shim: hw init -> tm_main)
    └── Eclipse/
        ├── .project                    (links shared port + test source)
        ├── .cproject                   (M4 / soft-float / IOsonata_SAM4LCxC)
        ├── .gitignore
        └── .settings/language.settings.xml
```
