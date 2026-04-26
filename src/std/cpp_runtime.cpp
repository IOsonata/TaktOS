/**---------------------------------------------------------------------------
@file   cpp_runtime.cpp

@brief  First-draft C/C++ runtime lock hooks for TaktOS

This file supplies weak hooks for freestanding runtime components that need a
process-wide lock: malloc/new and C++ local-static initialization guards.  The
locks use TaktOS mutexes and are deliberately small.  Future Partitura support
can replace the allocator policy without changing the public standard thread
front ends.

QM - outside cert boundary.
----------------------------------------------------------------------------*/
#include "cxx/takt_cpp_runtime.h"

#include "TaktOSMutex.h"
#include "TaktKernel.h"

#include <stdint.h>

static TaktOSMutex_t s_malloc_mutex;
static TaktOSMutex_t s_guard_mutex;
static int           s_runtime_init_done = 0;

void TaktCppRuntimeInit(void)
{
    uint32_t state = TaktOSEnterCritical();
    if (s_runtime_init_done == 0)
    {
        (void)TaktOSMutexInit(&s_malloc_mutex);
        (void)TaktOSMutexInit(&s_guard_mutex);
        s_runtime_init_done = 1;
    }
    TaktOSExitCritical(state);
}

static void runtime_lock(TaktOSMutex_t *mutex)
{
    if (s_runtime_init_done == 0)
    {
        TaktCppRuntimeInit();
    }
    (void)TaktOSMutexLock(mutex, true, TAKTOS_WAIT_FOREVER);
}

static void runtime_unlock(TaktOSMutex_t *mutex)
{
    (void)TaktOSMutexUnlock(mutex);
}

extern "C" void __attribute__((weak)) __malloc_lock(void *)
{
    runtime_lock(&s_malloc_mutex);
}

extern "C" void __attribute__((weak)) __malloc_unlock(void *)
{
    runtime_unlock(&s_malloc_mutex);
}

extern "C" int __attribute__((weak)) __cxa_guard_acquire(uint64_t *guard)
{
    if (guard == nullptr)
    {
        return 0;
    }

    if ((*guard & 1u) != 0u)
    {
        return 0;
    }

    runtime_lock(&s_guard_mutex);
    if ((*guard & 1u) != 0u)
    {
        runtime_unlock(&s_guard_mutex);
        return 0;
    }
    return 1;
}

extern "C" void __attribute__((weak)) __cxa_guard_release(uint64_t *guard)
{
    if (guard != nullptr)
    {
        *guard |= 1u;
    }
    runtime_unlock(&s_guard_mutex);
}

extern "C" void __attribute__((weak)) __cxa_guard_abort(uint64_t *)
{
    runtime_unlock(&s_guard_mutex);
}
