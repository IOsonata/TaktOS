/**---------------------------------------------------------------------------
@file   ptimer.h

@brief  TaktOS PSE51  POSIX interval timers (timer_*)

Implements IEEE 1003.1-2017 timer_create / timer_settime / timer_gettime /
timer_delete.

Each timer_t maps to a static pool slot.  Expiry dispatches the
sigevent.sigev_notify_function callback from the timer service task context
(SIGEV_THREAD).  SIGEV_SIGNAL is not supported on bare-metal.

NOTE: The standard header for timer_* is <time.h>, but ptimer.h is used
here to avoid a naming collision with the time.h that provides clock_*.

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
#ifndef __PTIMER_H__
#define __PTIMER_H__

#include "posix_types.h"


#ifndef TAKT_POSIX_MAX_TIMERS
#  define TAKT_POSIX_MAX_TIMERS  8u
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Create a per-process timer.
 *
 * Allocates a timer from the static pool and associates it with @p clk_id.
 * Only CLOCK_MONOTONIC is supported; CLOCK_REALTIME is accepted as an alias.
 * The notification method must be SIGEV_THREAD  the callback in
 * sevp->sigev_notify_function is called from the timer service task context.
 *
 * @param	clk_id   : Clock ID (CLOCK_MONOTONIC or CLOCK_REALTIME).
 * @param	sevp     : Notification specification.  sigev_notify must be
 *                    SIGEV_THREAD; sigev_notify_function must be non-NULL.
 * @param	timerid  : Receives the new timer ID on success.
 * @return	0 on success, EAGAIN if the timer pool is exhausted, EINVAL on
 *          bad arguments, ENOTSUP if SIGEV_SIGNAL is requested.
 */
int timer_create(clockid_t clk_id, struct sigevent* sevp, timer_t* timerid);

/**
 * @brief	Delete a timer.
 *
 * The timer is cancelled and its pool slot is released.  Any pending
 * expiry notification is discarded.
 *
 * @param	timerid : Timer ID returned by timer_create.
 * @return	0 on success, EINVAL if @p timerid is invalid.
 */
int timer_delete(timer_t timerid);

/**
 * @brief	Arm or disarm a timer.
 *
 * Sets the timer to expire at the interval described by @p new_value.
 * Setting both it_value fields to zero disarms the timer.
 *
 * @param	timerid    : Timer to configure.
 * @param	flags      : 0 for relative time, TIMER_ABSTIME for absolute time.
 * @param	new_value  : New expiry time (it_value) and reload interval
 *                      (it_interval; zero = one-shot).
 * @param	old_value  : If non-NULL, receives the previous timer setting.
 * @return	0 on success, EINVAL on bad arguments.
 */
int timer_settime(timer_t timerid, int flags, const struct itimerspec* new_value,
                  struct itimerspec* old_value);

/**
 * @brief	Query the remaining time until the next timer expiry.
 *
 * @param	timerid     : Timer to query.
 * @param	curr_value  : Receives the time remaining (it_value) and the
 *                       reload interval (it_interval).
 * @return	0 on success, EINVAL if @p timerid is invalid.
 */
int timer_gettime(timer_t timerid, struct itimerspec* curr_value);

/**
 * @brief	Return the overrun count for the most recent timer expiry.
 *
 * Returns the number of additional expirations that occurred between the
 * scheduled expiry and the delivery of the notification.  On TaktOS the
 * timer service task runs at a fixed priority and overruns are rare;
 * the maximum returned value is DELAYTIMER_MAX.
 *
 * @param	timerid : Timer to query.
 * @return	Overrun count  0 on success, -1 on error (EINVAL).
 */
int timer_getoverrun(timer_t timerid);

#ifdef __cplusplus
}
#endif

#endif // __PTIMER_H__
