# KVB Benchmark Report — STM32F0308-DISCO

**Target:** STM32F030R8 on STM32F0308-DISCO board
**CPU:** ARM Cortex-M0 (ARMv6-M)
**Clock:** 48 MHz from HSI + PLL (configured by IOsonata SystemInit)
**SRAM:** 8 KB
**Flash:** 64 KB
**Compiler:** xPack arm-none-eabi-gcc 12.2.1, `-Os`, Release build
**Date:** 2026-04-27
**Document version:** 4.0 (10 s window, 5-run aggregate, ThreadX added, leak-fix + TaktOS IPCP)

Author: Nguyen Hoan Hoang, I-SYST inc.

---

## 1. Summary

This report presents head-to-head KVB benchmark results between **TaktOS**
(this work, MIT, certification target IEC 61508 SIL 2 → ISO 26262 ASIL D),
**FreeRTOS V11.3** (Amazon/AWS, MIT), and **Eclipse ThreadX 6.x**
(Microsoft, MIT) on the same physical hardware, the same C/C++ application
code, the same UART, the same compiler, and the same KVB test framework.
Only the kernel under test differs.

| Test ID                  |   TaktOS  | FreeRTOS V11.3 | ThreadX 6.x | TaktOS vs FreeRTOS | TaktOS vs ThreadX |
|--------------------------|----------:|---------------:|------------:|-------------------:|------------------:|
| SCHED_COOP_001 (yields)  | 2,518,433 |       1,775,300 |   1,196,797 |          **1.42×** |         **2.10×** |
| SYNC_SEM_FAST_001 (p/s)  |   244,349 |         112,654 |     196,295 |          **2.17×** |         **1.24×** |
| SYNC_MUTEX_FAST_001 (p/s)|   176,040 |          78,097 |      88,352 |          **2.25×** |         **1.99×** |
| SYNC_MUTEX_OWNERSHIP_001 |      PASS |            PASS |        PASS |             parity |            parity |
| IPC_QUEUE_FAST_001 (p/s) |    79,104 |          39,316 |      64,623 |          **2.01×** |         **1.22×** |
| TIME_SLEEP_001           |  PASS @10476 µs | PASS @10005 µs | PASS @9537 µs | strict-bound: TaktOS+FreeRTOS PASS, ThreadX FAIL | strict-bound only-PASS |

Suite outcome (all three kernels):
- TaktOS:          6 PASS / 0 FAIL / 6 total
- FreeRTOS V11.3:  6 PASS / 0 FAIL / 6 total
- ThreadX 6.x:     6 PASS / 0 FAIL / 6 total

All three kernels pass every KVB test at the documented thresholds.
TIME_SLEEP_001 has new behavior in this measurement: with the
KVB test-cleanup fix landed (see §3.5 / §9), FreeRTOS V11.3 now
satisfies the stricter at-least-N-ticks contract by a 5 µs margin
(10005 µs ≥ 10000 µs nominal). TaktOS remains comfortably above the
strict bound. Only ThreadX undershoots at 9537 µs — within KVB's
documented 90 % threshold (9000 µs floor) but not the strict bound.
See §6.

`p/s` = wait/post or send/recv pairs per second. Numbers are aggregated
over 5 separate runs per kernel; per-metric run-to-run variance is
reported in §4 alongside each measurement.

---

## 2. What this report measures

KVB measures **end-to-end iteration throughput** of small workloads
designed to exercise specific kernel features. Each throughput test
runs a tight loop (yield, sem post/wait, mutex lock/unlock, queue
send/receive) for a fixed measurement window and reports completed
iterations per second.

This is the same measurement model used by Thread-Metric (Express
Logic / Eclipse ThreadX, the de-facto cross-RTOS benchmark since
the early 2000s). Both KVB and Thread-Metric report iterations per
second; neither reports per-primitive cycle counts. The "per-operation
cost" interpretation (e.g. "3.97 µs per semaphore wait/post pair on
TaktOS") is inferred by dividing one second by the iteration count,
and is accurate only to the extent that the loop body around the
primitive is small relative to the primitive itself — a property
KVB's tests are written to satisfy, but a property the reader should
be aware of.

KVB's tests are deliberately scoped narrower than Thread-Metric's:
KVB's SYNC_MUTEX_OWNERSHIP_001 verifies the kernel's behavioural
specification, not throughput; KVB's TIME_SLEEP_001 verifies a
duration lower bound, not throughput. These are PASS/FAIL behavioural
tests, distinct from the four throughput tests.

For **cycle-accurate primitive-level analysis**, see the TaktOS
engineering specification §4.4 (ARM ~47 cycles) and §5.4 (RISC-V ~88
cycles Zbb / ~98 cycles no-Zbb), derived from llvm-mca static
pipeline analysis of the actual context-switch instruction sequences.
For **higher-throughput Thread-Metric numbers on faster targets**,
see the Thread-Metric runs on nRF52832 (Cortex-M4 @ 64 MHz) and
nRF54L15 (Cortex-M33 @ 128 MHz) elsewhere in the TaktOS benchmark
suite. KVB does NOT report cycle counts because Cortex-M0 lacks DWT —
there is no hardware cycle counter available on this part for any
benchmark to use.

