/**---------------------------------------------------------------------------
@file   sem.cpp

@brief  TaktOS PSE51  unnamed semaphores (sem_t implementation)

Implements sem_init / sem_destroy / sem_wait / sem_trywait /
sem_timedwait / sem_post on top of TaktOSSem_t.

Each sem_t occupies a slot in a fixed-size pool.  Named semaphores
(sem_open) are not supported (PSE51 profile).

QM  outside cert boundary.

@author Nguyen Hoan Hoang
@date   Apr. 2026

@license

MIT License

Copyright (c) 2026 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/
#include "posix/semaphore.h"
#include "TaktOSSem.h"

struct SemSlot { TaktOSSem_t s; bool in_use; };
static SemSlot s_sems[TAKT_POSIX_MAX_SEMS];

// Helper: translate TaktOSSemTake ticks argument for blocking/non-blocking.
static inline TaktOSErr_t sem_take(TaktOSSem_t* s, uint32_t ticks)
{
    if (ticks == TAKTOS_NO_WAIT)
    {
        return TaktOSSemTake(s, false, TAKTOS_NO_WAIT);
    }
    return TaktOSSemTake(s, true, ticks);
}

int sem_init(sem_t* sem, int /*pshared*/, unsigned int value)
{
    if (!sem)
    {
        errno = EINVAL;
        return -1;
    }
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_SEMS; ++i)
    {
        if (!s_sems[i].in_use)
        {
            TaktOSSemInit(&s_sems[i].s, value, TAKT_POSIX_SEM_MAX_VALUE);
            s_sems[i].in_use = true;
            *sem = static_cast<sem_t>(i);
            return 0;
        }
    }
    errno = ENOMEM; return -1;
}

int sem_destroy(sem_t* sem)
{
    if (!sem || *sem < 0 || (uint32_t)*sem >= TAKT_POSIX_MAX_SEMS)
    {
        errno = EINVAL;
        return -1;
    }
    s_sems[*sem].in_use = false;
    *sem = -1;
    return 0;
}

int sem_wait(sem_t* sem)
{
    if (!sem || *sem < 0)
    {
        errno = EINVAL;
        return -1;
    }
    return (TaktOSSemTake(&s_sems[*sem].s, true, TAKTOS_WAIT_FOREVER) == TAKTOS_OK)
           ? 0 : (errno = EINTR, -1);
}

int sem_trywait(sem_t* sem)
{
    if (!sem || *sem < 0)
    {
        errno = EINVAL;
        return -1;
    }
    return (TaktOSSemTake(&s_sems[*sem].s, false, TAKTOS_NO_WAIT) == TAKTOS_OK)
           ? 0 : (errno = EAGAIN, -1);
}

int sem_timedwait(sem_t* sem, const struct timespec* abs)
{
    if (!sem || *sem < 0 || !abs)
    {
        errno = EINVAL;
        return -1;
    }
    uint32_t ticks = takt_timespec_to_ticks(abs, takt_posix_tick_hz());
    return (sem_take(&s_sems[*sem].s, ticks) == TAKTOS_OK)
           ? 0 : (errno = ETIMEDOUT, -1);
}

int sem_post(sem_t* sem)
{
    if (!sem || *sem < 0)
    {
        errno = EINVAL;
        return -1;
    }
    return (TaktOSSemGive(&s_sems[*sem].s, false) == TAKTOS_OK)
           ? 0 : (errno = EOVERFLOW, -1);
}

int sem_getvalue(sem_t* sem, int* sval)
{
    if (!sem || *sem < 0 || !sval)
    {
        errno = EINVAL;
        return -1;
    }
    *sval = (int)s_sems[*sem].s.Count;
    return 0;
}

sem_t* sem_open(const char*, int, ...)
{
	errno = ENOTSUP; return (sem_t*)SEM_FAILED;
}

int sem_close(sem_t*)
{
	errno = ENOTSUP; return -1;
}

int sem_unlink(const char*)
{
	errno = ENOTSUP; return -1;
}


