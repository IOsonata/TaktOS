# Contributing to TaktOS

## Before you start

TaktOS has a formal certification boundary. Changes inside the boundary
(scheduler, thread, semaphore, mutex, queue, ARM ports, and any future
architecture port) carry different obligations than changes outside it
(POSIX PSE51 layer, docs, tests, examples, benchmarks). Read this document
before opening a pull request.

---

## Cert boundary vs QM

| Area | Boundary | What this means for contributors |
|---|---|---|
| `include/` (kernel public headers) | **In** | Any change requires an updated MC/DC note in the PR |
| `src/taktos.cpp`, `src/taktos_thread.cpp`, `src/taktos_sem.cpp`, `src/taktos_mutex.cpp`, `src/taktos_queue.cpp` | **In** | Same |
| `ARM/include/`, `ARM/src/TaktKernelCM.cpp` | **In** | Reviewed as kernel code |
| `ARM/cm0/PendSV_M0.S`, `ARM/cm4/PendSV_M4.S`, `ARM/cm7/PendSV_M7.S`, `ARM/cm33/PendSV_M33.S`, `ARM/cm55/PendSV_M55.S` | **In** | Assembly changes require per-variant branch coverage |
| Any new architecture port (e.g., `RISCV/`, `XTENSA/`) | **In** | Same rules as ARM ports. See _Contributing a new architecture port_ below. |
| `src/posix/`, `include/posix/` | **QM** | Normal review, no MC/DC obligation |
| `test/` | Out | Normal review |
| `examples/`, `Benchmark/` | Out | Normal review |
| `docs/`, `README.md` | Out | Normal review |

If your change touches a cert-boundary file, your PR description must include:

1. Which branches / decisions changed
2. Whether the change adds, removes, or modifies a branch
3. Confirmation that the existing MC/DC test vectors still cover all paths,
   or a list of new vectors needed in `test/test_kernel.cpp`

---

## Coding standards

### Language

- **C++23 freestanding** for all cert-boundary code. Build flag: `-std=gnu++23`.
  Permitted standard headers: `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`,
  `<string.h>` for `memcpy`/`memset` on kernel objects. No `<iostream>`,
  `<string>`, `<vector>`, or any hosted C++ headers.
- **`-fno-exceptions -fno-rtti`** — no exception handling, no RTTI.
- **GNU ARM Thumb-2 assembly** for the per-arch `PendSV_M*.S` files. Use
  `.syntax unified`. One file per ARM variant (M0, M4, M7, M33, M55).
- **Assembly for new architecture ports** follows the same conventions:
  GNU toolchain, snake_case labels, cycle-count annotations in comments,
  one context-switch file per ISA variant. Example path for a RISC-V RV32
  port: `RISCV/rv32/ctx_switch.S`.

### Includes

- Project headers use double quotes: `#include "TaktOS.h"`. Never use
  angle brackets for project headers.
- Angle brackets are for compiler / libc headers only: `<stdint.h>`,
  `<stdbool.h>`, `<string.h>`, etc.

### Naming

Actual conventions as used throughout the current tree:

| Kind | Convention | Example |
|---|---|---|
| Kernel object types (C) | `TaktOS*_t` | `TaktOSSem_t`, `TaktOSMutex_t`, `TaktOSQueue_t`, `TaktOSThread_t` |
| Opaque handles (C) | `hTaktOS*_t` | `hTaktOSThread_t`, `hTaktOSSem_t` |
| C struct tags | `__TaktOS*_s` | `__TaktOSSem_s`, `__TaktOSMutex_s` — underscore prefix lets a C++ class wrapper share the typedef name |
| Public C functions | `TaktOS*` | `TaktOSInit`, `TaktOSSemGive`, `TaktOSThreadCreate` |
| Internal kernel helpers | `Takt*` | `TaktReadyTask`, `TaktBlockTask`, `TaktForceNextThread` |
| C++ wrapper classes | `TaktOS*` (no `_t`) | `TaktOSSem`, `TaktOSMutex`, `TaktOSThread`, `TaktOSQueue` |
| Struct fields | PascalCase, no prefix | `Count`, `MaxCount`, `Priority`, `WakeTick`, `pOwner`, `pNext` |
| Pointer fields | `p` prefix, then PascalCase | `pSp`, `pNext`, `pStackBottom`, `pMsg`, `pWaitList` |
| File-static globals | `s_PascalCase` | `s_RunQueue`, `s_IdleThreadMem`, `s_TickClockSrc` |
| Extern globals | `g_PascalCase` | `g_TaktosCtx`, `g_TickCount` |
| Constants / macros | `TAKTOS_` or `TAKT_` prefix | `TAKTOS_WAIT_FOREVER`, `TAKT_WOKEN_BY_EVENT`, `TAKTOS_THREAD_MEM_SIZE` |
| Assembly labels | snake_case | `_takt_ctx_switch_rv`, `.skip_mpu` |