Both kernels run identical workloads through identical KVB
infrastructure; differences in reported iterations/second therefore
reflect the difference between the two kernels' implementations of
the primitive(s) being exercised, plus a small constant for the
fixed loop body that is the same for both.

---

## 3. Methodology

### 3.1 Hardware

Single physical STM32F0308-DISCO board, ST-LINK/V2-1 onboard. UART
output captured at 115200 8N1 over the ST-LINK virtual COM port. Same
board power-cycled between TaktOS, FreeRTOS, and ThreadX firmware loads.
Each firmware ran 5 separate boot cycles (TaktOS, ThreadX) or 6
(FreeRTOS — one extra run after the test-cleanup fix landed) so
run-to-run determinism could be characterised.

### 3.2 Common across all three kernels

- **CPU clock:** 48 MHz (HSI + PLL via IOsonata SystemInit, identical
  initialisation path)
- **Tick rate:** 1000 Hz
- **Compiler:** xPack arm-none-eabi-gcc 12.2.1, `-Os -mcpu=cortex-m0
  -mthumb -mfloat-abi=soft`, Release configuration
- **Linker script:** `gcc_stm32f030x.ld` (identical, IOsonata-supplied)
- **Startup file:** IOsonata Vectors_STM32F030 (identical)
- **C library:** newlib-nano with `--specs=nano.specs --specs=nosys.specs`
- **HAL:** IOsonata UART driver, identical UART FIFO sizes
- **KVB framework:** identical sources, identical KVB test bodies
- **Worker stack size:** `KVB_DEFAULT_STACK_SIZE = 256` bytes (usable)
- **Worker count:** `KVB_WORKER_THREAD_COUNT = 3`
- **Measurement window:** **10000 ms (10 s)** per throughput test.
  This was a deliberate revision from an earlier 1-second window:
  with 10 seconds, the per-run tick-edge slop drops from ~0.4 % of
  window to ~0.04 %, and the test setup transient (thread creation,
  semaphore allocation, runner pre-measurement work) drops from a
  few percent to <0.5 % of the measured window. The earlier 1-second
  numbers were dominated by setup transient and significantly
  understated steady-state throughput on both kernels — the gap on
  the SYNC_SEM benchmark grew from a 1.06× advantage at 1 s to a
  2.21× advantage at 10 s once the transient was amortised.

### 3.3 Stack-guard parity

All three kernels run with **stack guard validation always on**:
- TaktOS: per-thread stack-guard region, scheduler-side validation,
  `pStackBottom` at TCB offset 16, `TAKTOS_STACK_GUARD_ALIGN = 256`
  on Cortex-M0+, `__builtin_trap` hook on overflow. Always-on, not
  optional.
- FreeRTOS: `configCHECK_FOR_STACK_OVERFLOW = 2` (canary pattern
  validation at every context switch).
- ThreadX: stack overflow checking enabled (`TX_ENABLE_STACK_CHECKING`),
  combined with the `_txe_*` parameter-validation wrappers
  (object-validity check). Worker stack budget bumped to 768 B per
  thread on this target — documented asymmetry, see §3.4.

This is **deliberate**: each kernel's stack guard cost is a feature of
the kernel, not optional instrumentation. Keeping the runtime safety
checks enabled across all three keeps the comparison like-for-like —
every kernel pays for stack guard work on every switch.

### 3.4 Memory allocation strategy

- **TaktOS:** static allocation only, no heap. All TCBs and stacks
  caller-supplied via KVB's per-test arrays. Inline-in-handle —
  every kernel object is a struct member of `KvbThread` /
  `KvbSemaphore` / etc.
- **FreeRTOS:** `configSUPPORT_STATIC_ALLOCATION=1` plus a 4 KB heap_4
  arena. The heap is required because the V11.3 ARM_CM0 split port
  (port.c + portasm.c) hard-faults on first PendSV with static-only
  allocation on this IOsonata startup environment (root cause not
  identified; reproduced reliably with
  `configSUPPORT_DYNAMIC_ALLOCATION=0`).
- **ThreadX:** static allocation only, no heap. ThreadX hangs control
  blocks (TCB / TX_SEMAPHORE / TX_MUTEX / TX_QUEUE) on kernel-global
  linked lists for the lifetime of each object — addresses must
  remain stable. So the KVB ThreadX port keeps a port-private static
  slot pool (4 thread + 6 sem + 2 mutex + 2 queue on this target)
  in BSS, indexed by handle, matching the pattern used by
  Microsoft's own Thread-Metric ThreadX reference port.
- **ThreadX worker stack:** 768 B per worker thread, vs 256 B on TaktOS
  and FreeRTOS. Documented asymmetry. The `_txe_*` parameter-validation
  wrappers add a real C call frame to every public ThreadX API call
  (~30 B on M0 -Os), and that compounds with the deeper internal
  call chain in `tx_thread_relinquish`. 256 B has no margin for the
  resulting depth. TaktOS inlines its checks; FreeRTOS uses
  `configASSERT` macros at the call site.

These are **methodology asymmetries**, not benchmark tuning: heap_4
adds per-allocation overhead that static-only does not, but allocations
happen during test setup and not inside the measured loops, so
steady-state throughput is not affected. The slot pool indirection in
the ThreadX port adds two memory loads per kernel call, comparable to
the `xQueue*` pointer chasing in FreeRTOS — its effect is reflected in
the throughput numbers and not subtracted out. The comparison fairness
inside the measurement window is preserved across all three kernels.

