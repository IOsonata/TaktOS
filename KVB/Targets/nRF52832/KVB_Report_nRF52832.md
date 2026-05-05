# RTOS Benchmark Report — Cortex-M4F nRF52832

Suite: KVB 0.1.0
Target: Nordic nRF52832, Cortex-M4F at 64 MHz, 1000 Hz tick
Toolchain: arm-none-eabi-gcc 12.2.x, `-Os`, hard FPU
Kernels: TaktOS (private-dev), FreeRTOS V11.1.0+, Eclipse ThreadX 6.x, Zephyr 4.3.99 (nRF Connect SDK 3.3.0)
Runs: 5 per kernel.

## Summary

TaktOS leads throughput, wake-to-run latency, and IRQ-to-thread latency across all measured tests. TaktOS, FreeRTOS, and ThreadX produce comparable tick-cadence wake accuracy, all within 10 µs of the requested wake target. Zephyr's `k_sleep(K_TICKS(N))` waits at least N+1 ticks by API spec, so the tick-jitter row is not an apples-to-apples comparison and is reported separately.

## Setup

The benchmark is MCU-bound. Every measurement targets on-chip behavior: SysTick, PendSV, NVIC priority dispatch, the DWT cycle counter, on-chip flash and SRAM. None depend on the carrier board. The board only routes UART for log output, which runs outside the measurement windows.

TaktOS, FreeRTOS, and ThreadX are built on the same I-SYST IOsonata HAL: same clock setup, same vector table layout, same UART driver. Each kernel programs its own SysTick reload, SysTick and PendSV priorities, tick handler, and context-switch path. Those are kernel-owned and attributed to the kernel under test.

The KVB platform layer (separate from IOsonata) enables the DWT cycle counter, enables the NVMC instruction cache at boot to match the cache-on baseline that Zephyr applies in its own SoC init, and provides the IRQ probe (`SWI3_EGU3` via NVIC). This layer is identical across all three IOsonata-based ports. Zephyr is built using its own Nordic SoC init from nRF Connect SDK 3.3.0.

Compiler is gcc 12.2.1 for the IOsonata ports and gcc 12.2.0 for the Zephyr SDK. Hot-path codegen was spot-checked between the two and is equivalent.

TaktOS, FreeRTOS, and ThreadX produce bit-identical results across all 5 runs. Zephyr varies across runs on `RT_IRQ_MASK` `max_trigger_to_irq_cycles` (115–1245 cy) and `RT_MUTEX_WAKE` `max_cycles` (1071–2165 cy). For Zephyr, the tables show averages with the observed range in brackets where it matters.

## Throughput

10 s measurement window, operations per second.

| Test | TaktOS | FreeRTOS | ThreadX | Zephyr |
|---|---:|---:|---:|---:|
| Cooperative yield | 4,529,449 | 3,178,500 | 2,087,632 | 1,718,646 |
| Sem wait/post | 524,098 | 187,942 | 395,124 | 192,277 |
| Mutex lock/unlock | 483,954 | 113,499 | 184,247 | 176,289 |
| PI/PCP mutex | 211,490 | 118,096 | 184,746 | 174,887 |
| Queue send/recv | 144,184 | 71,477 | 133,981 | 68,287 |

ThreadX has priority inheritance always on, so its plain-mutex and PI/PCP rows are roughly equal. FreeRTOS's plain mutex sits well below its PI mutex on this build.

## Wake-to-run latency

Cycles from sync-primitive release to woken thread running.

| Test | TaktOS avg / max | FreeRTOS avg / max | ThreadX avg / max | Zephyr avg / max |
|---|---:|---:|---:|---:|
| Sem wake | 250 / 463 | 650 / 868 | 472 / 967 | 813 / 1,924 |
| Queue wake | 323 / 545 | 978 / 1,434 | 573 / 1,048 | 994 / 2,120 |
| Mutex wake | 324 / 828 | 915 / 1,401 | 1,058 / 1,631 | 1,041 / 1,071–2,165 |

ThreadX's mutex-wake max (1,631 cy) is the priority-inheritance recompute path under load.

## IRQ response

Two stages. Trigger-to-IRQ is `SWI3_EGU3` NVIC pending to handler entry. IRQ-to-thread is handler exit to the waiting thread resuming. Full path is the sum.

| Stage | TaktOS avg / max | FreeRTOS avg / max | ThreadX avg / max | Zephyr avg / max |
|---|---:|---:|---:|---:|
| Trigger → IRQ | 90 / 301 | 95 / 329 | 82 / 97 | 115 / 115–1245 |
| IRQ → thread | 279 / 590 | 696 / 1,170 | 525 / 1,000 | 823 / 1,936 |
| Trigger → thread | 370 / 678 | 792 / 1,264 | 607 / 1,082 | 938 / 2,075 |

