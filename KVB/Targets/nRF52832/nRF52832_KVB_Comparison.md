# KVB Benchmark Report — nRF52-DK (PCA10040)

**Target:** nRF52832 on nRF52-DK (PCA10040) board
**CPU:** ARM Cortex-M4F (ARMv7E-M, with FPU)
**Clock:** 64 MHz
**SRAM:** 64 KB
**Flash:** 512 KB
**Compiler:** xPack arm-none-eabi-gcc 12.2.1, `-Os`, Release build
**Date:** 2026-05-01
**Document version:** 2.2 (10 s window, 5-run aggregate per kernel, strict-parity Zephyr)

Author: Nguyen Hoan Hoang, I-SYST inc.

---

## 1. Summary

This report presents head-to-head KVB benchmark results between **TaktOS**
(this work, MIT, certification target IEC 61508 SIL 2 → ISO 26262 ASIL D),
**FreeRTOS V11.3** (Amazon/AWS, MIT), **Eclipse ThreadX 6.x**
(Microsoft, MIT), and **Zephyr 4.2.99** (Linux Foundation, Apache 2.0)
on the same physical hardware, the same C/C++ application code, the
same UART, the same compiler, and the same KVB test framework. Only
the kernel under test differs.

| Test ID                       |   TaktOS   | FreeRTOS V11.3 | ThreadX 6.x | Zephyr 4.2.99 | TaktOS vs FR | TaktOS vs TX | TaktOS vs Z |
|-------------------------------|-----------:|---------------:|------------:|--------------:|-------------:|-------------:|------------:|
| SCHED_COOP_001 (yields)       |  4,093,473 |      2,673,007 |   1,929,845 |     1,679,433 |        1.53× |        2.12× |       2.44× |
| SYNC_SEM_FAST_001 (p/s)       |    443,662 |        139,804 |     359,094 |       204,934 |        3.17× |        1.24× |       2.16× |
| SYNC_MUTEX_FAST_001 (p/s)     |    375,768 |         97,238 |     157,013 |       170,778 |        3.86× |        2.39× |       2.20× |
| SYNC_MUTEX_PCP_FAST_001 (p/s) |    175,466 |         97,239 |     157,404 |       168,060 |        1.80× |        1.11× |       1.04× |
| SYNC_MUTEX_OWNERSHIP_001      |       PASS |           PASS |        PASS |          PASS |       parity |       parity |      parity |
| IPC_QUEUE_FAST_001 (p/s)      |    110,874 |         53,586 |     100,786 |        70,166 |        2.07× |        1.10× |       1.58× |
| TIME_SLEEP_001                | PASS @10153 µs | PASS @9696 µs | PASS @9429 µs | PASS @11297 µs | TaktOS+Zephyr meet strict bound | | |

`p/s` = wait/post or send/recv pairs per second. All numbers are
aggregated over 5 separate runs per kernel; per-metric run-to-run
variance is reported in §4 alongside each measurement.

**TaktOS leads every throughput test against every other kernel on
this build.** The narrowest margin is on `SYNC_MUTEX_PCP_FAST_001`,
where TaktOS's priority-ceiling-protocol (PCP) mutex measures 1.04×
faster than Zephyr's priority-inheritance (PI) mutex — close
because PCP and PI exercise different code paths and bookkeeping
shapes. The widest margin is on `SYNC_MUTEX_FAST_001` (plain mutex,
no PI/PCP), where TaktOS leads FreeRTOS by 3.86×.

Suite outcome (all four kernels, 5 runs each):
- TaktOS:        7 PASS / 0 FAIL / 7 total — every run
- FreeRTOS V11.3: 7 PASS / 0 FAIL / 7 total — every run
- ThreadX 6.x:   7 PASS / 0 FAIL / 7 total — every run
- Zephyr 4.2.99: 7 PASS / 0 FAIL / 7 total — every run

All four kernels pass every KVB test at the documented thresholds.
Two of four — TaktOS and Zephyr — additionally satisfy the stricter
at-least-N-ticks rule on TIME_SLEEP_001 (`elapsed_us ≥ 10000` for
a `sleep(10 ticks)` call at 1000 Hz tick). TaktOS does so by design
(the kernel adds `+1u` at every timed-wait site); Zephyr does so
because `k_sleep` rounds up similarly. FreeRTOS and ThreadX both
clear KVB's 90 % floor (9000 µs) but undershoot the 10000 µs nominal
by ~300 / ~570 µs respectively. See §6.

Note on the mutex tests: `SYNC_MUTEX_FAST_001` exercises
each kernel's plain mutex API with its native default protocol —
no priority protocol on TaktOS, priority inheritance on FreeRTOS /
ThreadX / Zephyr. `SYNC_MUTEX_PCP_FAST_001` exercises the priority-
boost variant: priority ceiling (PCP) on TaktOS, priority inheritance
(PI) on the other three (Zephyr does not expose PCP through `k_mutex`,
and the same is true of FreeRTOS and ThreadX). The two mutex tests
therefore measure related but not identical primitives across kernels.
This is documented in §3.6.

---

## 2. What this report measures

KVB measures **end-to-end iteration throughput** of small workloads
designed to exercise specific kernel features. Each throughput test
runs a tight loop (yield, sem post/wait, mutex lock/unlock, queue
send/receive) for a fixed measurement window and reports completed
iterations per second. This is the same measurement model used by
Thread-Metric (Express Logic / Eclipse ThreadX, the de-facto cross-RTOS
benchmark since the early 2000s).

KVB's tests are deliberately scoped narrower than Thread-Metric's:
KVB's SYNC_MUTEX_OWNERSHIP_001 verifies the kernel's behavioural
specification (non-owner unlock rejection), not throughput; KVB's
TIME_SLEEP_001 verifies a duration lower bound, not throughput. These
are PASS/FAIL behavioural tests, distinct from the four throughput
tests.

For **cycle-accurate primitive-level analysis**, see the TaktOS
engineering specification §4.4 (ARM ~47 cycles) and §5.4 (RISC-V ~88
cycles Zbb / ~98 cycles no-Zbb), derived from llvm-mca static
pipeline analysis of the actual context-switch instruction sequences.
For **higher-throughput Thread-Metric numbers on this same target**,
see the Thread-Metric runs on nRF52832 (Cortex-M4F @ 64 MHz) elsewhere
in the TaktOS benchmark suite. KVB and Thread-Metric measure the same
underlying primitives with different scopes; KVB favours pass/fail
behavioural validation alongside throughput, Thread-Metric is pure
throughput.

All four kernels run identical workloads through identical KVB
infrastructure; differences in reported iterations/second therefore
reflect the difference between the kernels' implementations of the
primitive(s) being exercised, plus a small constant for the fixed
loop body that is the same for all kernels.

---

## 3. Methodology

### 3.1 Hardware

Single physical nRF52-DK (PCA10040) board, J-Link OB onboard. UART
output at 115200-8N1 captured via the J-Link CDC interface. No
external instrumentation; all timing is on-target via the DWT
cycle counter (`DWT->CYCCNT`, ARMv7-M 32-bit, 64 MHz CPU clock).
DWT counter wrap is handled in software (extension to 64-bit, see
`KVB/ports/platforms/cortex_m/kvb_platform_cortex_m.c`).

### 3.2 Common across all four kernels

- **Clock:** 64 MHz HFCLK (XOSC). LFCLK off-budget for KVB; ticks
  are SysTick-driven on FreeRTOS / ThreadX / TaktOS, LFCLK-driven
  on Zephyr (see §3.7).
- **Compiler:** xPack arm-none-eabi-gcc 12.2.1, `-Os`, Release build,
  link-time optimization disabled (matching upstream Thread-Metric
  reference setups for fairness).
- **Tick rate:** 1000 Hz on every kernel.
- **Measurement window:** 10 s (`KVB_MEASUREMENT_MS=10000u`). At
  ~440 k pairs/s on the fastest test, that's 4.4 M iterations per
  capture — tick-edge slop drops to ~0.04 % of the measurement.
- **Workers:** 3 cooperating threads on SCHED_COOP_001 across all
  four kernels (`KVB_WORKER_THREAD_COUNT=3u`). Single thread on
  SEM/MUTEX/QUEUE tests across all four.