### 3.5 Mutex ownership rejection mechanisms

`SYNC_MUTEX_OWNERSHIP_001` deliberately drives a non-owner unlock to
verify the kernel rejects it. Each kernel reports the rejection
differently, and the KVB port layer adapts each native behaviour into
a single `KVB_ERR_NOT_OWNER` status code:

- **TaktOS:** native ownership check on the unlock path returns
  `TAKTOS_ERR_INVALID` cleanly. KVB port maps it to `KVB_ERR_NOT_OWNER`.
  No ownership pre-check needed; the kernel itself rejects the call.
  No measurable overhead on the SYNC_MUTEX_FAST_001 fast path.

- **FreeRTOS:** V11.3 `xSemaphoreGive` on a mutex by a non-owner task
  trips `configASSERT` (infinite spin with interrupts disabled). The
  KVB FreeRTOS port pre-checks ownership using
  `xSemaphoreGetMutexHolder` and returns `KVB_ERR_NOT_OWNER` directly
  without ever entering the asserting code path. The pre-check costs
  ~30 cycles per `kvb_mutex_unlock` call and applies unconditionally,
  including on the SYNC_MUTEX_FAST_001 fast path. At 48 MHz this is
  ~625 ns per unlock, roughly 4.9 % of the per-pair time (12.8 µs) on
  FreeRTOS. **The reported 78,097 p/s for FreeRTOS mutex therefore
  understates raw FreeRTOS mutex throughput by roughly 4.9 %.** Removing
  the pre-check would shift the FreeRTOS number to roughly 82,100 p/s —
  TaktOS still leads by 2.14×, vs the reported 2.25×.

- **ThreadX:** `tx_mutex_put` returns `TX_NOT_OWNED` cleanly when called
  by a non-owner. KVB port maps it to `KVB_ERR_NOT_OWNER`. Like TaktOS,
  no ownership pre-check needed; the kernel itself rejects the call.
  This is why the KVB ThreadX port can let SYNC_MUTEX_FAST_001 measure
  the unmodified ThreadX lock/unlock fast path.

So on this benchmark: **TaktOS and ThreadX measure pure native fast-path
costs; FreeRTOS measures fast path plus the +30-cycle ownership pre-check
the port has to add to make the OWNERSHIP test work without halting on
configASSERT.** Documented; folded into the §4.4 analysis.

### 3.6 Configuration files

- `KVB/Targets/STM32F0308/include/FreeRTOSConfig.h`
- `KVB/Targets/STM32F0308/include/kvb_config_stm32f0308_freertos.h`
- `KVB/Targets/STM32F0308/include/kvb_config_stm32f0308.h` (TaktOS)
- `KVB/Targets/STM32F0308/include/kvb_config_stm32f0308_threadx.h`
- `KVB/include/tx_user.h` (ThreadX feature gates  shared across every KVB target)

---

## 4. Detailed results

All values are mean ± stddev across 5 separate runs per kernel
(6 for FreeRTOS, full power-cycle between runs, identical firmware
on each). Where stddev is reported as 0, all runs produced
bit-identical results.

### 4.1 Determinism observation

Before discussing per-test numbers, the headline determinism finding
deserves its own column:

- **TaktOS:** every metric on every test, all 5 runs, **bit-identical**
  (rel.σ = 0). The kernel is fully deterministic on this workload —
  no measurement-to-measurement drift in iteration count, scheduling
  decisions, or sleep duration.
- **FreeRTOS V11.3:** every metric on every test, all 6 runs,
  **bit-identical** (rel.σ = 0). The single-yield first-power-on
  shift observed in earlier captures (run 1: 1,775,292 yields, runs
  2-4: 1,775,293) does not appear in this measurement run. The
  cleanup fix in `KVB/src/tests/sync/` (each test now calls
  `kvb_*_delete` before return) appears to have stabilized the heap_4
  placement timing — every test now leaves the heap in the same
  state, so the first-boot transient that depended on heap-placement
  timing is gone.
- **ThreadX 6.x:** every metric on every test, all 5 runs,
  **bit-identical** (rel.σ = 0). The single-µs first-power-on
  shift in TIME_SLEEP_001 elapsed time observed in earlier captures
  (run 1: 9210 µs, runs 2-5: 9229 µs) similarly does not appear in
  this measurement run.

All three kernels are now fully deterministic on this bare-metal MCU
across all runs. No first-boot transients of any kind. This is the
expected and desired behaviour for an embedded RTOS on a deterministic
part — and it is what makes the cross-kernel comparison reliable from
a single run, let alone five (or six for FreeRTOS).

### 4.2 SCHED_COOP_001 — cooperative yield throughput

Three worker threads at the same priority round-robin via cooperative
yield calls (`kvb_thread_yield`) for 10 s. Reports total yield count.

| Kernel          | Workers | Total yields  | Per-thread (min/max) |
|-----------------|--------:|--------------:|---------------------:|
| TaktOS          |       3 |     2,518,433 | 839,477 / 839,478    |
| FreeRTOS V11.3  |       3 |     1,775,300 | 591,766 / 591,767    |
| ThreadX 6.x     |       3 |     1,196,797 | 398,932 / 398,932    |