No `m_` prefix is used. No `namespace` wrappers in C headers. C++ wrapper
classes do not declare a `using namespace` in any header.

### Formatting

- 4-space indent. No tabs.
- Line length: 100 columns soft limit, 120 hard limit.
- **Opening brace on the next line** for functions, control flow, and classes
  (Allman style — this matches the entire existing source tree).
- One blank line between logical sections within a function.
- Assembly: opcode at column 8, operands at column 16, comment at column 44.

```c
TaktOSErr_t TaktOSSemInit(TaktOSSem_t *pSem, uint32_t Initial, uint32_t MaxCount)
{
    if (pSem == nullptr || MaxCount == 0u || Initial > MaxCount)
    {
        return TAKTOS_ERR_INVALID;
    }

    pSem->Count    = Initial;
    pSem->MaxCount = MaxCount;
    pSem->WaitList.pHead = nullptr;

    return TAKTOS_OK;
}
```

```asm
        CPSID   I                       /* EnterCritical */
        MRS     r0, PSP                 /* R0 = current task PSP */
        STMDB   r0!, {r4-r11, lr}       /* save callee-save regs + EXC_RETURN */
```

### Memory allocation

No `new`, `delete`, `malloc`, or `free` anywhere in the cert boundary.
All kernel objects are user-allocated (static or from application-owned
buffers). If a resource can be exhausted, the function returns a failure
code; it never silently drops data and never blocks indefinitely without
a timeout escape.

Thread memory is sized with `TAKTOS_THREAD_MEM_SIZE(StackSize)` — declared
in `include/TaktOSThread.h`. The per-arch overhead component
(`TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD`) lives in `ARM/include/TaktKernelCore.h`
and must be defined in every new architecture port's `<ARCH>/include/TaktKernelCore.h`.

### Critical sections

All kernel objects in this release serialize their operations with
`TaktOSEnterCritical()` / `TaktOSExitCritical()`. Critical sections must
be as short as possible:

- Never call a function from inside a critical section that may itself try
  to enter one.
- Never block (sleep, take a semaphore, wait on a queue) with the critical
  section held.
- The fast path may be force-inlined by defining `TAKT_INLINE_OPTIMIZATION`;
  the slow paths stay out-of-line (`TAKT_COLD TAKT_NOINLINE`).

If a PR introduces atomics or lock-free code paths inside the cert boundary,
the design must be discussed in a GitHub Discussion before implementation.
Atomics are not part of the current kernel design.

### No ISA leakage

No file outside an architecture-specific subdirectory (`ARM/`, and any future
`RISCV/`, `XTENSA/`, etc.) may contain architecture-specific instructions or
MMIO references. Examples of things that must stay inside the per-arch tree:

- `CPSID`, `CPSIE`, `MRS PRIMASK`, `MSR PRIMASK` (ARM)
- `CLZ`, `LDREX`, `STREX` in inline assembly (ARM)
- `csrci`, `csrsi`, `csrr`, `csrw` (RISC-V CSR instructions, if a port is added)
- Any MMIO address literal

