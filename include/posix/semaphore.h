/**---------------------------------------------------------------------------
@file   semaphore.h

@brief  TaktOS PSE51  unnamed semaphores (sem_t)

Implements IEEE 1003.1-2017 PSE51 sem_init / sem_destroy / sem_wait /
sem_trywait / sem_timedwait / sem_post.

PSE51 requires only unnamed semaphores (sem_init / sem_destroy).
Named semaphores (sem_open) are PSE52+ and require a filesystem.
sem_t wraps TaktOSSem_t directly.

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
#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <stdint.h>

#include "posix_types.h"

// Opaque semaphore  index into TaktOS Sem pool.
typedef int sem_t;

#define SEM_FAILED  ((sem_t)-1)

#ifndef TAKT_POSIX_MAX_SEMS
#  define TAKT_POSIX_MAX_SEMS  16u
#endif
#ifndef TAKT_POSIX_SEM_MAX_VALUE
#  define TAKT_POSIX_SEM_MAX_VALUE  0x7FFFFFFFu
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief	Initialize an unnamed semaphore.
 *
 * @param	sem      : Semaphore to initialize.
 * @param	pshared  : Must be 0 (process-shared semaphores are not supported
 *                    on bare-metal).
 * @param	value    : Initial count (must be  TAKT_POSIX_SEM_MAX_VALUE).
 * @return	0 on success, EAGAIN if the pool is exhausted, EINVAL on bad arguments.
 */
int sem_init(sem_t* sem, int pshared, unsigned int value);

/**
 * @brief	Destroy an unnamed semaphore.
 *
 * @param	sem : Semaphore to destroy.  Must have no waiters.
 * @return	0 on success, EBUSY if threads are waiting, EINVAL if invalid.
 */
int sem_destroy(sem_t* sem);

/**
 * @brief	Decrement (wait on) a semaphore, blocking if the count is zero.
 *
 * @param	sem : Semaphore to decrement.
 * @return	0 on success, EINVAL if invalid.
 */
int sem_wait(sem_t* sem);

/**
 * @brief	Attempt to decrement a semaphore without blocking.
 *
 * @param	sem : Semaphore to try to decrement.
 * @return	0 on success, EAGAIN if the count is zero, EINVAL if invalid.
 */
int sem_trywait(sem_t* sem);

/**
 * @brief	Decrement a semaphore with an absolute timeout.
 *
 * Blocks until the count becomes non-zero or @p abs_timeout (CLOCK_MONOTONIC)
 * expires.
 *
 * @param	sem          : Semaphore to decrement.
 * @param	abs_timeout  : Absolute deadline as a timespec.
 * @return	0 on success, ETIMEDOUT on deadline expiry, EINVAL on bad arguments.
 */
int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout);

/**
 * @brief	Increment (post) a semaphore, unblocking one waiter if any.
 *
 * ISR-safe when the underlying TaktOSSem supports ISR calls.
 *
 * @param	sem : Semaphore to increment.
 * @return	0 on success, EINVAL if invalid, EOVERFLOW if the count would
 *          exceed TAKT_POSIX_SEM_MAX_VALUE.
 */
int sem_post(sem_t* sem);

/**
 * @brief	Read the current count of a semaphore without blocking.
 *
 * The returned value may be stale immediately; do not use it to make
 * scheduling decisions.
 *
 * @param	sem   : Semaphore to query.
 * @param	sval  : Receives the current count (negative values are not used
 *                 in this implementation; count is always  0).
 * @return	0 on success, EINVAL if invalid.
 */
int sem_getvalue(sem_t* sem, int* sval);

// Named semaphore stubs  return ENOTSUP (requires PSE52 filesystem)

/**
 * @brief	Open or create a named semaphore  not supported (PSE52+).
 *
 * @return	SEM_FAILED with errno set to ENOTSUP.
 */
sem_t* sem_open(const char* name, int oflag, ...);

/**
 * @brief	Close a named semaphore  not supported (PSE52+).
 *
 * @return	-1 with errno set to ENOTSUP.
 */
int sem_close(sem_t* sem);

/**
 * @brief	Remove a named semaphore  not supported (PSE52+).
 *
 * @return	-1 with errno set to ENOTSUP.
 */
int sem_unlink(const char* name);

#ifdef __cplusplus
}
#endif

#endif // __SEMAPHORE_H__