**TaktOS leads by 1.42× over FreeRTOS, 2.10× over ThreadX.**
**FreeRTOS leads ThreadX by 1.48×.**

This benchmark stresses the **scheduler hot path**. Each yield does:
context save → run-queue update → SelectNext → context restore. TaktOS's
single-cycle CLZ-based bitmap SelectNext (~6 cycles on M4, software
binary-search fallback ~12 cycles on M0) outperforms FreeRTOS's
linked-list-based ready list walk on M0 where there is no CLZ.

ThreadX's `tx_thread_relinquish` falls behind FreeRTOS on this
benchmark because of two compounding factors specific to the
KVB ThreadX port: (1) the `_txe_*` parameter-validation wrappers add a
real C call frame to every public ThreadX API call (~30 B + ~6 cycles
on M0), and (2) the slot-pool indirection in the KVB ThreadX port
adds two memory loads per `kvb_thread_yield`. The 1.48× FreeRTOS-over-
ThreadX gap on this test is consistent with the documented ~30-cycle
validation overhead per call multiplied by the per-yield call count.
This is a **legitimate kernel/port characteristic**, not an artifact
to subtract — both behaviours ship in real ThreadX deployments.

All three kernels distribute work nearly perfectly across the 3 workers.
TaktOS and FreeRTOS each distribute within 1 yield; ThreadX distributes
exactly equally (398,932 each, max-min = 0).

### 4.3 SYNC_SEM_FAST_001 — uncontended counting semaphore

Single thread loops `kvb_sem_post` then `kvb_sem_wait` for 10 s.
Reports total wait/post pairs and computed throughput.

| Kernel          | Pairs       | Throughput (p/s) | µs/pair |
|-----------------|------------:|-----------------:|--------:|
| TaktOS          |   2,443,520 |          244,349 |   4.09  |
| FreeRTOS V11.3  |   1,126,656 |          112,654 |   8.88  |
| ThreadX 6.x     |   1,963,008 |          196,295 |   5.09  |

**TaktOS leads FreeRTOS by 2.17×, ThreadX by 1.24×.**
**ThreadX leads FreeRTOS by 1.74×.**

This benchmark exercises the **uncontended atomic fast path**. All
three kernels handle the no-waiter case with interrupt-disable
critical sections on M0 (no LDREX/STREX — M0 lacks them).

The TaktOS-vs-FreeRTOS 2.21× gap reflects code path length: TaktOS's
counting semaphore is a single struct field decrement under a brief
critical section; FreeRTOS implements counting semaphore on top of
the queue subsystem with `xQueueSemaphoreTake/xQueueGenericSend`
doing event-list manipulation even when uncontended.

The ThreadX-vs-FreeRTOS 1.76× gap is on the FreeRTOS side of the
ledger — ThreadX's `tx_semaphore_get` / `tx_semaphore_put` are
purpose-built for semaphores and don't go through a general queue
substrate. The TaktOS-vs-ThreadX 1.25× residual gap reflects the
slot-pool indirection in the KVB ThreadX port plus the
parameter-validation overhead — TaktOS has neither.

### 4.4 SYNC_MUTEX_FAST_001 — uncontended mutex lock/unlock

Single thread loops `kvb_mutex_lock` then `kvb_mutex_unlock` for 10 s.

| Kernel          | Pairs       | Throughput (p/s) | µs/pair |
|-----------------|------------:|-----------------:|--------:|
| TaktOS          |   1,760,512 |          176,040 |   5.68  |
| FreeRTOS V11.3  |     781,056 |           78,097 |  12.80  |
| ThreadX 6.x     |     883,712 |           88,352 |  11.32  |

**TaktOS leads FreeRTOS by 2.25×, ThreadX by 1.99×.**
**ThreadX leads FreeRTOS by 1.13×.**

Reasons:
- TaktOS's mutex uses **IPCP (Immediate Priority Ceiling Protocol)**:
  on `Lock`, the holder's effective priority is raised to the lock's
  ceiling; on `Unlock`, the effective priority is restored to the
  next-higher held ceiling (or the base priority if none). This is
  O(1) per acquire and bounds priority inversion to the longest critical
  section at the ceiling. The fast path is a single-word owner-pointer
  CAS plus the priority-bump bookkeeping — about 67 cycles of IPCP work
  per lock/unlock pair on M0, which is the visible cost vs an
  uncontended PI mutex. **TaktOS gives up some throughput for the
  worst-case-bounded behaviour IPCP guarantees.**
- FreeRTOS V11.3 implements mutex on top of the queue subsystem.
  Queue is the underlying mechanism; mutex is a queue with a
  holder pointer + priority inheritance state. Each lock/unlock
  walks the queue ready/blocked lists even when uncontended.
  FreeRTOS path additionally pays the +30-cycle ownership pre-check
  from the KVB port (§3.5), accounting for ~4.9 % of the per-pair
  time on FreeRTOS. Removing the pre-check would tighten the gap
  from 2.25× to ~2.14×.
- ThreadX's `tx_mutex_get` / `tx_mutex_put` are purpose-built for
  mutexes and report `TX_NOT_OWNED` cleanly on non-owner unlock —
  no port-side pre-check needed (§3.5). Their cost reflects the
  `_txe_*` parameter validation, the slot-pool indirection in the
  KVB port, and the priority-inheritance bookkeeping ThreadX does
  unconditionally on every lock/unlock.

