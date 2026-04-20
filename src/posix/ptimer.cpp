/**---------------------------------------------------------------------------
@file   ptimer.cpp

@brief  TaktOS PSE51  POSIX interval timer implementation (timer_*)

Implements timer_create / timer_settime / timer_gettime / timer_delete.

Each POSIX timer owns a small service task.  The task either blocks
forever waiting for a control event (create / settime / delete / disarm)
or blocks with a timeout equal to the armed expiry tick.  A timeout means
the timer expired; a posted semaphore means the configuration changed.

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
#include "posix/ptimer.h"
#include "posix/time.h"
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktKernel.h"         // TaktOSEnterCritical / TaktOSExitCritical
#include <string.h>

static uint32_t timespec_to_ticks_rel(const struct timespec& ts)
{
    const uint32_t hz = takt_posix_tick_hz();
    const uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    uint64_t ticks = (ns * hz) / 1000000000ULL;
    if (ticks == 0u && (ts.tv_sec != 0 || ts.tv_nsec != 0))
    {
        ticks = 1u;
    }
    return (uint32_t)ticks;
}

// Timer service task stack + TCB bundled as one block per TaktOS convention.
#define PTIMER_TASK_STACK_SZ  512u

struct PtimerSlot {
    struct sigevent   sevp;
    struct itimerspec value;
    uint32_t          arm_tick;
    int               overrun;
    bool              in_use;

    TaktOSSem_t       fire_sem;
    uint8_t           task_mem[TAKTOS_THREAD_MEM_SIZE(PTIMER_TASK_STACK_SZ)];
};

static PtimerSlot s_ptimers[TAKT_POSIX_MAX_TIMERS];

static void ptimer_task(void* arg)
{
    PtimerSlot* slot = static_cast<PtimerSlot*>(arg);

    while (true)
    {
        struct sigevent sevp_copy{};
        uint32_t arm_tick = 0u;

        uint32_t primask = TaktOSEnterCritical();
        if (!slot->in_use)
        {
            TaktOSExitCritical(primask);
            break;
        }
        arm_tick = slot->arm_tick;
        sevp_copy = slot->sevp;
        TaktOSExitCritical(primask);

        if (arm_tick == 0u)
        {
            TaktOSSemTake(&slot->fire_sem, true, TAKTOS_WAIT_FOREVER);
            continue;
        }

        const uint32_t now = TaktOSTickCount();
        if (arm_tick > now)
        {
            const bool signaled = (TaktOSSemTake(&slot->fire_sem, true,
                                                  arm_tick - now) == TAKTOS_OK);
            if (signaled)
            {
                continue;
            }
        }

        primask = TaktOSEnterCritical();
        if (!slot->in_use)
        {
            TaktOSExitCritical(primask);
            break;
        }
        if (slot->arm_tick == 0u)
        {
            TaktOSExitCritical(primask);
            continue;
        }

        const uint32_t period_ticks = timespec_to_ticks_rel(slot->value.it_interval);
        if (period_ticks != 0u)
        {
            slot->arm_tick = TaktOSTickCount() + period_ticks;
        }
        else
        {
            slot->arm_tick = 0u;
        }
        slot->overrun = 0;
        sevp_copy = slot->sevp;
        TaktOSExitCritical(primask);

        if (sevp_copy.sigev_notify == SIGEV_THREAD &&
            sevp_copy.sigev_notify_function != nullptr)
        {
            sevp_copy.sigev_notify_function(sevp_copy.sigev_value);
        }
    }
}

int timer_create(clockid_t clk_id, struct sigevent* sevp, timer_t* timerid)
{
    if (!timerid)
    {
        errno = EINVAL;
        return -1;
    }
    if (clk_id != CLOCK_MONOTONIC && clk_id != CLOCK_REALTIME)
    {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < (int)TAKT_POSIX_MAX_TIMERS; ++i)
    {
        if (!s_ptimers[i].in_use)
        {
            PtimerSlot* slot = &s_ptimers[i];
            memset(slot, 0, sizeof(PtimerSlot));
            slot->in_use = true;
            slot->sevp   = sevp ? *sevp : sigevent{ SIGEV_NONE, 0, {0}, nullptr, nullptr };

            TaktOSSemInit(&slot->fire_sem, 0u, 1u);

            TaktOSThread_t* t = TaktOSThreadCreate(
                slot->task_mem, sizeof(slot->task_mem),
                ptimer_task, slot, 20u);

            if (!t)
            {
                slot->in_use = false;
                errno = EAGAIN; return -1;
            }
            *timerid = i;
            return 0;
        }
    }
    errno = EAGAIN; return -1;
}

int timer_delete(timer_t timerid)
{
    if (timerid < 0 || (uint32_t)timerid >= TAKT_POSIX_MAX_TIMERS)
    {
        errno = EINVAL;
        return -1;
    }
    PtimerSlot* slot = &s_ptimers[timerid];
    if (!slot->in_use)
    {
        errno = EINVAL;
        return -1;
    }

    uint32_t primask = TaktOSEnterCritical();
    slot->in_use   = false;
    slot->arm_tick = 0u;
    TaktOSExitCritical(primask);
    TaktOSSemGive(&slot->fire_sem, false);
    return 0;
}

int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec* new_value,
                  struct itimerspec* old_value)
{
    if (timerid < 0 || (uint32_t)timerid >= TAKT_POSIX_MAX_TIMERS || !new_value)
    {
        errno = EINVAL;
        return -1;
    }
    PtimerSlot* slot = &s_ptimers[timerid];
    if (!slot->in_use)
    {
        errno = EINVAL;
        return -1;
    }

    uint32_t primask = TaktOSEnterCritical();
    if (old_value)
    {
        *old_value = slot->value;
    }
    slot->value   = *new_value;
    slot->overrun = 0;

    if (new_value->it_value.tv_sec == 0 && new_value->it_value.tv_nsec == 0)
    {
        slot->arm_tick = 0u;
        TaktOSExitCritical(primask);
        TaktOSSemGive(&slot->fire_sem, false);
        return 0;
    }

    uint32_t start_tick;
    if (flags & TIMER_ABSTIME)
    {
        const uint32_t hz = takt_posix_tick_hz();
        start_tick = (uint32_t)((uint64_t)new_value->it_value.tv_sec * hz
                              + (uint64_t)new_value->it_value.tv_nsec
                                / (1000000000ULL / hz));
    }
    else
    {
        start_tick = TaktOSTickCount()
                   + timespec_to_ticks_rel(new_value->it_value);
    }
    slot->arm_tick = start_tick;
    TaktOSExitCritical(primask);
    TaktOSSemGive(&slot->fire_sem, false);
    return 0;
}

int timer_gettime(timer_t timerid, struct itimerspec* curr)
{
    if (timerid < 0 || (uint32_t)timerid >= TAKT_POSIX_MAX_TIMERS || !curr)
    {
        errno = EINVAL;
        return -1;
    }
    PtimerSlot* slot = &s_ptimers[timerid];
    if (!slot->in_use)
    {
        errno = EINVAL;
        return -1;
    }

    *curr = slot->value;
    if (slot->arm_tick == 0u)
    {
        curr->it_value = {0, 0};
    }
    else
    {
        const uint32_t hz  = takt_posix_tick_hz();
        const uint32_t now = TaktOSTickCount();
        const uint32_t rem = (slot->arm_tick > now) ? (slot->arm_tick - now) : 0u;
        const uint64_t ns  = (uint64_t)rem * (1000000000ULL / hz);
        curr->it_value.tv_sec  = (time_t)(ns / 1000000000ULL);
        curr->it_value.tv_nsec = (long)(ns % 1000000000ULL);
    }
    return 0;
}

int timer_getoverrun(timer_t timerid)
{
    if (timerid < 0 || (uint32_t)timerid >= TAKT_POSIX_MAX_TIMERS)
    {
        errno = EINVAL;
        return -1;
    }
    return s_ptimers[timerid].overrun;
}
