/**---------------------------------------------------------------------------
@file   posix_types.h

@brief  TaktOS PSE51  shared primitive types for the POSIX layer

Provides struct timespec, CLOCK_* identifiers, errno infrastructure, and
standard POSIX error codes.  Include this from all other PSE51 headers
rather than pulling in a full hosted libc.

PSE51 profile (IEEE 1003.1-2017 A.2.1): single-process, no fork/exec,
no filesystem, no stdio.  Threads + mutexes + semaphores + message queues
+ timers + clocks.

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
#ifndef __POSIX_TYPES_H__
#define __POSIX_TYPES_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ssize_t  not provided by freestanding stdint.h */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

//  Time types 
typedef long            time_t;
typedef long            suseconds_t;
typedef uint32_t        clockid_t;
typedef int             timer_t;

#ifndef __timespec_defined
#define __timespec_defined
struct timespec {
    time_t  tv_sec;
    long    tv_nsec;
};
#endif

struct itimerspec {
    struct timespec it_interval;
    struct timespec it_value;
};

//  Clock IDs 
#define CLOCK_REALTIME           ((clockid_t)0)
#define CLOCK_MONOTONIC          ((clockid_t)1)
#define CLOCK_PROCESS_CPUTIME_ID ((clockid_t)2)
#define CLOCK_THREAD_CPUTIME_ID  ((clockid_t)3)

//  Signal event 
#define SIGEV_NONE      0
#define SIGEV_SIGNAL    1
#define SIGEV_THREAD    2

union sigval {
    int   sival_int;
    void* sival_ptr;
};

struct sigevent {
    int          sigev_notify;
    int          sigev_signo;
    union sigval sigev_value;
    void       (*sigev_notify_function)(union sigval);
    void        *sigev_notify_attributes;
};

//  POSIX scheduling policy 
#define SCHED_FIFO      1
#define SCHED_RR        2
#define SCHED_OTHER     0

struct sched_param {
    int sched_priority;
};

#define errno (*takt_posix_errno_location())

#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENOMEM         12
#define EACCES         13
#define EFAULT         14
#define EBUSY          16
#define EEXIST         17
#define EINVAL         22
#define ENFILE         23
#define EMFILE         24
#define ENOSPC         28
#define ERANGE         34
#define EDEADLK        35
#define ENAMETOOLONG   36
#define ENOMSG         42
#define ETIME          62
#define ENOTSUP        95
#define EMSGSIZE       90
#define EOVERFLOW      75
#define EAGAIN         11
#define ETIMEDOUT      110
#define EWOULDBLOCK    EAGAIN

#ifdef __cplusplus
extern "C" {
#endif

//  errno / timing helpers 

/**
 * @brief	Return a pointer to the per-thread errno storage cell.
 *
 * The @c errno macro expands to @c *takt_posix_errno_location() so that each
 * thread has its own errno value.  Implemented in src/posix/errno.cpp.
 *
 * @return	Pointer to the calling thread's errno integer.
 */
int* takt_posix_errno_location(void);

/**
 * @brief	Return the current kernel tick count.
 *
 * Used by the POSIX timing helpers to compute absolute deadlines.
 * Wraps at UINT32_MAX.
 *
 * @return	Current tick count.
 */
uint32_t takt_posix_tick_count(void);

/**
 * @brief	Return the kernel tick rate in Hz.
 *
 * Used with takt_posix_tick_count() to convert between ticks and nanoseconds.
 *
 * @return	Tick frequency in Hz (e.g. 1000 for a 1 ms tick).
 */
uint32_t takt_posix_tick_hz(void);

/**
 * @brief	Convert an absolute POSIX timespec deadline to a relative tick count.
 *
 * Converts @p abs_timeout (expressed in seconds + nanoseconds since the epoch)
 * to a tick count relative to the current tick, suitable for passing to the
 * TaktOS blocking primitives.  Returns 0 if the deadline has already passed.
 * Returns UINT32_MAX - 1 if the delta overflows 32 bits.
 *
 * @param	abs_timeout : Absolute deadline as a POSIX timespec.
 * @param	tick_hz     : Kernel tick rate in Hz.
 * @return	Relative tick count (0 = already expired, UINT32_MAX = wait forever).
 */
static inline uint32_t takt_timespec_to_ticks(const struct timespec* abs_timeout,
                                              uint32_t tick_hz) {
    uint32_t now_ticks = takt_posix_tick_count();
    uint64_t abs_ticks = (uint64_t)abs_timeout->tv_sec * tick_hz
                       + (uint64_t)abs_timeout->tv_nsec / (1000000000ULL / tick_hz);

    if (abs_ticks <= now_ticks)
    {
        return 0u;
    }
    uint64_t delta = abs_ticks - now_ticks;
    return (delta > 0xFFFFFFFEu) ? 0xFFFFFFFFu
                                 : (uint32_t)delta;
}


#ifdef __cplusplus
}
#endif

#endif // __POSIX_TYPES_H__