| **Methodology note (TaktOS Mutex throughput shift):** Earlier KVB
  results captured before the TaktOS mutex was rewritten from PI to
  IPCP showed TaktOS at 235,951 p/s on this test. The current 176,040
  p/s reflects the IPCP rewrite and the corresponding worst-case-bounded
  guarantee. The change is bounded (~1.4 µs/pair on M0) and reflects
  a deliberate quality-vs-throughput trade. TaktOS still leads on
  every kernel for this test; the gap to FreeRTOS narrowed from 2.97×
  to 2.25×, and the gap to ThreadX narrowed from 2.63× to 1.99×. |

### 4.5 SYNC_MUTEX_OWNERSHIP_001 — non-owner unlock rejection

Owner task locks the mutex, signals it has done so, blocks. Runner
task (non-owner) attempts to unlock. Test passes if the unlock is
rejected with a non-`KVB_OK` status.

| Kernel          | Result | Returned status               |
|-----------------|:------:|:------------------------------|
| TaktOS          | PASS   | `KVB_ERR_NOT_OWNER` (3)       |
| FreeRTOS V11.3 | PASS   | `KVB_ERR_NOT_OWNER` (3)       |
| ThreadX 6.x     | PASS   | `KVB_ERR_NOT_OWNER` (3)       |

**Three-way parity.** All three kernels reject non-owner unlock and
report it through the same `KVB_ERR_NOT_OWNER` status code at the KVB
boundary — though the underlying mechanisms differ:

- **TaktOS:** native ownership check on the unlock path returns
  `TAKTOS_ERR_INVALID`; KVB port maps to `KVB_ERR_NOT_OWNER`.
- **FreeRTOS:** the kernel's native rejection is via `configASSERT`
  (infinite-spin halt). The KVB port's `xSemaphoreGetMutexHolder`
  pre-check (§3.5) intercepts the call and returns the status
  before `configASSERT` fires.
- **ThreadX:** native ownership check returns `TX_NOT_OWNED`; KVB
  port maps to `KVB_ERR_NOT_OWNER`.

The semantic outcome is the same across all three: the mutex stays
locked, the owner remains the only task that can release it, no
silent corruption. This is a real safety property — every kernel
under test enforces ownership at the API boundary, with measurably
identical observable behaviour.

### 4.6 IPC_QUEUE_FAST_001 — same-thread queue send/receive

Single thread loops `kvb_queue_send` then `kvb_queue_receive` for 10 s.

| Kernel          | Pairs    | Throughput (p/s) | µs/pair |
|-----------------|---------:|-----------------:|--------:|
| TaktOS          |  791,296 |           79,104 |  12.64  |
| FreeRTOS V11.3  |  393,216 |           39,316 |  25.43  |
| ThreadX 6.x     |  646,400 |           64,623 |  15.47  |

**TaktOS leads FreeRTOS by 2.01×, ThreadX by 1.22×.**
**ThreadX leads FreeRTOS by 1.64×.**

The queue benchmark moves a `KVB_QUEUE_MESSAGE_SIZE = 16` byte
payload per send/receive. At this payload size the data plane
copy is a meaningful fraction of the work. TaktOS's queue uses
plain `Avail/pWrite/pRead` indices under a single critical section,
while FreeRTOS V11.3 uses its full queue object (event-list pointers,
locking counters, etc.) per operation. ThreadX's `tx_queue_send` /
`tx_queue_receive` are purpose-built for queues but pay the
`_txe_*` validation overhead on every call. The ranking matches the
SYNC_SEM pattern: TaktOS minimal, ThreadX purpose-built but with
validation overhead, FreeRTOS general-purpose queue substrate.

### 4.7 TIME_SLEEP_001 — thread sleep duration accuracy

Caller invokes `kvb_thread_sleep_ticks(10)` (= 10 ms at 1000 Hz tick).
KVB's pass criterion is `elapsed_us >= min_expected_us` where
`min_expected_us = 9000 µs` (90 % of nominal). TaktOS additionally
specifies a stricter contract: at-least-N-ticks, i.e. `elapsed_us >=
nominal_us = 10000 µs`.

| Kernel          | KVB result | Requested | Elapsed (µs) | ≥ 10000 µs? |
|-----------------|:----------:|----------:|-------------:|:-----------:|
| TaktOS          |   PASS     |    10 ms  |        10476 |   ✓ yes     |
| FreeRTOS V11.3  |   PASS     |    10 ms  |        10005 |   ✓ yes (5 µs margin) |
| ThreadX 6.x     |   PASS     |    10 ms  |         9537 |   ✗ no      |

**All three kernels pass KVB's documented threshold. TaktOS exceeds
the stricter at-least-N-ticks contract comfortably; FreeRTOS V11.3
satisfies it by a 5 µs margin. ThreadX undershoots by 463 µs.** See §6.

This is a **kernel-quality finding**, not a benchmark performance
metric, and the picture has changed in this measurement compared to
prior captures of this same suite.