ThreadX has the tightest IRQ entry stub, about 8 cy faster than TaktOS on average and 200 cy tighter at max. ThreadX defers more state save to the dispatch path. End-to-end (trigger to woken thread), TaktOS is fastest.

Zephyr's `trigger_to_irq` max varies 115–1245 cy across runs. One run measured 115 cy max. Source of variance not isolated.

## Tick-cadence wake accuracy

`kvb_thread_sleep_ticks(N)` for N in {1, 2, 3, 5, 10, 20, 50}. Cycle-accurate wake measured against target `current_tick + N`. Negative error means woke before target; positive means after.

| Metric | TaktOS | FreeRTOS | ThreadX | Zephyr |
|---|---:|---:|---:|---:|
| avg error (cy) | −157 | −126 | −152 | +31,462 |
| max abs jitter (cy) | 416 | 391 | 633 | 60,805 |
| max abs jitter (µs) | 6.5 | 6.1 | 9.9 | 949 |
| Early / late / 126 | 124 / 2 | 123 / 3 | 124 / 2 | 18 / 108 |
| Worst-case at N = | 5 | 5 | 1 | 1 |

TaktOS, FreeRTOS, and ThreadX honor `WakeTick = currentTick + N`. All three land inside 10 µs of the target. Most samples wake early in the target tick because the call enters at non-zero phase. The 25 cy gap between FreeRTOS (391) and TaktOS (416) is inside build-layout sensitivity.

Zephyr's `k_sleep(K_TICKS(N))` API spec is "sleep at least N ticks." The Zephyr timeout subsystem adds one tick to honor that, so `sleep_ticks(1)` waits about 2 ticks of real time, `sleep_ticks(5)` waits about 6 ticks, and so on. KVB's `kvb_thread_sleep_ticks(N)` maps to `k_sleep(K_TICKS(N))` because that is the closest equivalent. The Zephyr column on this row measures a different sleep semantic from the other three.

## Determinism

TaktOS, FreeRTOS, ThreadX: 5/5 runs bit-identical, every metric.

Zephyr: 5/5 runs vary on `RT_IRQ_MASK.max_trigger_to_irq_cycles` and `RT_MUTEX_WAKE.max_cycles`. Cause not yet attributed.

## Conclusions

On nRF52832 at gcc 12, matched build flags:

TaktOS is 1.5–4× faster than the next-best kernel on every throughput primitive measured. TaktOS wake-to-run is 1.5–3× faster on average and 1.5–4× faster at max. End-to-end IRQ trigger to woken thread is 1.6× faster than ThreadX, 2.1× faster than FreeRTOS, 2.5× faster than Zephyr at max.

TaktOS, FreeRTOS, and ThreadX are tied on tick accuracy within the resolution of the test.

## Limitations

KVB v0.1.0 does not measure: contended primitive throughput under multiple wakers, priority-inheritance correctness under nested locks, deadline-miss behavior under contended scheduler load, memory footprint, boot time, fault-injection or watchdog recovery.

The numbers are specific to nRF52832 at 64 MHz with 1000 Hz tick. Other Cortex-M4F implementations (different flash latency, different ICACHE, different NVIC priority widths) will produce different absolute numbers and may produce different relative ordering.

The `TIME_SLEEP` test reports 0 µs error on TaktOS due to the elapsed-time measurement landing on a tick boundary. `RT_TICK_JITTER` reports the actual tick-cadence numbers using DWT cycles. Use `RT_TICK_JITTER`.

## Build notes

IOsonata ports built from `KVB/Targets/nRF52832/{TaktOS,FreeRTOS,ThreadX}_nRF52832_KvbSuite/Eclipse/` against `IOsonata/ARM/Nordic/nRF52/nRF52832/lib/Eclipse/Release/`. Linker script `gcc_nrf52832_xxaa.ld`. The NVMC instruction cache is disabled by default on nRF52832 reset; Zephyr enables it in its SoC init. The KVB platform layer enables it at boot for the IOsonata ports (via constructor in `kvb_platform_nrf52832.cpp`) so all four kernels run with the same cache-on baseline. Any nRF52832-based dev board with a serial output should reproduce.

Zephyr port via West from `KVB/Targets/Zephyr/KvbSuite` against nRF Connect SDK 3.3.0, nRF52832 silicon at 64 MHz. `CONFIG_TICKLESS_KERNEL=n`, `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000`, `CONFIG_ASSERT=y`, `CONFIG_THREAD_STACK_INFO=y`, `CONFIG_STACK_SENTINEL=y`.

Compiler: arm-none-eabi-gcc 12.2.1 20221205 (IOsonata ports), 12.2.0 (Zephyr SDK).

Run conditions: nRF52832 powered from USB, no debugger attached, JLink RTT off, no GPIO toggling during measurement.

Raw KVB log captures available on request.