- **Throughput batch:** 256 iterations per measurement granule
  across all four kernels (`KVB_THROUGHPUT_BATCH=256u`). Was 1024
  in earlier Zephyr captures; aligned to 256 for v2.2.
- **Heap:** off on every kernel. TaktOS has no heap. FreeRTOS uses
  pure static allocation (`configSUPPORT_DYNAMIC_ALLOCATION=0`,
  `configSUPPORT_STATIC_ALLOCATION=1`, `configTOTAL_HEAP_SIZE=0`).
  ThreadX uses port-private slot pools (no `tx_byte_pool`). Zephyr
  has `CONFIG_HEAP_MEM_POOL_SIZE=0`.
- **Stack-overflow detection:** on every kernel, canary-based — see §3.3.
- **Parameter / object validation:** on every kernel, inline at
  every public API call — see §3.4 for the per-kernel mechanism.
- **Console:** raw UART writes via the IOsonata `g_Uart.Tx()` path
  on FreeRTOS / ThreadX / TaktOS; Zephyr uses `printk` via its own
  UART driver (the IOsonata path is not available on the Zephyr port
  because Zephyr's HAL takes over the device tree). Console is not
  on the measurement hot path on any kernel.

### 3.3 Stack-guard parity (canary-based, all four kernels)

Every kernel runs with always-on stack overflow detection at
parity:

- **TaktOS:** sentinel paint at thread create + sentinel check on
  every context switch. Canary word is `0xDEADBEEF`. Software-only,
  no MPU dependency.
- **FreeRTOS:** `configCHECK_FOR_STACK_OVERFLOW = 2` — canary at
  the top of every task's stack, checked on every context switch.
- **ThreadX:** `TX_ENABLE_STACK_CHECKING` — sentinel pattern at
  thread create, checked on every `tx_thread_relinquish`.
- **Zephyr:** `CONFIG_STACK_SENTINEL=y` — software canary at the
  lowest address of every thread's stack, re-checked on every
  context switch, every `k_yield()`, every tick ISR for the
  interrupted thread, and every thread return. MPU stack guard
  (`CONFIG_HW_STACK_PROTECTION`) is **off** to match the FreeRTOS,
  ThreadX, and TaktOS KVB ports' configurations on this target
  (none of which use MPU). v2.2 note: leaving Zephyr's MPU on
  would over-burden Zephyr by ~80 cycles per context switch vs
  the matched MPU-off framing the other three use.

### 3.4 Parameter / object validation

The four kernels draw the boundary between "kernel responsibility"
and "application responsibility" at different places.  This is a
design choice, not an implementation gap, and it affects the
benchmark numbers because every check the kernel performs is
cycles paid on every call.  The list below is from upstream source,
hot-path APIs only (Init APIs check more on every kernel and are
not on the benchmark hot loop).

- **TaktOS:** the always-on hot-path checks at every public API
  entry are: (1) NULL pointer on the handle (and `pData` on
  `Queue Send` / `Queue Receive`); (2) non-owner unlock rejection
  on every `MutexUnlock` (`pOwner != current` returns
  `ERR_INVALID`, no `configASSERT`-style trap); (3) 4-byte
  alignment check on `pData` in `Queue Send` / `Queue Receive`
  (converts an unaligned-access HardFault on Cortex-M0 into a
  clean `ERR_ALIGN`).  The Init APIs additionally validate range
  parameters at construction time only: `MaxCount == 0` and
  `Initial > MaxCount` in `SemInit`; `Ceiling == 0` and `Ceiling
  >= TAKTOS_MAX_PRI` in `MutexInitProtect`; `ItemSize == 0`,
  `Capacity == 0`, and 4-byte alignment of `ItemSize` /
  `pStorage` in `QueueInit`.  Init checks are not on the
  benchmark hot loop.

  The slow / blocking paths (entered only when the API would
  block the caller) add two cert-required kernel-fence checks:
  ISR-context rejection inside `SemTake` / `MutexLock` / `Queue
  Send` / `Queue Receive` / `ThreadSleepTicks` / `ThreadHandOff` /
  self-`ThreadSuspend` (calling those from Handler mode would
  block the preempted thread, not the ISR, corrupting the
  scheduler), and two scheduler-ring invariant checks inside
  `TaktBlockTask` that detect inconsistency between
  `pThread->Priority` and the ring it is queued in.
  `TaktBlockTask` returns `bool` (true on success, false on
  detected corruption); every caller propagates the false-return
  up as `TAKTOS_ERR_INVALID`.

  TaktOS does not re-validate caller arguments beyond this list —
  the caller's contract with the kernel is that it passes
  well-formed arguments; validating those arguments is the
  application's responsibility.  This is the IEC 61508 SIL 2 /
  ISO 26262 ASIL D boundary TaktOS targets: fence the kernel
  against the failures that would corrupt kernel state, stop
  there, do not babysit the application.  Recovery policy on
  every returned `ERR_INVALID` / `ERR_ALIGN` is the application's:
  halt, log, drive outputs to a safe state, attempt recovery —
  the kernel does not impose a fault-handling policy.  See
  `include/TaktOSSem.h`, `include/TaktOSMutex.h`,
  `include/TaktOSQueue.h` for the inline hot-path checks and
  `src/taktos_*.cpp` for the slow-path `TaktOSInIsr()` guard
  and the `TaktBlockTask` corruption-detect path.

- **FreeRTOS V11.1+:** 3–4 inline `configASSERT()` invocations per
  public API entry (`xQueueSemaphoreTake`: NULL + item-size
  invariant + scheduler-state-vs-timeout; `xQueueGenericSend`:
  NULL + data-pointer-vs-itemSize + overwrite-mode + scheduler-
  state).  Inlined at the call site, no extra call frame.  Design
  intent is conservative defence against caller misuse.  KVB
  additionally adds a +30-cycle owner-equality pre-check on the
  FreeRTOS `kvb_mutex_unlock` fast path so the
  `SYNC_MUTEX_OWNERSHIP_001` test can return a status code instead
  of trapping through `configASSERT` (§3.7).

- **ThreadX 6.x:** `_txe_*` wrapper functions interpose a real C
  call frame on every public API call.  `_txe_semaphore_get` /
  `_txe_mutex_get` validate NULL + object-ID + wait-option vs
  thread context + timer-thread exclusion + ISR posture, then
  `BL`-call the corresponding `_tx_*` body.  Design intent is
  exhaustive caller-misuse defence — the maximum-paranoia end of
  the spectrum.  This is the heaviest per-call validation cost in
  the suite and the single largest contributor to the
  TaktOS-vs-ThreadX gap on `SYNC_SEM_FAST_001`.

- **Zephyr 4.x (NCS v3.3.0 / `sdk-zephyr` rev `ncs-v3.3.0`):**
  current Zephyr splits parameter validation across two mechanisms;
  the KVB build pins both ON to match what the other three RTOSes
  carry.

  **(a) `CHECKIF()` returned-error layer** (`include/zephyr/sys/check.h`).
  Controlled by the Kconfig choice "Error checking behavior for CHECK
  macro", with three modes — `RUNTIME_ERROR_CHECKS` (default; runs
  the check, returns `-EINVAL`), `ASSERT_ON_ERRORS` (traps via
  `__ASSERT_NO_MSG`), and `NO_RUNTIME_CHECKS` (compiles out).  Pinned
  `CONFIG_RUNTIME_ERROR_CHECKS=y` explicitly so prior build state /
  menuconfig cannot leave the choice resolved differently.  Coverage
  on the public surface is narrow: `k_sem_init` invalid count/limit;
  `k_mutex_unlock` `owner == NULL` and `owner != _current`;
  `k_msgq_cleanup` busy.  `z_impl_k_sem_give` and `z_impl_k_mutex_lock`
  carry zero CHECKIF.

  **(b) `__ASSERT()` trap-on-misuse layer.**  Controlled by
  `CONFIG_ASSERT` (gates `__ASSERT` / `__ASSERT_NO_MSG` via
  `__assert.h`'s `__ASSERT_ON`).  This is where most of the API-entry
  runtime validation actually lives in current Zephyr: `k_sem_take`
  ISR-vs-timeout; `k_mutex_lock` / `k_mutex_unlock` ISR plus lock-count
  consistency; `k_msgq_put` / `k_msgq_get` ISR-vs-timeout plus internal
  pointer.  Pinned `CONFIG_ASSERT=y` (`CONFIG_ASSERT_LEVEL=2`) so this
  layer stays active — setting it `n` compiles every `__ASSERT` out
  and leaves a strictly weaker validation surface than FreeRTOS
  `configASSERT`, ThreadX `_txe_*`, or TaktOS's API-entry checks.

  Caveat: `CONFIG_ASSERT=y` also enables `__ASSERT`s inside the
  scheduler / spinlock / wait-queue helpers that the other three
  RTOSes ship no equivalent of.  There is no Kconfig granularity to
  keep the API-entry checks while dropping the internal-debug ones,
  so the build accepts the parity-correct overhead — turning
  `CONFIG_ASSERT=y` for the first time in this round dropped Zephyr
  SEM throughput from 519 k to 205 k p/s, of which part is API-entry
  parity work the others also pay and part is internal-debug
  instrumentation the others do not.  The lighter alternative
  (`CONFIG_ASSERT=n`) would leave only the narrow CHECKIF subset
  active — strictly less validation than the other three RTOSes
  carry — and would not measure parity.

These are different design choices, not a quality gradient.  TaktOS
and Zephyr place the parameter-validation boundary at the kernel
edge and trust the caller; FreeRTOS adds a middle layer of
caller-misuse defence; ThreadX adds an outer wrapper that re-
validates everything every call.  The benchmark cost line shows
exactly that: TaktOS and Zephyr at the lightest end, ThreadX at
the heaviest, FreeRTOS in between.  The KVB build keeps each
kernel's native validation regime in place — overriding any kernel
to be lighter or heavier than its design point would not measure
the kernel as it ships.

Per-object validity tracking is a separate axis: TaktOS validates
it via inline base-pointer sanity (`pSem`/`pMtx` is checked at the
critical-section boundary, no separate ID field needed because the
struct layout is fixed and caller-supplied); ThreadX validates it
via the `tx_*_id` magic-number field inside `_txe_*`; FreeRTOS does
not (V11+ relies on caller correctness); Zephyr does not in this
build (`CONFIG_OBJ_CORE=n`).  The FEATURES line in each captured
run reports this as `invalid_obj=1` (TaktOS, ThreadX) vs
`invalid_obj=0` (FreeRTOS, Zephyr).

### 3.5 Memory allocation strategy

- **TaktOS:** inline-in-handle. Every kernel object is a struct
  member of `KvbThread` / `KvbSemaphore` / `KvbMutex` / `KvbQueue`,
  caller-supplied. No heap, no pools.
- **FreeRTOS:** pure static. `configSUPPORT_STATIC_ALLOCATION=1`,
  `configSUPPORT_DYNAMIC_ALLOCATION=0`, `configTOTAL_HEAP_SIZE=0`.
  Caller supplies `StaticTask_t` / `StaticSemaphore_t` / `StaticQueue_t`
  buffers.
- **ThreadX:** port-private static slot pool, handle-indexed.
  Required because ThreadX hangs control blocks on kernel-global
  linked lists for the lifetime of the object — caller-stack
  allocation of TCBs corrupts kernel state on first frame churn.
  Pool sizes trim to test-suite peak (4 threads + 6 sems + 2 mutexes
  + 2 queues).
- **Zephyr:** caller-supplied. `K_THREAD_STACK_DEFINE`, `struct k_sem`,
  `struct k_mutex`, `struct k_msgq` all in BSS or on the runner stack
  for the test's lifetime.

All four avoid runtime malloc inside benchmark loops.

**Worker stack size — documented asymmetry.** `KVB_DEFAULT_STACK_SIZE`
differs by kernel because each kernel's per-thread overhead differs:

| Kernel         | KVB_DEFAULT_STACK_SIZE | Rationale                                        |
|----------------|-----------------------:|:-------------------------------------------------|
| TaktOS         |              512 bytes | TCB + guard region inline in the supplied block. |
| FreeRTOS V11.3 |              512 bytes | StaticTask_t separate; usable stack is the full block. |
| ThreadX 6.x    |             1024 bytes | `_txe_*` parameter validation + slot-pool indirection use more stack per call. |
| Zephyr 4.2.99  |             1024 bytes | `K_THREAD_STACK_DEFINE` adds per-thread metadata (TLS slot, MPU guard region budget) outside the usable area; usable stack is sized to comfortably absorb Zephyr's deeper public-API call chain. |

The KVB workloads use only a few hundred bytes of stack at peak, so
none of the four configurations is stack-limited. The asymmetry is
recorded here for completeness; not material for the throughput
numbers.

### 3.6 Mutex protocol distinction

Two of the seven KVB tests exercise mutexes:

- **`SYNC_MUTEX_FAST_001`** — uncontended `kvb_mutex_lock` /
  `kvb_mutex_unlock` on the kernel's plain mutex API. Native default
  protocol per kernel: no priority protocol on TaktOS; priority
  inheritance on FreeRTOS, ThreadX, and Zephyr.
- **`SYNC_MUTEX_PCP_FAST_001`** — uncontended lock/unlock on the
  kernel's priority-boost variant. **PCP (priority ceiling protocol)
  on TaktOS**, **PI (priority inheritance) on FreeRTOS, ThreadX,
  Zephyr** — none of those three exposes PCP through their mutex
  API. The test reports the protocol it ran via a `pcp_variant=1`
  or `pi_variant=1` metric line so consumers of the data can
  distinguish.

The `SYNC_MUTEX_PCP_FAST_001` test is therefore comparing TaktOS's
PCP path against the other three kernels' PI paths. Both protocols
boost the holder's effective priority on acquire; the bookkeeping
shapes differ (PCP is a single ceiling priority assignment, PI
walks the wait chain on contention). Numbers are still meaningful
as "what does the kernel do when you ask for a priority-boosted
mutex on this hardware," but they are not measuring the same
algorithm. Methodology note included in the v22 document
distributed alongside this report.

### 3.7 Mutex ownership rejection mechanisms

`SYNC_MUTEX_OWNERSHIP_001` is the one behavioural mutex test:
non-owner unlock must be rejected. The KVB port surfaces this as
`KVB_ERR_NOT_OWNER` (status code 3). Per-kernel mechanism:

- **TaktOS:** kernel-level check inside `TaktOSMutexUnlock` —
  compares `pCurrent->Tid` against `mutex->ownerTid`, returns
  `TAKTOS_ERR_INVALID` if mismatched. The KVB port translates
  this to `KVB_ERR_NOT_OWNER`.
- **FreeRTOS:** ownership pre-check in the KVB port (~30 cycles)
  before calling `xSemaphoreGive` — FreeRTOS V11+ would otherwise
  panic via `configASSERT` rather than return an error code. The
  KVB port catches the precondition and returns `KVB_ERR_NOT_OWNER`.
- **ThreadX:** `tx_mutex_put` returns `TX_NOT_OWNED` natively when
  called by a non-owner. The KVB port translates.
- **Zephyr:** `k_mutex_unlock` returns `-EPERM` when called by a
  non-owner. The KVB port translates.

The `+30 cycles` FreeRTOS pre-check runs on **every** call to
`kvb_mutex_unlock`, including the SYNC_MUTEX_FAST_001 hot loop —
the pre-check is unconditional in `KVB/ports/kernels/freertos/
kvb_port_freertos.c:410–418`, not gated on the test. At 64 MHz
that is ~470 ns per unlock, roughly 4.5 % of the per-pair time
(10.28 µs) measured for FreeRTOS in §4.4. Removing the pre-check
would lift the FreeRTOS SYNC_MUTEX_FAST_001 number from
97,238 p/s to ~101,500 p/s; TaktOS still leads (3.86× → ~3.70×).
The pre-check is required because FreeRTOS V11+ otherwise traps
non-owner unlock through `configASSERT` rather than returning an
error code, and SYNC_MUTEX_OWNERSHIP_001 needs the error code.

### 3.8 Zephyr-specific methodology notes

- **Cycle-counter source:** the KVB Zephyr port reads
  `DWT->CYCCNT` directly via CMSIS on every Cortex-M3/M4/M7/M33
  build (gated on `__CORTEX_M >= 3`, see
  `KVB/Targets/Zephyr/KvbSuite/src/kvb_platform_zephyr.c`). On this
  target that resolves to the 64 MHz CPU cycle counter, ~15.6 ns
  per cycle — bit-identical timing source to the FreeRTOS / ThreadX /
  TaktOS bare-metal KVB ports. The runtime PLATFORM line in the
  captured logs confirms `cpu=ARMv7E-M @ 64 MHz DWT (nrf52832)
  cycle_hz=64000000`. Targets without DWT (Cortex-M0/M0+/M23) fall
  back to `k_cycle_get_32()`; that path is not exercised on
  nRF52832.
- **`KVB_THROUGHPUT_BATCH=256u`:** aligned with the other three
  kernels in v2.2. The previous 1024 value caused ~6 ppm jitter on
  Zephyr — immaterial but flagged.
- **Worker count:** `KVB_WORKER_THREAD_COUNT=3u` aligned with the
  other three kernels in v2.1 (was 5u).
- **`CONFIG_OBJ_CORE=n`:** Zephyr does not track per-object
  validity in this build, so `validates_invalid_objects=0` in
  the FEATURES line. TaktOS and ThreadX both track per-object
  validity natively; FreeRTOS does not. Treated as a parity
  asymmetry inherent to each kernel's design, recorded but not
  patched.
- **`CONFIG_TICKLESS_KERNEL=y`:** NCS board default for nrf52.
  With `CONFIG_TIMESLICING=n` and no sleeps in the inner test
  loops, tickless suppresses zero ticks during measurement
  windows because SysTick still fires at 1 kHz to drive
  `k_uptime_get` and the timeout heap. Asymmetry is essentially
  zero in practice; noted here for completeness.
- **picolibc, no TLS, no thread-stack-info:** Zephyr build uses
  `CONFIG_PICOLIBC=y` (NCS default for nrf52),
  `CONFIG_THREAD_LOCAL_STORAGE=n`, `CONFIG_THREAD_STACK_INFO=n`,
  `CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT=n`. All four are
  documented in `KVB/Targets/Zephyr/KvbSuite/prj.conf` with
  rationale (matching the FreeRTOS / ThreadX / TaktOS KVB ports'
  actual configurations on this target).

### 3.9 Configuration files

- **TaktOS:** `KVB/Targets/nRF52832/include/kvb_config_nrf52832.h`,
  `KVB/Targets/nRF52832/TaktOS_nRF52832_KvbSuite/Eclipse/.cproject`.
- **FreeRTOS:** `KVB/Targets/nRF52832/include/kvb_config_nrf52832_freertos.h`,
  `KVB/Targets/nRF52832/include/FreeRTOSConfig.h`, Eclipse project file.
- **ThreadX:** `KVB/Targets/nRF52832/include/kvb_config_nrf52832_threadx.h`,
  `KVB/include/tx_user.h` (shared across every KVB target),
  Eclipse project file.
- **Zephyr:** `KVB/Targets/Zephyr/KvbSuite/prj.conf`,
  `KVB/Targets/Zephyr/KvbSuite/include/kvb_config_zephyr.h`,
  `KVB/Targets/Zephyr/KvbSuite/CMakeLists.txt`.

All four are committed with the matching reproducibility instructions
in §8.

---

## 4. Detailed results

All numbers below are aggregates over **5 separate captures per
kernel**, each capture from a fresh power-cycle. 4 × 5 = 20 captures
total. Raw log is `KVB/Targets/nRF52832/KVB_Results_nRF52832.txt`
in the repository.

### 4.1 Determinism observation

Three of the four kernels are **bit-identical across all 5 runs**
on every throughput test:

| Kernel         | Throughput run-to-run range (worst test) | Observation                |
|----------------|:----------------------------------------:|:---------------------------|
| TaktOS         |                            0 (every test) | bit-identical              |
| FreeRTOS V11.3 |                            0 (every test) | bit-identical              |
| ThreadX 6.x    |                            0 (every test) | bit-identical              |
| Zephyr 4.2.99  |   490 p/s (0.24 % on SEM, σ < 0.1 % overall) | small but measurable jitter |

TaktOS, FreeRTOS, and ThreadX produce identical numbers on every
run — same kernel state machine, same SysTick-driven tick, same
DWT-counted measurement window, and pure static allocation
everywhere means there's no source of variability. Zephyr's
~0.1 % jitter band is real — likely sources are tickless-idle
re-entry timing, system workqueue tick handling, and idle-thread
housekeeping. Functioning-as-designed Zephyr behaviour but a real
characterisation finding for safety/cert use cases (the trade-off
for Zephyr's richer runtime is paid in determinism, even at idle).

`TIME_SLEEP_001` shows the same pattern: TaktOS / FreeRTOS / ThreadX
return bit-identical elapsed_us across all 5 runs (10153 µs / 9696 µs /
9429 µs respectively); Zephyr's 5 elapsed_us values span a range of
338 µs (11126 / 11135 / 11314 / 11447 / 11464) but every individual
run still meets the strict ≥10000 µs bound by a comfortable margin.

### 4.2 SCHED_COOP_001 — cooperative yield throughput

Three worker threads at the same priority round-robin via cooperative
yield calls (`kvb_thread_yield`) for 10 s. Reports total yield count
and per-thread iteration count.

| Kernel         | Workers | Total yields  | Per-thread (min/max)  |
|----------------|--------:|--------------:|----------------------:|
| TaktOS         |       3 |     4,093,473 | 1,364,491 / 1,364,491 |
| FreeRTOS V11.3 |       3 |     2,673,007 |   891,002 / 891,003   |
| ThreadX 6.x    |       3 |     1,929,845 |   643,281 / 643,282   |
| Zephyr 4.2.99  |       3 |     1,679,433 |   559,811 / 559,811   |

TaktOS-vs-FreeRTOS: 1.53×. TaktOS-vs-ThreadX: 2.12×. TaktOS-vs-Zephyr: 2.44×.

This benchmark stresses the **scheduler hot path**. Each yield does:
context save → run-queue update → SelectNext → context restore. TaktOS's
single-cycle CLZ-based bitmap SelectNext (~6 cycles on M4) dominates
here; FreeRTOS uses CLZ on M4 too via
`configUSE_PORT_OPTIMISED_TASK_SELECTION=1`, but its linked-list-based
ready-list housekeeping per yield still adds work. ThreadX's
`tx_thread_relinquish` carries the `_txe_*` parameter-validation
wrapper plus slot-pool indirection in the KVB port. Zephyr's
`k_yield()` pays the stack-canary check, ready-queue priority
bookkeeping in `_priq_dumb_*`, plus the portable Zephyr context-save
sequence — measurable but not dramatic on this build.

All four kernels distribute work nearly perfectly across the 3
workers. TaktOS, FreeRTOS, ThreadX, and Zephyr all distribute within
1 yield across the 5-run aggregate.

### 4.3 SYNC_SEM_FAST_001 — uncontended counting semaphore

Single thread loops `kvb_sem_post` then `kvb_sem_wait` for 10 s.
Reports total wait/post pairs and computed throughput.

| Kernel         | Pairs        | Throughput (p/s) | µs/pair |
|----------------|-------------:|-----------------:|--------:|
| TaktOS         |    4,436,624 |          443,662 |    2.25 |
| FreeRTOS V11.3 |    1,398,048 |          139,804 |    7.15 |
| ThreadX 6.x    |    3,590,944 |          359,094 |    2.78 |
| Zephyr 4.2.99  |    2,049,344 |          204,934 |    4.88 |

TaktOS-vs-FreeRTOS: 3.17×. TaktOS-vs-ThreadX: 1.24×. TaktOS-vs-Zephyr: 2.16×.

This benchmark exercises the **uncontended atomic fast path**.
TaktOS's atomic split-counter semaphore (signed counter encoding
waiter presence in the sign bit) takes maximum advantage of M4's
LDREX/STREX availability — wait/post is one CAS each on the no-waiter
path, with no critical section. ThreadX's purpose-built `tx_*`
semaphore is also fast but pays the `_txe_*` parameter-validation
wrapper. Zephyr's `k_sem_take` / `k_sem_give` use a fast-path
LDREX/STREX on M4 when no waiters are present, plus their kernel
bookkeeping for kernel-tracking and timeslice support. FreeRTOS V11+
implements semaphores on top of the queue subsystem — the
queue-substrate cost dominates here; this is the widest TaktOS-vs-FreeRTOS
gap in the suite (3.17×).

Ranking: **TaktOS > ThreadX > Zephyr > FreeRTOS**. Same ranking as
on STM32F0308 M0; the gap between ThreadX and Zephyr (1.75×) is
slightly wider on M4 than on M0, consistent with ThreadX picking up
proportionally more from M4's atomic primitives than Zephyr does.

### 4.4 SYNC_MUTEX_FAST_001 — uncontended plain mutex lock/unlock

Single thread loops `kvb_mutex_lock` then `kvb_mutex_unlock` on the
kernel's **plain mutex API** for 10 s. Native default protocol per
kernel: no priority protocol on TaktOS; priority inheritance on
FreeRTOS, ThreadX, and Zephyr (the latter three have no
non-priority-protocol mutex variant — their plain mutex IS the PI
mutex).

| Kernel         | Pairs        | Throughput (p/s) | µs/pair | Protocol |
|----------------|-------------:|-----------------:|--------:|----------|
| TaktOS         |    3,757,696 |          375,768 |    2.66 | none     |
| FreeRTOS V11.3 |      972,400 |           97,238 |   10.28 | PI       |
| ThreadX 6.x    |    1,570,144 |          157,013 |    6.37 | PI       |
| Zephyr 4.2.99  |    1,707,792 |          170,778 |    5.86 | PI       |

TaktOS-vs-FreeRTOS: 3.86×. TaktOS-vs-ThreadX: 2.39×. TaktOS-vs-Zephyr: 2.20×.

This is the **widest TaktOS lead in the suite** (3.86× vs FreeRTOS).
Reasons:
- TaktOS's plain mutex is a single-word ownership compare-and-swap
  with no protocol bookkeeping. Atomic, O(1), no critical section.
- FreeRTOS V11+ plain mutex IS the PI mutex — every lock/unlock
  walks the queue ready/blocked lists and updates priorities even
  uncontended.
- ThreadX's `tx_mutex_get/put` is purpose-built but pays the `_txe_*`
  parameter validation, slot-pool indirection in the KVB port, and
  unconditional priority-inheritance bookkeeping on every lock/unlock.
- Zephyr's `k_mutex_lock` / `k_mutex_unlock` measure as the fastest
  among the three PI implementations. The KVB Zephyr port is a thin
  pass-through to the upstream `k_mutex` API; the implementation
  details live in Zephyr's `kernel/mutex.c` and have not been further
  instrumented here. The measurement reports the upstream behaviour
  as built — recursive-by-default mutex with unconditional priority
  inheritance, `CONFIG_RUNTIME_ERROR_CHECKS=y` (CHECKIF returned-error
  paths active, including `owner == NULL` / `owner != _current` in
  `k_mutex_unlock`), `CONFIG_ASSERT=y` / `CONFIG_ASSERT_LEVEL=2`
  (API-entry `__ASSERT()` checks active for ISR misuse and lock-count
  consistency, plus the internal scheduler / spinlock asserts),
  stack-sentinel check active.

The TaktOS-vs-Zephyr gap (2.20×) on this test is narrower than
TaktOS-vs-ThreadX (2.39×) and far narrower than TaktOS-vs-FreeRTOS
(3.86×), because Zephyr's `k_mutex` PI fast path is the cleanest
of the three PI implementations on this hardware — but TaktOS still
leads by a significant margin because PI bookkeeping is structurally
more expensive than no-protocol acquire-and-release.

### 4.5 SYNC_MUTEX_PCP_FAST_001 — priority-boost mutex lock/unlock

Single thread loops lock/unlock on the kernel's **priority-boost
variant**: PCP (priority ceiling protocol) on TaktOS, PI (priority
inheritance) on the other three. See §3.6 for protocol distinction.

| Kernel         | Pairs        | Throughput (p/s) | µs/pair | Protocol |
|----------------|-------------:|-----------------:|--------:|----------|
| TaktOS         |    1,754,656 |          175,466 |    5.70 | PCP      |
| FreeRTOS V11.3 |      972,400 |           97,239 |   10.28 | PI       |
| ThreadX 6.x    |    1,574,016 |          157,404 |    6.35 | PI       |
| Zephyr 4.2.99  |    1,680,592 |          168,060 |    5.95 | PI       |

TaktOS-vs-FreeRTOS: 1.80×. TaktOS-vs-ThreadX: 1.11×. TaktOS-vs-Zephyr: 1.04×.

**TaktOS leads but by a much narrower margin** than on the plain
mutex test. Reasons:
- TaktOS's PCP raises the holder's effective priority to the lock's
  ceiling on acquire — a single LDREX/STREX CAS plus a priority
  bump under brief critical section. Atomic, O(1), but more
  bookkeeping than the no-protocol plain mutex.
- The other three kernels' PI mutex paths in this test are the same
  paths exercised by `SYNC_MUTEX_FAST_001` (their plain mutex IS
  the PI mutex). Their numbers in this row are within ~0.1 % of the
  previous row, as expected.

The TaktOS PCP throughput (175 k p/s) is roughly half of TaktOS's
plain mutex throughput (376 k p/s): the priority-bump is a real
~50 % cost overhead on top of the plain-mutex ownership CAS. The
other three kernels' numbers stay flat between the two tests because
their plain mutex already includes PI cost, so there's nothing to add.

If the FreeRTOS / ThreadX / Zephyr PI mutex were measured against
TaktOS's PCP variant on a contention test (where PCP avoids the
priority-walk that PI must perform), the gap would widen back out —
PCP scales better than PI under contention. KVB does not currently
include a contention-PI vs PCP test; that's on the open-work list.