The shift is real and instrumentation-side, not a kernel change. The
KVB test-cleanup fix landed in `KVB/src/tests/sync/kvb_test_sync.c` (each
test now calls `kvb_*_delete` before return) added a small amount of
work between scheduler start and the SLEEP measurement starting tick.
That work shifts the moment when `kvb_thread_sleep_ticks(10)` is
called inside the tick period — it now lands closer to a tick edge on
the FreeRTOS path, which under the V11.3 `vTaskDelay` semantics happens
to land just past a tick boundary, giving a measured 10005 µs. The
algorithm still has the same off-by-one shape; it just gets lucky on
which side of the boundary the call hits.

ThreadX shifted in the same direction (9229 → 9537, +308 µs) but did
not cross the strict bound, because its baseline shortfall was larger.
The 9537 µs corresponds to ~9.5 tick periods elapsed before wake — the
same partial-tick mechanism described in §6, just with different
fixed startup work between the wallclock-read and the sleep entry.

TaktOS's `TaktOSThreadSleepTicks(N)` continues to pass the strict bound
deterministically because the kernel adds `+1u` to `WakeTick` at every
timed-wait site, guaranteeing N full tick periods elapse before wake
regardless of where in the current period the call lands. **TaktOS is
the only kernel under test that meets the strict bound by design**;
FreeRTOS meets it on this run by a 5 µs margin (i.e., depends on
where in the tick the call happens to land), and ThreadX does not
meet it at all on this run.

---

## 5. What the numbers say

In one sentence: **TaktOS leads both FreeRTOS V11.3 and Eclipse
ThreadX 6.x on every throughput benchmark on Cortex-M0, with
1.22×–2.10× over ThreadX and 1.42×–2.25× over FreeRTOS.**

The pattern across the four throughput tests:

| Test                | TaktOS vs FreeRTOS | TaktOS vs ThreadX | ThreadX vs FreeRTOS |
|---------------------|-------------------:|------------------:|--------------------:|
| SCHED_COOP_001      |              1.42× |             2.10× |               0.67× |
| SYNC_SEM_FAST_001   |              2.17× |             1.24× |               1.74× |
| SYNC_MUTEX_FAST_001 |              2.25× |             1.99× |               1.13× |
| IPC_QUEUE_FAST_001  |              2.01× |             1.22× |               1.64× |

ThreadX vs FreeRTOS is mixed — ThreadX is faster on SEM, MUTEX, and
QUEUE because those are purpose-built primitives, but slower on
SCHED_COOP because `tx_thread_relinquish` carries the parameter-
validation overhead that FreeRTOS's `taskYIELD` macro (a direct
PendSV pend) does not. Two real RTOSes with different design
priorities; the data shows where each leads.

This matches the TaktOS design intent: the simpler the operation,
the closer all kernels run to the hardware floor (interrupt-disable
critical section + memory access). As operations get more complex,
TaktOS's purpose-built primitives (single-word mutex, bitmap
scheduler, plain index queue) pull ahead of both FreeRTOS's
queue-as-everything implementation and ThreadX's purpose-built
but validation-heavy implementation.

The TIME_SLEEP_001 result is qualitatively different — it shows a
**behavioural** difference, not a performance one. TaktOS's
specification says "sleep for at least N ticks"; FreeRTOS's and
ThreadX's specifications both say "sleep for between 0 and N tick
periods" — the same partial-tick semantics. KVB tests against a
9000 µs floor that all three meet. On this measurement run TaktOS
satisfies the stricter at-least-N-ticks contract by a comfortable
476 µs margin; FreeRTOS V11.3 satisfies it by only a 5 µs margin
(within tick-edge noise — see §6); ThreadX undershoots by 463 µs.
**Only TaktOS guarantees the strict bound by design**; the
FreeRTOS PASS is a matter of where in the tick the test happens to
land.

The determinism observation in §4.1 is independently meaningful:
all three kernels are bit-perfectly reproducible across power
cycles on this workload. The first-power-on transients observed
in earlier captures (FreeRTOS single-yield shift, ThreadX 19 µs
sleep shift) do not appear in this measurement run — the test
cleanup fix has stabilized the heap-placement and tick-edge alignment
timing. This is the desired behaviour for embedded RTOS on a
deterministic MCU and confirms that single-run measurements on this
platform are representative.

---

## 6. FreeRTOS and ThreadX TIME_SLEEP_001 off-by-one

`vTaskDelay(xTicksToDelay)` in FreeRTOS schedules wakeup at
`xConstTickCount + xTicksToDelay`, where `xConstTickCount` is the
tick count at the time `vTaskDelay` is called. If the SysTick is
already partway through the current tick period when `vTaskDelay`
is called, the next tick fires partway through that period —
fewer than `xTicksToDelay` full tick periods have elapsed by the
time the task wakes.

`tx_thread_sleep(timer_ticks)` in ThreadX has the same shape and the
same problem. Wakeup is scheduled at `current_tick + timer_ticks`,
where `current_tick` is the value when `tx_thread_sleep` is called
— same partial-tick gap on entry, same off-by-one on exit.

Worst case for both: `Sleep(N)` returns after `(N-1)` ticks +
`(time_until_next_tick)`. For `N=10` at 1000 Hz tick:
- FreeRTOS observed 10005 µs → `vTaskDelay(10)` happened to land
  effectively on a tick boundary (call entry within a ~5 µs window
  before tick edge). PASSes the strict 10000 µs bound by 5 µs.
- ThreadX observed 9537 µs → `tx_thread_sleep(10)` invoked ~463 µs
  into a tick period; the next 10 ticks finished 463 µs early.

