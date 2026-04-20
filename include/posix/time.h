/**---------------------------------------------------------------------------
@file   time.h

@brief  TaktOS PSE51  clocks and nanosleep

Implements IEEE 1003.1-2017 clock_gettime / clock_getres /
clock_nanosleep / nanosleep.

CLOCK_MONOTONIC maps to TaktOS tick count converted to struct timespec.
CLOCK_REALTIME  is an alias for CLOCK_MONOTONIC on bare-metal (no RTC).
clock_nanosleep and nanosleep block the calling task via the native TaktOS
sleep list.

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
#ifndef __TIME_H__
#define __TIME_H__

#include "posix_types.h"

// TIMER_ABSTIME flag for clock_nanosleep
#define TIMER_ABSTIME   1

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief	Get the current value of a clock.
 *
 * CLOCK_MONOTONIC and CLOCK_REALTIME are both supported; CLOCK_REALTIME is
 * an alias for CLOCK_MONOTONIC on bare-metal (no RTC).  Resolution is one
 * kernel tick (1 ms at 1 kHz).
 *
 * @param	clk_id  : CLOCK_MONOTONIC or CLOCK_REALTIME.
 * @param	tp      : Receives the current time as a timespec.
 * @return	0 on success, EINVAL on unsupported clock ID.
 */
int clock_gettime(clockid_t clk_id, struct timespec* tp);

/**
 * @brief	Get the resolution of a clock.
 *
 * Returns the tick period (1 / TAKT_TICK_HZ seconds).
 *
 * @param	clk_id  : CLOCK_MONOTONIC or CLOCK_REALTIME.
 * @param	res     : Receives the clock resolution as a timespec.
 * @return	0 on success, EINVAL on unsupported clock ID.
 */
int clock_getres(clockid_t clk_id, struct timespec* res);

/**
 * @brief	Set a clock  not supported on bare-metal; always returns EPERM.
 *
 * @param	clk_id  : Clock ID (ignored).
 * @param	tp      : New time value (ignored).
 * @return	-1 with errno set to EPERM.
 */
int clock_settime(clockid_t clk_id, const struct timespec* tp);

/**
 * @brief	Suspend the calling thread for the specified relative interval.
 *
 * @param	req  : Requested sleep duration (relative).
 * @param	rem  : If non-NULL, receives remaining time on early wakeup
 *               (always zero on TaktOS bare-metal).
 * @return	0 on success, EINVAL on bad arguments.
 */
int nanosleep(const struct timespec* req, struct timespec* rem);

/**
 * @brief	Suspend the calling thread with optional absolute-time semantics.
 *
 * If @p flags == TIMER_ABSTIME the thread sleeps until the absolute time
 * @p req on @p clk_id; otherwise it sleeps for the relative duration @p req.
 *
 * @param	clk_id  : CLOCK_MONOTONIC or CLOCK_REALTIME.
 * @param	flags   : 0 for relative sleep, TIMER_ABSTIME for absolute.
 * @param	req     : Duration (relative) or wake-up time (absolute).
 * @param	rem     : If non-NULL and flags == 0, receives remaining time on
 *                   early wakeup (always zero on TaktOS bare-metal).
 * @return	0 on success, EINVAL on bad arguments.
 */
int clock_nanosleep(clockid_t clk_id, int flags, const struct timespec* req,
					struct timespec* rem);

#ifdef __cplusplus
}
#endif

#endif // TIME_H