The portable kernel calls per-arch primitives through `TaktOSEnterCritical` /
`TaktOSExitCritical` (declared in the arch's `TaktOSCriticalSection.h`) and
through the weak handler-assignment interface in `TaktKernelCore.h`.

### POSIX PSE51 layer

`src/posix/` and `include/posix/` implement a POSIX PSE51 subset
(pthread, semaphore, mqueue, POSIX timers, clock, errno) on top of the
native kernel. This layer is outside the cert boundary (QM). Keep it
self-contained: do not let POSIX types leak into the native kernel headers,
and do not call POSIX functions from cert-boundary code.

There is no FreeRTOS shim and none is planned. FreeRTOS appears only as a
benchmark comparison target in `Benchmark/ThreadMetric/`.

---

## Commit messages

```
component: short imperative summary (<= 72 chars)

Optional body explaining WHY, not what. The diff shows what.
Reference open items with: Closes #N or See #N.

Cert-boundary note (if applicable):
  Branches changed: TaktOSSemGive fast-path on WaitList.pHead == NULL
  MC/DC impact: existing vectors cover both outcomes; no new vector needed
```

Component prefixes: `sched`, `thread`, `sem`, `mutex`, `queue`, `tick`,
`arm`, `posix`, `test`, `examples`, `bench`, `docs`, `build`. New architecture
ports add their own prefix (e.g., `riscv`, `xtensa`).

---

## Building and testing locally

### Host-native unit tests

A single compile command from the `test/` directory, no CMake:

```bash
cd test
g++ -std=gnu++23 -O0 -g -DTAKT_ARCH_STUB=1 -DTAKT_TEST_ACCESS=1 \
    -I../include -I../ARM/include -I. \
    test_kernel.cpp takt_arch_stub.cpp \
    ../src/taktos.cpp ../src/taktos_thread.cpp \
    ../src/taktos_sem.cpp ../src/taktos_mutex.cpp \
    ../src/taktos_queue.cpp \
    -o test_kernel && ./test_kernel
```

Pass criterion: exit code 0, "ALL TESTS PASSED" on stdout. Run the same
build with and without `-DTAKT_INLINE_OPTIMIZATION=1` when your change
touches the semaphore, mutex, or queue fast paths. Run under ASAN/UBSAN
(`-fsanitize=address,undefined`) for any kernel object change.

### Target builds (Eclipse)

Each target port has an Eclipse project:

| Target | Project path |
|---|---|
| Cortex-M0/M0+ | `ARM/cm0/Eclipse/` |
| Cortex-M4 | `ARM/cm4/Eclipse/` |
| Cortex-M7 | `ARM/cm7/Eclipse/` |
| Cortex-M33 | `ARM/cm33/Eclipse/` |

Toolchain: xPack `arm-none-eabi-`. New architecture ports follow the same
layout: `<ARCH>/<variant>/Eclipse/` with the appropriate cross toolchain.

### Example syntax-check harness

From the repo root:

```bash
cd examples && make check
# or
./examples/verify_examples.sh
```

Both run a syntax-only pass over every example using the current public
headers. Override `CC` / `CXX` to cross-check with the target toolchain.

### Benchmarks

The Thread-Metric ports live in `Benchmark/ThreadMetric/<board>/`. Each
has its own Eclipse project. TaktOS runs TM1, TM2, TM3, TM6, TM7, and TM8;
TM4 (interrupt preemption) and TM5 (memory allocation) are not run because
TaktOS does not own application IRQs and does not have a heap.

---

## Contributing a new architecture port

TaktOS currently ships ARM Cortex-M (M0/M0+, M4/M4F, M7, M33, M55). Ports
for other architectures — RISC-V RV32, Xtensa, AArch64, etc. — are welcome
as contributions. A port must follow the same structure as the ARM ports so
that the portable kernel code stays unchanged and the certification boundary
remains clean.

### Required directory layout

```
<ARCH>/
├── include/
│   ├── TaktOSCriticalSection.h     # EnterCritical / ExitCritical macros or inlines
│   ├── TaktKernelCore.h            # TAKTOS_THREAD_STACK_LAYOUT_OVERHEAD,
│   │                               # stack frame layout, TaktOSStackInit signature
│   └── TaktKernelTick.h            # tick source init / start declarations
├── <variant>/
│   ├── Eclipse/                    # Eclipse project for this specific silicon variant
│   ├── ctx_switch.S                # context-switch assembly (the equivalent of PendSV)
│   └── <port>.cpp                  # tick init, stack init, ISR plumbing
└── src/
    └── <port>.cpp                  # shared per-arch code (if any)
```

Example paths a RISC-V RV32 port would use: `RISCV/include/`,
`RISCV/rv32/Eclipse/`, `RISCV/rv32/ctx_switch.S`, `RISCV/rv32/clint.cpp`.

### What the port must provide

1. **Critical section primitives** — `TaktOSEnterCritical()` /
   `TaktOSExitCritical()` with the same semantics as the ARM version:
   returns a saveable state token, nests correctly, is safe from ISR and
   task context.
2. **Context switch** — a single assembly file that saves the outgoing
   thread's full callee-save set plus any return state, loads the next
   thread's state, and resumes. Register-save / register-restore must be
   provably symmetric.
3. **Stack init** — `TaktOSStackInit()` that builds a valid interrupt
   return frame on a fresh stack so the first resume enters the thread
   entry with the correct arguments.
4. **Tick source** — a platform timer interface (the ARM port uses SysTick;
   a RISC-V port would typically use the CLINT timer or a vendor timer block).
5. **Trigger context switch** — a primitive that causes the scheduler to
   run at the next safe point (PendSV on ARM; software-interrupt or
   equivalent on other ISAs).

### What the port must NOT do

- It must not modify any file in `include/` or `src/` (the portable kernel).
  If a port feels it needs to, stop and open a GitHub Discussion — that
  means the portable abstraction is missing something.
- It must not depend on heap allocation.
- It must not expose ISA-specific instructions or MMIO to portable callers
  (see _No ISA leakage_ above).

### Validation before submitting

- Build from the Eclipse project cleanly with `-Os` and `-O2`.
- Run Thread-Metric TM1, TM2, TM3, TM6, TM7, TM8 on real silicon for at
  least one variant and include the results in the PR description.
- Provide the register-save / register-restore symmetry analysis for the
  context-switch routine (either an annotated source listing or a short
  note mapping each save to its paired restore).
- Confirm host-native unit tests still pass with the stub arch layer.

### Certification posture for new ports

A new port joins the certification boundary and will be held to the same
MC/DC + branch coverage requirements as the ARM ports. Contributors do
not need to deliver certification evidence themselves — the kernel team
will fold the port into the SEooC evidence package during the next cycle —
but the code must be **structured** so that evidence generation is tractable:
no hidden branches, no runtime code generation, no indirect calls that
cannot be statically enumerated.

---

## Pull request process

1. Fork and create a branch: `feat/short-description` or `fix/short-description`.
2. Run the host-native unit tests locally, both with and without
   `TAKT_INLINE_OPTIMIZATION`, under ASAN and UBSAN.
3. If the change touches a cert-boundary file, include the MC/DC impact
   note in your PR description.
4. If the change touches an ARM PendSV file, state which variants you
   tested on target and include Thread-Metric deltas (TM1/TM2/TM3/TM6/TM7/TM8)
   on nRF52832 or nRF54L15 vs baseline.
5. If the change adds a new architecture port, state which targets you
   validated on, include Thread-Metric deltas from at least one of those
   targets, and include a register-save / register-restore symmetry
   analysis for the context-switch routine.
6. At least one maintainer review is required before merge.
7. Squash on merge if the branch has more than 3 commits.

---

## What we will not merge

- Heap allocation inside the cert boundary
- Hosted C++ headers (`<iostream>`, `<string>`, `<vector>`, etc.) inside
  the cert boundary
- A FreeRTOS compatibility layer in any form (see POSIX PSE51 note above)
- Breaking changes to the public C API in `include/TaktOS.h`,
  `include/TaktOSThread.h`, `include/TaktOSSem.h`, `include/TaktOSMutex.h`,
  or `include/TaktOSQueue.h` without a migration note
- Changes to any `PendSV_M*.S` file (or the equivalent context-switch file
  in a new architecture port) without per-variant branch coverage analysis
  and register-save / register-restore symmetry proof
- Any change that regresses Thread-Metric TM2 / TM3 / TM6 / TM7 / TM8 on
  nRF52832 or nRF54L15 without a justified trade-off documented in the PR

---

## Questions

Open a GitHub Discussion rather than an Issue for design questions,
porting inquiries, or certification questions. Issues are for confirmed
bugs and specification compliance gaps only.
