# QEMU mps2-an386 test for thread signal, queue and mqueue

Functional test (`main.cpp`) and instruction-count bench (`bench.cpp`) on the
QEMU Cortex-M4F model. Not cycle accurate: counts are executed instructions
(gdb `stepi`, hardware exception entry counts as one step).

    export CMSIS=/path/to/CMSIS_5/CMSIS/Core/Include
    ./build.sh t.elf main.cpp            # add -DTAKT_INLINE_OPTIMIZATION for the inline build
    qemu-system-arm -M mps2-an386 -nographic -semihosting-config enable=on,target=native -kernel t.elf
    ./bench.sh BENCH_SIG

IRQ0 is pended from software through NVIC ISPR to act as a peripheral ISR.

The experimental signal is a wake-only latch:

    // ISR: publish device work, then wake its thread.
    TaktOSThreadSignal(hWorker);
    // Worker: wait, then inspect and drain device work.
    TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER);

Posts made before waiting coalesce. A second post after direct delivery is
latched for the next wait. No RX/TX mask or output payload is transferred.
The pending byte reuses TCB padding; the default 32-bit TCB remains 40 bytes.
This changes the experimental signal API: remove the Bits / Mask / pBits
arguments. SignalClear is removed; a nonblocking wait consumes a pending wake.
Timeout `0` remains nonblocking, even when `bBlocking` is true.

Apply the archive over the matching TaktOS source tree, including
`ARM/include/TaktKernelCore.h`. On Cortex-M, blocking signal, semaphore,
mutex and queue waits use the same `TaktOSCtxSwitchBlock()` with a compiler
memory clobber and DSB/ISB after the PendSV request. Self-suspend, sleep,
handoff and self-destroy use it too. ISR wake requests use the plain pend
with a compiler barrier. The fallback for other ports provides only a
compiler barrier; hardware ordering remains the port's responsibility.

Compare the same build modes (the script defaults to out-of-line functions):

    ./bench.sh BENCH_SEM
    ./bench.sh BENCH_SIG
    ./bench.sh BENCH_SEM -DTAKT_INLINE_OPTIMIZATION
    ./bench.sh BENCH_SIG -DTAKT_INLINE_OPTIMIZATION

`wake_insns` includes ISR entry, notification, context switching, and the
waiter's return. `roundtrip_insns` also includes blocking again and switching
back. Instruction counts do not establish hardware cycle latency.