The bit-perfect reproducibility of these figures across all 5 runs
of each kernel confirms this is deterministic behaviour on this
firmware, not flaky measurement.

The FreeRTOS PASS in this run depends on where in the tick the test
infrastructure happens to place the sleep call. A change to the test
preamble that shifts the call position by ~10 µs would push FreeRTOS
back below 10000 µs. **The strict bound is met by chance, not by
design.** TaktOS meets it by design — the kernel adds `+1u` to
`WakeTick` at every timed-wait site, guaranteeing N full tick periods
elapse before wake regardless of where in the current period the call
lands. The cost is one extra tick of latency in the worst case (best
case unchanged).

This is a real, defensible TaktOS quality advantage: a stricter sleep
specification, deterministic lower bound. KVB's documented threshold
(9000 µs floor for a 10000 µs nominal) is met by all three kernels;
on this measurement run, FreeRTOS V11.3 passes the strict bound by
5 µs (chance) and ThreadX undershoots by 463 µs. Only TaktOS meets
the strict bound across all measurements regardless of preamble
timing.

For applications where FreeRTOS's or ThreadX's looser sleep semantics
are a problem, the standard workaround is to call `vTaskDelay(N+1)` /
`tx_thread_sleep(N+1)` — shifting the responsibility onto the caller.
TaktOS handles this inside the kernel.

---

## 7. Reproducibility

All sources, project files, and configurations needed to reproduce
these numbers are committed under:

- `KVB/Targets/STM32F0308/TaktOS_STM32F0308_KvbSuite/`    (TaktOS variant)
- `KVB/Targets/STM32F0308/FreeRTOS_STM32F0308_KvbSuite/`  (FreeRTOS variant)
- `KVB/Targets/STM32F0308/ThreadX_STM32F0308_KvbSuite/`   (ThreadX variant)
- `KVB/ports/kernels/taktos/`                              (TaktOS kernel port)
- `KVB/ports/kernels/freertos/`                            (FreeRTOS kernel port)
- `KVB/ports/kernels/threadx/`                             (ThreadX kernel port)
- `KVB/Targets/src/`                                       (kernel-agnostic test runner + main)
- `KVB/Targets/STM32F0308/src/`                            (per-MCU platform glue)
- `KVB/Targets/STM32F0308/include/`                        (per-MCU configs, FreeRTOSConfig.h)
- `KVB/include/tx_user.h`                                  (ThreadX feature config, shared across every KVB target)

Multi-run aggregation tool: `KVB/tools/compare_runs.py`. Takes one
or more KVB UART log files per `--label` group, computes per-metric
mean / sample stddev / min / max / N, prints a Markdown comparison
table.

To reproduce:

1. Clone IOsonata; place this `KVB/` tree alongside it in IOsonata's
   parent directory so that `IOCOMPOSER_HOME` resolves to the
   IOsonata parent directory.
2. Open Eclipse CDT, import all three projects from `Eclipse/`
   subdirectories. Build Release for each.
3. Flash the TaktOS variant. Connect to the ST-LINK VCOM at 115200
   8N1. Power-cycle the board, capture UART output until
   `[KVB] END RUN`. Save as `taktos_run1.log`. Press reset, capture
   again, save as `taktos_run2.log`. Repeat for 5 runs.
4. Repeat step 3 with the FreeRTOS variant, saving as
   `freertos_runN.log`.
5. Repeat step 3 with the ThreadX variant, saving as
   `threadx_runN.log`.
6. Aggregate:
   ```
   python3 KVB/tools/compare_runs.py \
       --label TaktOS    taktos_run1.log    taktos_run2.log    \
                         taktos_run3.log    taktos_run4.log    \
                         taktos_run5.log                       \
       --label FreeRTOS  freertos_run1.log  freertos_run2.log  \
                         freertos_run3.log  freertos_run4.log  \
                         freertos_run5.log                     \
       --label ThreadX   threadx_run1.log   threadx_run2.log   \
                         threadx_run3.log   threadx_run4.log   \
                         threadx_run5.log
   ```

---

## 8. Open work

- **Investigate FreeRTOS V11.3 ARM_CM0 static-allocation hardfault.**
  Static-only build hardfaults inside PendSV at first context switch
  on this IOsonata startup environment. Workaround: heap_4 with 4 KB
  heap (current configuration). Root cause: not identified;
  potentially specific to IOsonata's vector table structure
  interacting with the V11.3 split port's stack-guard probe sequence.
  Closing this would unify FreeRTOS allocation strategy with
  nRF52832 (pure static) and remove the only remaining methodology
  asymmetry between targets.

- **Optional: split FreeRTOS port mutex unlock into fast-path and
  ownership-checked variants** so SYNC_MUTEX_FAST_001 measures pure
  unlock cost without the ~30-cycle ownership pre-check overhead.
  Estimated improvement on the FreeRTOS mutex number: ~5 %, no change
  to any other test. The TaktOS lead would tighten from 2.25× to
  approximately 2.14×.

