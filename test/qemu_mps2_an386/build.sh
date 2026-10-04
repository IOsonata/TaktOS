#!/bin/bash
# Build a TaktOS test image for QEMU mps2-an386 (Cortex-M4F).
# usage: CMSIS=<path to CMSIS/Core/Include> ./build.sh <out.elf> <main source> [extra flags]
# example: ./build.sh t.elf main.cpp -DTAKT_INLINE_OPTIMIZATION
#          qemu-system-arm -M mps2-an386 -nographic -semihosting-config enable=on,target=native -kernel t.elf
set -e
cd "$(dirname "$0")"
R=../..
OUT=$1; MAIN=$2; shift 2
: "${CMSIS:?set CMSIS to the CMSIS/Core/Include directory}"
F="-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Os -g -ffunction-sections -fdata-sections -DTAKT_ARCH_CM4 -D__PROGRAM_START=__cmsis_start_unused -I$R/include -I$R/ARM/include -I$R/ARM/src -I$CMSIS $*"
CXX="arm-none-eabi-g++ $F -fno-exceptions -fno-rtti -std=c++17"
O=$(mktemp -d)
for s in $R/src/taktos.cpp $R/src/taktos_thread.cpp $R/src/taktos_sem.cpp $R/src/taktos_mutex.cpp \
         $R/src/taktos_queue.cpp $R/src/taktos_signal.cpp $R/ARM/src/TaktKernelCM.cpp \
         $R/src/posix/mqueue.cpp $R/src/posix/errno.cpp $R/src/posix/clock.cpp $MAIN; do
    $CXX -c $s -o $O/$(basename $s).o
done
arm-none-eabi-gcc $F -c startup.c -o $O/startup.o
arm-none-eabi-gcc $F -c $R/ARM/cm4/PendSV_M4.S -o $O/pendsv.o
arm-none-eabi-g++ $F -nostartfiles -T link.ld -Wl,--gc-sections --specs=nosys.specs $O/*.o -o $OUT
rm -rf $O