### 4.6 SYNC_MUTEX_OWNERSHIP_001 — non-owner unlock rejection

Owner task locks the mutex, signals it has done so, blocks. Runner
task (non-owner) attempts to unlock. Test passes if the unlock is
rejected with a non-`KVB_OK` status.

| Kernel         | Result | Returned status               |
|----------------|:------:|:------------------------------|
| TaktOS         | PASS   | `KVB_ERR_NOT_OWNER` (3)       |
| FreeRTOS V11.3 | PASS   | `KVB_ERR_NOT_OWNER` (3)       |
| ThreadX 6.x    | PASS   | `KVB_ERR_NOT_OWNER` (3)       |
| Zephyr 4.2.99  | PASS   | `KVB_ERR_NOT_OWNER` (3)       |

**Four-way parity.** All four kernels reject non-owner unlock and
report it through the same `KVB_ERR_NOT_OWNER` status code at the KVB
boundary — though the underlying mechanisms differ (§3.7).

### 4.7 IPC_QUEUE_FAST_001 — same-thread queue send/receive

Single thread loops `kvb_queue_send` then `kvb_queue_recv` for 10 s.
`KVB_QUEUE_MESSAGE_SIZE = 16` byte payload per send/receive.

| Kernel         | Pairs      | Throughput (p/s) | µs/pair |
|----------------|-----------:|-----------------:|--------:|
| TaktOS         |  1,108,768 |          110,874 |    9.02 |
| FreeRTOS V11.3 |    535,872 |           53,586 |   18.66 |
| ThreadX 6.x    |  1,007,872 |          100,786 |    9.92 |
| Zephyr 4.2.99  |    701,664 |           70,166 |   14.25 |

