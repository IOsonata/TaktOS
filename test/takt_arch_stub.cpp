/**---------------------------------------------------------------------------
@file   takt_arch_stub.cpp

@brief  Host-native (x86/x64) arch-port stubs for TAKT_ARCH_STUB=1 builds.

Provides lightweight implementations of the four functions that the kernel
sources call but that the ARM/RISC-V port files normally supply.  These stubs
allow test_kernel.cpp to link on the host without any target toolchain.

NOT in the safety boundary  used exclusively by the host unit-test build.

Build: included in test_kernel build line as an additional translation unit.
  g++ -std=c++20 -O0 -g -DTAKT_ARCH_STUB=1
      -I../include -I../ARM/include
      test_kernel.cpp takt_arch_stub.cpp
      ../src/taktos.cpp ../src/taktos_thread.cpp
      ../src/taktos_sem.cpp ../src/taktos_mutex.cpp
      ../src/taktos_queue.cpp
      -o test_kernel

----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "TaktOS.h"

//  TaktOSStackInit 
// On ARM this builds a fake Cortex-M exception frame so PendSV can return
// into the new task.  On the host, threads are never actually context-
// switched; TaktOSThreadCreate() only needs a non-null, in-range pSp value
// so that guard-word checks pass.  Return a pointer 64 bytes below the top
// of the provided stack region  enough headroom to satisfy any alignment
// requirements without touching any guard word.
extern "C"
void *TaktOSStackInit(void *pStackTop, void (*)(void*), void *)
{
    return static_cast<uint8_t*>(pStackTop) - 64;
}

//  TaktOSTickInit 
// No hardware tick source on the host.  The unit-test harness drives
// TaktKernelTickHandler() directly when tick-driven behaviour is tested.
extern "C"
void TaktOSTickInit(uint32_t, uint32_t, TaktOSTickClockSrc_t) {}

//  TaktOSStartFirst 
// Launches the first task on ARM via SVC #0.  Never called from unit tests
// (TaktOSStart() is not called; tests manipulate scheduler state directly).
// Provide a stub so the linker is satisfied.
extern "C"
void TaktOSStartFirst(void) {}

//  TaktOSStackOverflowHandler 
// Weak default is defined in taktos_thread.cpp; this strong definition is
// used by the host build (the weak symbol may not resolve correctly on all
// host linkers when the definition is in a separate .cpp).
extern "C"
void TaktOSStackOverflowHandler(hTaktOSThread_t)
{
    // On the host, treat a guard-word violation as a hard test failure.
    // The test framework will catch the non-zero exit code.
    __builtin_trap();
}
