#!/bin/bash
# Count executed instructions on one path with QEMU gdbstub + stepi.
# usage: CMSIS=... ./bench.sh BENCH_SEM|BENCH_SIG|BENCH_Q|BENCH_QNB|BENCH_MQ [extra flags]
cd "$(dirname "$0")"
MODE=$1; shift
./build.sh b.elf bench.cpp -D"$MODE" "$@" >/dev/null || exit 1
qemu-system-arm -M mps2-an386 -nographic -kernel b.elf -S -gdb tcp::1234 -monitor none -serial none >/dev/null 2>&1 &
sleep 0.5
gdb-multiarch -q -batch -x count.gdb b.elf 2>&1 | grep RESULT
kill %1 2>/dev/null; wait 2>/dev/null