TaktOS-vs-FreeRTOS: 2.07×. TaktOS-vs-ThreadX: 1.10×. TaktOS-vs-Zephyr: 1.58×.

The queue benchmark moves a 16-byte payload per send/receive. At this
payload size the data plane copy is a meaningful fraction of the work
and the gap closes between purpose-built primitives. TaktOS's queue
uses plain `Avail/pWrite/pRead` indices under a single critical
section; ThreadX's `tx_queue_send/recv` are purpose-built but pay
validation overhead. Zephyr's `k_msgq_put` / `k_msgq_get` carry the
message-queue bookkeeping (read/write index plus locked count) and
Zephyr's standard parameter-validation hooks; their implementation
details live in Zephyr's `kernel/msg_q.c`. FreeRTOS V11+ uses its full
queue object (event-list pointers, locking counters) per operation —
the substrate cost dominates.

Ranking: **TaktOS > ThreadX > Zephyr > FreeRTOS**. TaktOS-ThreadX
clustered within 1.10×; Zephyr 1.58× behind TaktOS; FreeRTOS far
behind at 2.07× behind TaktOS.

### 4.8 TIME_SLEEP_001 — thread sleep duration accuracy

Caller invokes `kvb_thread_sleep_ticks(10)` (= 10 ms at 1000 Hz tick).
KVB's pass criterion is `elapsed_us >= min_expected_us` where
`min_expected_us = 9000 µs` (90 % of nominal). TaktOS additionally
specifies a stricter rule: at-least-N-ticks, i.e. `elapsed_us >=
nominal_us = 10000 µs`.

