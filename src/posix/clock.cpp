/**---------------------------------------------------------------------------
@file   clock.cpp

@brief  TaktOS PSE51  clock_gettime, clock_getres, and nanosleep implementation

CLOCK_MONOTONIC and CLOCK_REALTIME both map to the TaktOS tick count
converted to struct timespec.  Tick rate must match the tickHz argument
passed to TaktOSInit(); override with TAKT_POSIX_TICK_HZ if needed.

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
#include "TaktOS.h"
#include "TaktOSThread.h"

#include "posix/time.h"

// Tick rate: must match the tickHz argument passed to TaktOSInit().
// Override by defining TAKT_POSIX_TICK_HZ in the build system.
#ifndef TAKT_POSIX_TICK_HZ
#  define TAKT_POSIX_TICK_HZ  1000u
#endif

uint32_t takt_posix_tick_count(void)
{
    return TaktOSTickCount();
}

uint32_t takt_posix_tick_hz(void)
{
    return TAKT_POSIX_TICK_HZ;
}

static void ticks_to_timespec(uint32_t ticks, uint32_t hz, struct timespec* tp)
{
    tp->tv_sec  = (time_t)(ticks / hz);
    tp->tv_nsec = (long)((ticks % hz) * (1000000000UL / hz));
}

int clock_gettime(clockid_t clk_id, struct timespec* tp)
{
    if (!tp)
    {
        errno = EINVAL;
        return -1;
    }
    if (clk_id != CLOCK_MONOTONIC && clk_id != CLOCK_REALTIME)
    {
        errno = EINVAL;
        return -1;
    }
    ticks_to_timespec(TaktOSTickCount(), TAKT_POSIX_TICK_HZ, tp);
    return 0;
}

int clock_getres(clockid_t clk_id, struct timespec* res)
{
    if (!res)
    {
        errno = EINVAL;
        return -1;
    }
    if (clk_id != CLOCK_MONOTONIC && clk_id != CLOCK_REALTIME)
    {
        errno = EINVAL;
        return -1;
    }
    res->tv_sec  = 0;
    res->tv_nsec = (long)(1000000000UL / TAKT_POSIX_TICK_HZ);
    return 0;
}

int clock_settime(clockid_t, const struct timespec*)
{
    errno = EPERM; return -1;
}

int nanosleep(const struct timespec* req, struct timespec* rem)
{
    if (!req || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L)
    {
        errno = EINVAL;
        return -1;
    }

    const uint64_t ns_total = (uint64_t)req->tv_sec * 1000000000ULL
                            + (uint64_t)req->tv_nsec;
    const uint64_t ns_per_tick = 1000000000ULL / TAKT_POSIX_TICK_HZ;
    uint32_t ticks = (uint32_t)((ns_total + ns_per_tick - 1u) / ns_per_tick);
    if (ticks == 0u)
    {
        ticks = 1u;
    }

    // TaktOSThreadSleep blocks current thread for the specified tick count.
    TaktOSThreadSleep(TaktOSCurrentThread(), ticks);

    if (rem)
    {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    return 0;
}

int clock_nanosleep(clockid_t clk_id, int flags,
                    const struct timespec* req, struct timespec* rem)
{
    if (!req)
    {
        errno = EINVAL;
        return EINVAL;
    }
    if (clk_id != CLOCK_MONOTONIC && clk_id != CLOCK_REALTIME)
    {
        errno = EINVAL;
        return EINVAL;
    }

    if (flags & TIMER_ABSTIME)
    {
        struct timespec now;
        clock_gettime(clk_id, &now);
        uint64_t now_ns = (uint64_t)now.tv_sec * 1000000000ULL + now.tv_nsec;
        uint64_t abs_ns = (uint64_t)req->tv_sec * 1000000000ULL + req->tv_nsec;
        if (abs_ns <= now_ns)
        {
            return 0;
        }
        uint64_t delta  = abs_ns - now_ns;
        struct timespec rel;
        rel.tv_sec  = (time_t)(delta / 1000000000ULL);
        rel.tv_nsec = (long)(delta % 1000000000ULL);
        return nanosleep(&rel, rem);
    }
    return nanosleep(req, rem);
}