- **TIME_SLEEP_001 FreeRTOS strict-bound margin (5 µs).** The
  FreeRTOS V11.3 PASS at the strict bound depends on where in the
  current tick the test infrastructure happens to land the
  `vTaskDelay(10)` call. A small change to the test preamble or
  surrounding test order would shift this either side of 10000 µs.
  This isn't a defect but it's a fragile result; the fact that
  TaktOS PASSes by 476 µs and FreeRTOS PASSes by 5 µs in the same
  measurement reflects a real difference in design intent. See §6.

- **Closed: kernel object cleanup leaks.** Earlier KVB tests created
  stack-local kernel objects (`KvbSemaphore sem`, `KvbMutex mutex`,
  etc.) without calling `kvb_*_delete` before return. With pure
  static allocation the storage IS the caller-supplied buffer; when
  the function returns, FreeRTOS's internal task lists hold dangling
  pointers into the popped frame. Fixed in `KVB/src/tests/sync/`,
  `sched/`, `ipc/` by adding cleanup at every return path. On
  STM32F0308 where FreeRTOS uses heap_4, this fix doesn't expose
  pre-existing breakage (the kernel storage was in heap, not stack)
  but does shift throughput slightly (~1-3% for sem/mutex/queue) and
  removes the first-power-on transients that were attributed to
  heap_4 placement timing. The fix is required for nRF52832 where
  pure-static FreeRTOS would have hardfaulted.

- **Closed: TaktOS Mutex throughput shift (235,951 → 176,040 p/s).**
  TaktOS Mutex implementation changed from PI to IPCP. On every
  `Lock`, IPCP raises the holder's effective priority to the
  lock's ceiling and tracks the held-ceiling stack; on every
  `Unlock` the effective priority is restored. About 1.4 µs of
  bookkeeping per pair on M0 — visible in the throughput number,
  not visible in the worst-case bound. This is a deliberate
  quality-vs-throughput trade for safety/cert customers needing
  bounded priority inversion analysis. TaktOS's lead is now 2.25×
  vs FreeRTOS (was 2.97×) and 1.99× vs ThreadX (was 2.63×). Still
  the leader on every test in the suite.

- **Closed: first-power-on transients.** Earlier captures showed a
  FreeRTOS single-yield first-boot shift (heap_4 placement) and a
  ThreadX 19 µs first-boot sleep shift (SysTick edge alignment).
  Neither appears in the new measurement run; the cleanup fix
  appears to have stabilized the heap-placement and tick-edge
  alignment timing. All three kernels are now bit-identical across
  all runs.

- **Closed: ThreadX KVB sem pool sizing.** Earlier iteration of the
  KVB ThreadX port sized `KVB_THREADX_SEM_POOL_SIZE = 4` against
  per-test peak; SYNC_MUTEX_OWNERSHIP_001 then failed with
  `release semaphore create failed` because tests don't call
  `kvb_sem_delete`, so slots accumulate. Bumped to 6 (cumulative
  peak across the run). +160 B BSS, fits within the documented 8 KB
  budget. With the test cleanup fix landed (objects now properly
  deleted), the pool size could be revisited — per-test peak ≤ 4
  may now be sufficient. Not changed in this revision; the 6 is
  harmless and the runs already ship with that value.

- **Port KVB to nRF52832 (Cortex-M4F) — DONE.** See companion document
  `nRF52832_KVB_Comparison.md`. Three-kernel parity demonstrated;
  TaktOS lead widens vs FreeRTOS (3.17× SEM on M4F vs 2.17× on M0)
  because TaktOS's atomic split-counter primitive picks up more
  speedup from the M4 ALU than FreeRTOS's queue-substrate semaphore
  does. Cross-target view in §7 of that document.

- **Port KVB to nRF54L15 (Cortex-M33 @ 128 MHz).** Working FreeRTOS
  and ThreadX Thread-Metric reference projects in the IOsonata tree
  at this target, so three-kernel parity is already demonstrated at
  the build level. Pending.

---

*Author: Nguyen Hoan Hoang, I-SYST inc., Brossard, Canada*
*KVB framework version: 0.1.0-private*
*Document version: 4.0 — 2026-04-29 — supersedes v3.0; reflects TaktOS Mutex IPCP rewrite, KVB test cleanup leak fix, and re-validation against recent KVB common-code bug fixes*

---

### Revision history

- **v4.0 (2026-04-29).** Re-measured all three kernels after (a) TaktOS
  Mutex changed from PI to IPCP, (b) `KVB/src/tests/sync/`,
  `sched/`, `ipc/` cleanup leak fix landed, and (c) recent KVB
  common-code bug fixes. The cleanup fix made the FreeRTOS V11.3
  TIME_SLEEP_001 result cross the strict 10000 µs at-least-N-ticks
  bound (from 9604 → 10005 µs); the IPCP rewrite shifted TaktOS Mutex
  throughput from 235,951 → 176,040 p/s as a deliberate
  quality-vs-throughput trade. All three kernels now bit-identical
  across all runs, no first-power-on transients. The KVB
  common-code bug fixes did not change any measured value (the
  re-run produced byte-identical capture vs the prior post-cleanup
  run), confirming the fixes were in code paths not exercised by
  these tests.

- **v3.0 (2026-04-29).** Added Eclipse ThreadX 6.x as a third kernel.
  Bumped run aggregate from 4 to 5 runs per kernel. Documented
  ThreadX KVB sem pool sizing (4 → 6).

- **v2.0 (earlier).** TaktOS-vs-FreeRTOS only.