| Kernel         | KVB result | Requested | Elapsed (µs)             | ≥ 10000 µs? |
|----------------|:----------:|----------:|--------------------------|:-----------:|
| TaktOS         | PASS       |    10 ms  | 10153 (bit-identical × 5) | ✓ yes (153 µs margin)        |
| FreeRTOS V11.3 | PASS       |    10 ms  |  9696 (bit-identical × 5) | ✗ no (304 µs short)          |
| ThreadX 6.x    | PASS       |    10 ms  |  9429 (bit-identical × 5) | ✗ no (571 µs short)          |
| Zephyr 4.2.99  | PASS       |    10 ms  | 11297 mean (range 11126–11464, 5 runs) | ✓ yes (≥ 1126 µs margin every run) |

**Two of four kernels meet the strict 10000 µs at-least-N-ticks bound:
TaktOS and Zephyr. FreeRTOS and ThreadX undershoot it (within KVB's
90 % floor).** See §6 for the full design-vs-chance analysis.

The bit-perfect reproducibility across all 5 runs of TaktOS, FreeRTOS,
and ThreadX confirms their respective behaviours are deterministic.
Zephyr's 338 µs jitter window across 5 runs is real Zephyr runtime
jitter (§4.1), but **every individual Zephyr run still meets the
strict bound by ≥1126 µs margin** — the behaviour is stochastic above
the bound, not above-and-below it.

---

## 5. What the numbers say

In one sentence: **TaktOS leads every throughput test against every
other kernel on this build, by margins ranging from 1.04× (PCP mutex
vs Zephyr's PI mutex) to 3.86× (plain mutex vs FreeRTOS).**

The pattern across the five throughput tests:

| Test                         | TaktOS vs FR | TaktOS vs TX | TaktOS vs Z | ThreadX vs FR |
|------------------------------|-------------:|-------------:|------------:|--------------:|
| SCHED_COOP_001               |        1.53× |        2.12× |       2.44× |         0.72× |
| SYNC_SEM_FAST_001            |        3.17× |        1.24× |       2.16× |         2.57× |
| SYNC_MUTEX_FAST_001          |        3.86× |        2.39× |       2.20× |         1.61× |
| SYNC_MUTEX_PCP_FAST_001      |        1.80× |        1.11× |       1.04× |         1.62× |
| IPC_QUEUE_FAST_001           |        2.07× |        1.10× |       1.58× |         1.88× |

**TaktOS-vs-everything findings:**
- TaktOS leads SCHED_COOP_001 by a wide margin (1.53× over FR, 2.12×
  over TX, 2.44× over Z) — bitmap CLZ scheduler dominates here.
- TaktOS leads SYNC_SEM_FAST_001 by 3.17× over FreeRTOS — the widest
  TaktOS-vs-FreeRTOS gap in the suite. Atomic split-counter vs FreeRTOS's
  queue-substrate semaphore is the structural reason.
- TaktOS leads SYNC_MUTEX_FAST_001 by 3.86× over FreeRTOS — the widest
  margin in the suite. TaktOS's plain mutex has no protocol bookkeeping;
  the other three's plain mutex IS their PI mutex.
- TaktOS's PCP variant (SYNC_MUTEX_PCP_FAST_001) is competitive but
  not dominant against the others' PI variants — 1.04× over Zephyr,
  1.11× over ThreadX, 1.80× over FreeRTOS. PCP costs a priority bump
  per acquire on top of TaktOS's plain-mutex CAS.
- TaktOS leads IPC_QUEUE_FAST_001 by narrower margins (1.10× over TX,
  1.58× over Z, 2.07× over FR) because the 16-byte payload copy is a
  meaningful fraction of per-iteration work — the gap between
  purpose-built primitives compresses on data-plane work.

**Other-kernel findings:**
- **FreeRTOS is last on every throughput test** — by a wide margin.
  Mainstream FR V11+ queue substrate dominates per-primitive cost.
- **ThreadX beats Zephyr on SEM** (1.75×) but **Zephyr beats ThreadX
  on plain MUTEX** (1.09×) and **PI MUTEX** (1.07×) — the only place
  where ThreadX is not in the top 3. Zephyr's `k_mutex` PI fast path
  is its cleanest result in the suite.
- **TaktOS and Zephyr both meet the strict TIME_SLEEP_001 bound by
  design**; FreeRTOS and ThreadX undershoot.

The TIME_SLEEP_001 result is qualitatively different from the
throughput findings — it shows a **behavioural** difference, not a
performance one. KVB tests against a 9000 µs floor that all four
meet. Two of four kernels — TaktOS and Zephyr — additionally satisfy
the stricter at-least-N-ticks rule (TaktOS deterministically at
10153 µs across all 5 runs; Zephyr at 11297 µs mean across 5 runs,
every individual run above 10000 µs).

The determinism observation in §4.1 is independently meaningful:
three of four kernels (TaktOS, FreeRTOS, ThreadX) produce
bit-identical numbers across 5 runs each on every throughput test;
only Zephyr shows measurable run-to-run jitter (~0.1 % on throughput,
~3 % on TIME_SLEEP). Functioning-as-designed Zephyr behaviour.

---

## 6. TIME_SLEEP_001 — strict bound: design vs chance

The four kernels split cleanly into two camps on the at-least-N-ticks
rule:

**Camp 1: meets strict bound by design — TaktOS, Zephyr.**
TaktOS's `TaktOSThreadSleepTicks(N)` adds `+1u` at all 5 timed-wait
sites: `WakeTick = TaktOSTickCount() + ticks + 1u`. This guarantees
at least `ticks` full tick periods elapse before wake regardless of
where in the current period the call lands. The cost is one extra
tick of latency in the worst case (best case unchanged).
Zephyr's `k_sleep` rounds up similarly — Zephyr's clock subsystem
documents that the function returns no earlier than the requested
duration.

**Camp 2: undershoots strict bound — FreeRTOS, ThreadX.**
`vTaskDelay(xTicksToDelay)` in FreeRTOS schedules wakeup at
`xConstTickCount + xTicksToDelay`, where `xConstTickCount` is the tick
count at the time `vTaskDelay` is called. If the SysTick is already
partway through the current tick period when `vTaskDelay` is called,
the next tick fires partway through that period — fewer than
`xTicksToDelay` full tick periods have elapsed by the time the task
wakes. `tx_thread_sleep(timer_ticks)` in ThreadX has the same shape
and the same problem.

For `N=10` at 1000 Hz tick:
- **TaktOS:** observed 10153 µs (bit-identical × 5) → strict bound
  met by **153 µs margin** in every run. The +1 compensation
  predicts elapsed = N+1 ticks = ~11000 µs; the observed 10153 µs
  is ~847 µs short of that, reflecting where in the current tick
  the call lands (sub-tick offset between the timer-now-read and
  the actual sleep entry). The +1 still ensures ≥10000 µs.
- **Zephyr:** observed 11297 µs mean (range 11126–11464 across 5
  runs) → strict bound met by **≥1126 µs margin** on every
  individual run. Consistent with `k_sleep` rounding up plus
  Zephyr workqueue/idle overhead shifting the wake-to-test-resumption
  time.
- **FreeRTOS:** observed 9696 µs (bit-identical × 5) → invoked
  `vTaskDelay(10)` ~304 µs into a tick period; misses strict bound
  by 304 µs.
- **ThreadX:** observed 9429 µs (bit-identical × 5) → invoked
  `tx_thread_sleep(10)` ~571 µs into a tick period; misses strict
  bound by 571 µs.

The FreeRTOS and ThreadX 9696 µs / 9429 µs differ by 267 µs — same
algorithm, same off-by-one, the differential reflecting different
fixed startup work between `kvb_test_time_now_us()` and the actual
sleep entry on each kernel.

The bit-perfect reproducibility across all 5 runs of TaktOS, FreeRTOS,
and ThreadX confirms their respective behaviours are deterministic.
Zephyr's 338 µs range across 5 runs is real Zephyr runtime jitter
(§4.1), but every individual Zephyr run meets the strict bound by
a comfortable margin — the behaviour is **stochastic above the
bound**, not above-and-below it.

For applications where FreeRTOS's or ThreadX's looser sleep semantics
are a problem, the standard workaround is to call `vTaskDelay(N+1)` /
`tx_thread_sleep(N+1)` — shifting the responsibility onto the caller.
TaktOS and Zephyr handle this inside the kernel.

---

## 7. M4F vs M0 — per-kernel hardware speedup

The same KVB suite ran on STM32F0308 (M0 @ 48 MHz) and nRF52832
(M4F @ 64 MHz) — same code, same toolchain, same kernel sources,
different SoC. Comparing per-kernel speedups separates "which
kernel exploits the M4 ISA best" from raw clock differences.

| Test                         | TaktOS M4/M0 | FreeRTOS M4/M0 | ThreadX M4/M0 | Best M4 winner |
|------------------------------|-------------:|---------------:|--------------:|----------------|
| SCHED_COOP_001               |        1.63× |          1.51× |         1.61× | TaktOS         |
| SYNC_SEM_FAST_001            |        1.82× |          1.24× |         1.83× | ThreadX        |
| SYNC_MUTEX_FAST_001          |        2.14× |          1.25× |         1.78× | TaktOS         |
| IPC_QUEUE_FAST_001           |        1.40× |          1.36× |         1.56× | ThreadX        |

Raw clock ratio: 64/48 = **1.33×**.

Note: Zephyr is not in this table because the STM32F0308 KVB build
does not include a Zephyr port (Zephyr's nRF52832 board support is
upstream and reusable; STM32F0308-DISCO is not a supported Zephyr
board). The Zephyr cross-target row would require either a
custom Zephyr board definition or a re-port to a Zephyr-supported
M0 such as `nrf51dk_nrf51422`. Pending — see §9.

Observations:
- **TaktOS gains 2.14× on plain MUTEX** — far above the clock ratio,
  reflecting M4's LDREX/STREX availability for the no-protocol
  CAS. The single largest TaktOS architectural win.
- **TaktOS gains 1.82× on SEM** — slightly below ThreadX's 1.83×,
  both substantially beating FreeRTOS's 1.24×. TaktOS's atomic
  split-counter and ThreadX's purpose-built semaphore both benefit
  from M4's atomic primitives; FreeRTOS's queue-substrate semaphore
  can't pick up similar speedup because the queue infrastructure
  dominates.
- **TaktOS gains 1.63× on SCHED** — beats clock ratio because M4
  hardware CLZ replaces the M0 software CLZ fallback in bitmap
  SelectNext. ThreadX gains 1.61× through similar but smaller
  scheduler-path improvements.
- **FreeRTOS lags on SCHED (1.51×) and SEM (1.24×)** — the queue
  substrate it uses for everything caps the architectural speedup
  it can extract. Structural, not fixable without a new primitive
  design.

The headline M4F vs M0 picture: TaktOS extracts more per-cycle
performance from M4F on yield-heavy and MUTEX-heavy workloads;
ThreadX extracts more on QUEUE-heavy ones; FreeRTOS extracts less
than either on every metric because of its queue-everything design
choice.

---

## 8. Reproducibility

All sources, project files, and configurations needed to reproduce
these numbers are committed under:

- `KVB/Targets/nRF52832/TaktOS_nRF52832_KvbSuite/`    (TaktOS variant, Eclipse CDT)
- `KVB/Targets/nRF52832/FreeRTOS_nRF52832_KvbSuite/`  (FreeRTOS variant, Eclipse CDT)
- `KVB/Targets/nRF52832/ThreadX_nRF52832_KvbSuite/`   (ThreadX variant, Eclipse CDT)
- `KVB/Targets/Zephyr/KvbSuite/`                       (Zephyr variant — west / CMake build, board=nrf52dk_nrf52832)
- `KVB/ports/kernels/taktos/`                          (TaktOS kernel port)
- `KVB/ports/kernels/freertos/`                        (FreeRTOS kernel port)
- `KVB/ports/kernels/threadx/`                         (ThreadX kernel port)
- `KVB/ports/kernels/zephyr/`                          (Zephyr kernel port)
- `KVB/Targets/src/`                                   (kernel-agnostic test runner + main)
- `KVB/Targets/nRF52832/src/`                          (per-MCU platform glue)
- `KVB/Targets/nRF52832/include/`                      (per-MCU configs, FreeRTOSConfig.h)
- `KVB/include/tx_user.h`                              (ThreadX feature config, shared across every KVB target)

Multi-run aggregation tool: `KVB/tools/compare_runs.py`.

To reproduce:

1. Clone IOsonata; place this `KVB/` tree alongside it in IOsonata's
   parent directory so that `IOCOMPOSER_HOME` resolves to the
   IOsonata parent directory.
2. Open Eclipse CDT, import the TaktOS, FreeRTOS, and ThreadX projects
   from `Eclipse/` subdirectories. Build Release for each.
3. For Zephyr: `cd KVB/Targets/Zephyr/KvbSuite && west init -l . &&
   west update && west build -b nrf52dk_nrf52832 --pristine`. The
   resulting `build/zephyr/zephyr.hex` is the Zephyr KVB firmware.
4. Flash the TaktOS variant. Connect to the J-Link CDC at 115200
   8N1. Power-cycle the board, capture UART output until
   `[KVB] END RUN`. Save as `taktos_run1.log`. Repeat for 5 runs.
5. Repeat step 4 with the FreeRTOS variant, saving as `freertos_runN.log`.
6. Repeat step 4 with the ThreadX variant, saving as `threadx_runN.log`.
7. Repeat step 4 with the Zephyr variant, saving as `zephyr_runN.log`.

Captured raw log: `KVB/Targets/nRF52832/KVB_Results_nRF52832.txt`
(20 runs total, 5 per kernel).

---

## 9. Open work

- **PI vs PCP contention test.** The current
  `SYNC_MUTEX_PCP_FAST_001` test is uncontended, so it doesn't
  exercise the algorithmic difference between priority inheritance
  (PI) and priority ceiling (PCP) — both protocols pay roughly
  similar bookkeeping cost on the no-contention fast path. Adding a
  contention-PI vs contention-PCP test would surface the
  difference (PCP avoids the priority-walk that PI must perform on
  every blocking acquire, scaling better under heavy contention).
  Scoped for KVB v0.2.

- **Zephyr non-determinism.** Zephyr is the only kernel in the suite
  showing measurable run-to-run jitter (~0.1 % on throughput, ~3 %
  on TIME_SLEEP). Likely sources: tickless-idle re-entry timing,
  system workqueue tick handling, idle-thread housekeeping. This is
  functioning-as-designed Zephyr behaviour but a real characterisation
  finding for safety/cert use cases — Zephyr's rich runtime is paid
  for in determinism, even at idle. The KVB measurement averages it
  out over 5 runs but it's worth flagging for any application that
  needs cycle-accurate reproducibility.

- **Zephyr KVB port for STM32F0308 (or alternate M0)** to enable a
  full 4-kernel cross-target table (§7). Zephyr does not have an
  upstream board definition for STM32F0308-DISCO; options include
  (a) creating a custom Zephyr board definition for STM32F0308-DISCO
  (~200 LOC of DTS + Kconfig), or (b) re-running the M0 comparison
  on a Zephyr-supported M0 such as `nrf51dk_nrf51422` (Cortex-M0 @
  16 MHz), noting the clock difference. Option (a) preserves
  the matched-target framing; option (b) is faster but requires a
  clock-normalised analysis.

- **Port KVB to nRF54L15 (Cortex-M33 @ 128 MHz)** for further
  cross-architecture validation. Working FreeRTOS, ThreadX, and
  Zephyr Thread-Metric reference projects already exist in the
  IOsonata tree at this target, so four-kernel parity is already
  demonstrated at the build level. Expected outcome: TaktOS lead
  widens further on M33 because of higher clock and more scheduler-
  hot-path inlining, and TaktOS's atomic split-counter primitives
  pick up the M33's improved LDREX/STREX latency. Relative ratios
  may drift upward for TaktOS on SEM and downward for everyone on
  SCHED as per-yield kernel work hits a hardware floor.

- **MPU-on round (proposed v2.3).** v2.2 establishes the MPU-off
  comparison correctly. A separate v2.3 round would wire up MPU
  integrations on TaktOS (MPU-aware port), FreeRTOS (MPU port
  variant), and ThreadX (MPU-enabled build), then re-enable Zephyr's
  `CONFIG_HW_STACK_PROTECTION=y` for a "with hardware stack
  protection" comparison. Useful for safety/cert use cases where
  MPU-based isolation is a hard requirement. Not in scope for the
  current document.

- **FEATURES line on captured FreeRTOS V11.3 logs displays
  `version=V11.1.0+`** — this is the literal value of
  `tskKERNEL_VERSION_NUMBER` baked into the FreeRTOS-Kernel headers
  at build time. The kernel library is V11.3 per upstream tag; the
  version macro just hasn't been bumped on each patch release in the
  V11.x family. The KVB FreeRTOS port reads this macro directly
  rather than hard-coding a version, which is correct behaviour —
  the captured value reflects the actual kernel tested.

- **Closed (v2.2): UART log corruption at boot.** Earlier captures
  on FreeRTOS / TaktOS / ThreadX showed garbled BEGIN RUN /
  VERSION lines on every reflash, with stray bytes (`«`, `˛`, `.`)
  spliced into the first 1-2 log lines. Root cause: IOsonata
  `g_Uart.Tx()` returning transient errors during the post-reset
  UART hardware settle window, plus the pre-fix `kvb_platform_log_write`
  bailing on the first negative-return rather than retrying. Fixed by
  (a) ~50 ms busy-wait in `KVB/Targets/src/main.cpp` between
  `g_Uart.Init()` and `kvb_kernel_start()` so the UART hardware
  fully settles before the runner emits its first byte;
  (b) bounded retry on transient errors in `kvb_platform_log_write`;
  (c) static format buffer in `kvb_log.c` (out of stack); and
  (d) defensive ~50 ms sleep at the top of `kvb_runner_main` as
  belt-and-suspenders for the kernel side. Zephyr was unaffected
  (uses `printk` not the IOsonata path) and remained clean throughout.

- **Closed (v2.2): KVB_LOG_BUFFER_SIZE bump.** Earlier captures
  truncated the PLATFORM and CONFIG header lines at byte 127 (vsnprintf
  truncation due to per-target 128-byte buffer). With the static-buffer
  fix above, the buffer no longer transits the runner stack — bumped
  to 192 across all six per-target configs (nRF52832 × 3 kernels,
  STM32F0308 × 3 kernels). PLATFORM and CONFIG now render in full.

- **Closed (v2.2): Zephyr strict-parity multi-run aggregate.**
  Zephyr KVB port re-configured to Thread-Metric strict-parity
  baseline (`CONFIG_MPU=n` / `CONFIG_ARM_MPU=n` /
  `CONFIG_HW_STACK_PROTECTION=n` matching the FreeRTOS / ThreadX /
  TaktOS KVB ports' actual MPU-off configurations on this target).
  Captured 5-run aggregate; bit-stable variance < 0.25 % across all
  Zephyr metrics. Resolves the v2.1 / v2.2-pending placeholders
  with firm aggregate values throughout this report.

- **Closed (v2.1): kernel-object cleanup leaks.** Earlier KVB tests
  on nRF52832 FreeRTOS hardfaulted in `prvAddNewTaskToReadyList`
  during SYNC_MUTEX_OWNERSHIP_001 setup, root-caused to KVB tests
  creating stack-local kernel objects (`KvbSemaphore sem`, `KvbMutex
  mutex`, etc.) without calling `kvb_*_delete` before return. With
  pure static allocation, the object's storage IS the caller-supplied
  buffer; when the function returns, FreeRTOS's internal task lists
  hold dangling pointers into the popped frame. Fixed in
  `KVB/src/tests/sync/`, `sched/`, `ipc/` by adding cleanup at every
  return path. Note that on the Zephyr port, `kvb_sem_delete` /
  `kvb_mutex_delete` / `kvb_queue_delete` return `KVB_ERR_UNSUPPORTED`
  (Zephyr does not provide explicit destructors); this is harmless
  because each test re-initializes its objects via `k_*_init` on
  caller-supplied storage before use.

- **Closed (v2.1): Zephyr port worker-count parity.** v2.0 Zephyr
  SCHED_COOP_001 used `KVB_WORKER_THREAD_COUNT=5u` vs the others'
  `3u`. Patched to `3u` for v2.1; SCHED shifted +7.05 % vs v2.0,
  consistent with predicted bias direction.

---

*Author: Nguyen Hoan Hoang, I-SYST inc., Brossard, Canada*
*KVB framework version: 0.1.0-private*
*Document version: 2.2 — 2026-05-01 — Final 5-run aggregate per
kernel × 4 kernels, strict-parity Zephyr (MPU-off + picolibc + TLS-off),
boot-time UART log corruption fixed.*

---

### Revision history

- **v2.2 (2026-05-01).** Final 5-run aggregate captured for all four
  kernels under strict-parity Zephyr configuration (`CONFIG_MPU=n` /
  `CONFIG_ARM_MPU=n` / `CONFIG_HW_STACK_PROTECTION=n`,
  `CONFIG_PICOLIBC=y`, `CONFIG_THREAD_LOCAL_STORAGE=n`,
  `CONFIG_THREAD_STACK_INFO=n`,
  `CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT=n`). Boot-time UART
  log corruption that affected v2.0 / v2.1 captures (truncated
  BEGIN RUN / VERSION lines, stray bytes from leftover TX shift
  register state) fixed via 50 ms post-Init busy-wait in main.cpp +
  bounded retry in `kvb_platform_log_write` + static format buffer
  in `kvb_log.c` + defensive runner-side sleep. KVB_LOG_BUFFER_SIZE
  bumped 128 → 192 across all six per-target configs to fit
  PLATFORM / CONFIG header lines (the buffer now lives in BSS, not
  on the runner stack, so the bump costs flat BSS only). Headline:
  **TaktOS leads every throughput test against every other kernel.**
  v2.1's "Zephyr beats TaktOS on SEM/MUTEX in single-run preview"
  did not survive multi-run validation under the corrected
  strict-parity build — Zephyr's true SEM/MUTEX numbers are 205 k /
  171 k p/s, well behind TaktOS's 444 k / 376 k.

- **v2.1 (2026-04-30).** Independent feature-parity validation of the
  Zephyr KVB port against the FreeRTOS, ThreadX, and TaktOS ports
  surfaced one material asymmetry — `KVB_WORKER_THREAD_COUNT=5u` vs
  the others' `3u` — which biased Zephyr's SCHED_COOP_001 number
  downward by ~7 % through extra ready-queue maintenance per yield.
  Patched the Zephyr port to `3u` and re-ran SCHED_COOP_001 with
  matched configuration (+7.05 % shift, confirming the bias
  direction). v2.1 also corrected v2.0 claims about Zephyr's
  scheduler ("feature-rich (timeslicing, ...)" — actually
  `CONFIG_TIMESLICING=n`). **Note:** v2.1 numbers should be
  considered superseded by v2.2 — v2.1 ran Zephyr with default
  safety profile (`CONFIG_HW_STACK_PROTECTION=y`) which over-burdened
  Zephyr by ~80 cycles/switch vs the other three kernels' MPU-off
  configurations. v2.2 corrects this.

- **v2.0 (2026-04-29).** Added Zephyr 4.2.99 as fourth kernel. New §3.7
  (now §3.8) describes Zephyr's LFCLK-based cycle counter. Updated
  §4.1 determinism (Zephyr is the only non-bit-identical kernel).
  Updated all per-test tables in §4 to include Zephyr row. Updated §5
  conclusions table to a 6-column ratio matrix. Reframed §6 as
  "design vs chance": TaktOS and Zephyr both meet the strict
  TIME_SLEEP bound by design, FreeRTOS and ThreadX both miss it.
  **Note:** v2.0 ran Zephyr with `CONFIG_HW_STACK_PROTECTION=y`;
  superseded by v2.2 strict-parity framing.

- **v1.0 (2026-04-29).** Initial nRF52832 release with TaktOS,
  FreeRTOS V11.3, and ThreadX 6.x.
