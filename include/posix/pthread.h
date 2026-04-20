/**---------------------------------------------------------------------------
@file   pthread.h

@brief  TaktOS PSE51  threads, mutexes, and condition variables

Covers the IEEE 1003.1-2017 PSE51 thread API:
  pthread_create / join / detach / exit / self / equal / once
  pthread_attr_t  (stack, priority, detach state)
  pthread_mutex_t (normal + priority-inheritance via TaktOS Mutex)
  pthread_cond_t  (condition variable via Sem + Mutex)

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
#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include <stdint.h>

#include "posix_types.h"

//  Pool size 
#ifndef TAKT_POSIX_MAX_THREADS
#  define TAKT_POSIX_MAX_THREADS   16u
#endif
#ifndef TAKT_POSIX_MAX_MUTEXES
#  define TAKT_POSIX_MAX_MUTEXES   16u
#endif
#ifndef TAKT_POSIX_MAX_CONDS
#  define TAKT_POSIX_MAX_CONDS     8u
#endif
#ifndef TAKT_POSIX_DEFAULT_STACK
#  define TAKT_POSIX_DEFAULT_STACK 1024u
#endif

//  Opaque handle types 
typedef int  pthread_t;          // index into thread pool (-1 = invalid)
typedef int  pthread_mutex_t;    // index into mutex pool  (-1 = invalid)
typedef int  pthread_cond_t;     // index into cond pool   (-1 = invalid)
typedef int  pthread_key_t;      // single supported TLS key (key value 0)
typedef int  pthread_once_t;     // 0 = uninitialised, 1 = in progress, 2 = done

#define PTHREAD_ONCE_INIT        0
#define PTHREAD_MUTEX_INITIALIZER  (-2)  // static init sentinel
#define PTHREAD_COND_INITIALIZER   (-2)

//  Detach state 
#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1

//  Mutex type 
#define PTHREAD_MUTEX_DEFAULT      0
#define PTHREAD_MUTEX_NORMAL       0
#define PTHREAD_MUTEX_ERRORCHECK   1   // mapped to NORMAL (single-process)
#define PTHREAD_MUTEX_RECURSIVE    2   // not supported  ENOTSUP

//  Mutex protocol (priority inheritance) 
#define PTHREAD_PRIO_NONE          0
#define PTHREAD_PRIO_INHERIT       1   // default  maps to TaktOS Mutex
#define PTHREAD_PRIO_PROTECT       2   // ceiling  not in v1.0

//  Thread attribute 
typedef struct {
    uint32_t        stack_size;
    void*           stack_addr;      // NULL = use pool
    int             detach_state;    // JOINABLE | DETACHED
    struct sched_param schedparam;
    int             sched_policy;    // SCHED_FIFO | SCHED_RR
    int             inheritsched;    // PTHREAD_INHERIT_SCHED (ignored)
    int             _valid;          // internal: 0 = uninitialised
} pthread_attr_t;

#define PTHREAD_INHERIT_SCHED    0
#define PTHREAD_EXPLICIT_SCHED   1

//  Mutex attribute 
typedef struct {
    int  type;       // PTHREAD_MUTEX_DEFAULT etc.
    int  protocol;   // PTHREAD_PRIO_INHERIT etc.
    int  _valid;
} pthread_mutexattr_t;

//  Condition variable attribute (nothing configurable in PSE51) 
typedef struct {
	int _valid;
} pthread_condattr_t;

#ifdef __cplusplus
extern "C" {
#endif

// 
// Thread API
// 

/**
 * @brief	Create a new thread.
 *
 * Allocates a thread from the static pool and starts it executing @p start.
 * If @p attr is NULL the thread is created joinable with the default stack
 * size (TAKT_POSIX_DEFAULT_STACK) and priority TAKTOS_PRIORITY_NORMAL.
 *
 * @param	thread  : Receives the new thread ID on success.
 * @param	attr    : Thread attributes, or NULL for defaults.
 * @param	start   : Thread entry function; must not return (call pthread_exit).
 * @param	arg     : Opaque argument passed to @p start.
 * @return	0 on success, EAGAIN if the thread pool is exhausted, EINVAL on
 *          bad attributes.
 */
int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                    void* (*start)(void*), void* arg);

/**
 * @brief	Wait for a thread to terminate.
 *
 * Blocks the caller until the thread identified by @p thread exits.
 * If @p retval is non-NULL, the value passed to pthread_exit is stored there.
 * Joining a detached thread returns EINVAL.
 *
 * @param	thread  : Thread ID to join.
 * @param	retval  : Receives the thread's exit value, or NULL to discard.
 * @return	0 on success, ESRCH if not found, EINVAL if detached or already joined.
 */
int pthread_join(pthread_t thread, void** retval);

/**
 * @brief	Detach a thread so its resources are released automatically on exit.
 *
 * A detached thread cannot be joined.  Calling pthread_detach on an already
 * detached thread returns EINVAL.
 *
 * @param	thread : Thread ID to detach.
 * @return	0 on success, ESRCH if not found, EINVAL if already detached.
 */
int pthread_detach(pthread_t thread);

/**
 * @brief	Terminate the calling thread.
 *
 * Stores @p retval for a future pthread_join caller.  Does not return.
 * Equivalent to returning from the thread start function.
 *
 * @param	retval : Exit value made available to pthread_join.
 */
void pthread_exit(void* retval)  __attribute__((noreturn));

/**
 * @brief	Return the ID of the calling thread.
 *
 * @return	Thread ID of the caller.
 */
pthread_t pthread_self(void);

/**
 * @brief	Test whether two thread IDs refer to the same thread.
 *
 * @param	t1 : First thread ID.
 * @param	t2 : Second thread ID.
 * @return	Non-zero if equal, 0 if different.
 */
int pthread_equal(pthread_t t1, pthread_t t2);

/**
 * @brief	Ensure a one-time initializer runs exactly once.
 *
 * The first call with a given @p once object runs @p init; all subsequent
 * calls are no-ops.  Thread-safe.
 *
 * @param	once : Control object (initialize to PTHREAD_ONCE_INIT).
 * @param	init : Initialization function to call at most once.
 * @return	0 on success.
 */
int pthread_once(pthread_once_t* once, void (*init)(void));

//  Thread attribute API 

/**
 * @brief	Initialize a thread attribute object with default values.
 *
 * @param	attr : Attribute object to initialize.
 * @return	0 on success, EINVAL if @p attr is null.
 */
int pthread_attr_init(pthread_attr_t* attr);

/**
 * @brief	Destroy a thread attribute object.
 *
 * @param	attr : Attribute object to destroy.
 * @return	0 on success, EINVAL if @p attr is null or uninitialized.
 */
int pthread_attr_destroy(pthread_attr_t* attr);

/**
 * @brief	Set the stack size in a thread attribute object.
 *
 * @param	attr       : Thread attribute object.
 * @param	stacksize  : Desired stack size in bytes (minimum implementation-defined).
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize);

/**
 * @brief	Get the stack size from a thread attribute object.
 *
 * @param	attr       : Thread attribute object (const).
 * @param	stacksize  : Receives the stack size in bytes.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_getstacksize(const pthread_attr_t* attr, size_t* stacksize);

/**
 * @brief	Set a pre-allocated stack address in a thread attribute object.
 *
 * @param	attr       : Thread attribute object.
 * @param	stackaddr  : Base address of the caller-provided stack buffer, or NULL
 *                      to use a pool-allocated stack.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_setstackaddr(pthread_attr_t* attr, void* stackaddr);

/**
 * @brief	Get the stack address from a thread attribute object.
 *
 * @param	attr       : Thread attribute object (const).
 * @param	stackaddr  : Receives the stack address (NULL if pool-allocated).
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_getstackaddr(const pthread_attr_t* attr, void** stackaddr);

/**
 * @brief	Set the detach state in a thread attribute object.
 *
 * @param	attr         : Thread attribute object.
 * @param	detachstate  : PTHREAD_CREATE_JOINABLE or PTHREAD_CREATE_DETACHED.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate);

/**
 * @brief	Get the detach state from a thread attribute object.
 *
 * @param	attr         : Thread attribute object (const).
 * @param	detachstate  : Receives PTHREAD_CREATE_JOINABLE or PTHREAD_CREATE_DETACHED.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_getdetachstate(const pthread_attr_t* attr, int* detachstate);

/**
 * @brief	Set the scheduling parameters in a thread attribute object.
 *
 * @param	attr  : Thread attribute object.
 * @param	p     : Scheduling parameters (sched_priority maps to TaktOS priority).
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_setschedparam(pthread_attr_t* attr, const struct sched_param* p);

/**
 * @brief	Get the scheduling parameters from a thread attribute object.
 *
 * @param	attr  : Thread attribute object (const).
 * @param	p     : Receives the scheduling parameters.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_getschedparam(const pthread_attr_t* attr, struct sched_param* p);

/**
 * @brief	Set the scheduling policy in a thread attribute object.
 *
 * @param	attr    : Thread attribute object.
 * @param	policy  : SCHED_FIFO or SCHED_RR (both map to TaktOS priority scheduling).
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_setschedpolicy(pthread_attr_t* attr, int policy);

/**
 * @brief	Get the scheduling policy from a thread attribute object.
 *
 * @param	attr    : Thread attribute object (const).
 * @param	policy  : Receives the scheduling policy.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_getschedpolicy(const pthread_attr_t* attr, int* policy);

/**
 * @brief	Set the inherit-scheduler flag in a thread attribute object.
 *
 * PSE51 requires this function; the value is accepted but has no effect 
 * TaktOS always uses the priority from the attribute object.
 *
 * @param	attr     : Thread attribute object.
 * @param	inherit  : PTHREAD_INHERIT_SCHED or PTHREAD_EXPLICIT_SCHED.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_setinheritsched(pthread_attr_t* attr, int inherit);

/**
 * @brief	Get the inherit-scheduler flag from a thread attribute object.
 *
 * @param	attr     : Thread attribute object (const).
 * @param	inherit  : Receives PTHREAD_INHERIT_SCHED or PTHREAD_EXPLICIT_SCHED.
 * @return	0 on success, EINVAL on bad arguments.
 */
int pthread_attr_getinheritsched(const pthread_attr_t* attr, int* inherit);

// 
// Mutex API
// 

/**
 * @brief	Initialize a mutex attribute object with default values.
 *
 * @param	attr : Mutex attribute object to initialize.
 * @return	0 on success, EINVAL if @p attr is null.
 */
int  pthread_mutexattr_init        (pthread_mutexattr_t* attr);

/**
 * @brief	Destroy a mutex attribute object.
 *
 * @param	attr : Mutex attribute object to destroy.
 * @return	0 on success, EINVAL if @p attr is null or uninitialized.
 */
int  pthread_mutexattr_destroy     (pthread_mutexattr_t* attr);

/**
 * @brief	Set the mutex type in a mutex attribute object.
 *
 * @param	attr  : Mutex attribute object.
 * @param	type  : PTHREAD_MUTEX_DEFAULT, PTHREAD_MUTEX_NORMAL, or
 *                 PTHREAD_MUTEX_ERRORCHECK.  PTHREAD_MUTEX_RECURSIVE
 *                 returns ENOTSUP.
 * @return	0 on success, EINVAL on bad arguments, ENOTSUP for recursive type.
 */
int  pthread_mutexattr_settype     (pthread_mutexattr_t* attr, int type);

/**
 * @brief	Get the mutex type from a mutex attribute object.
 *
 * @param	attr  : Mutex attribute object (const).
 * @param	type  : Receives the mutex type.
 * @return	0 on success, EINVAL on bad arguments.
 */
int  pthread_mutexattr_gettype     (const pthread_mutexattr_t* attr, int* type);

/**
 * @brief	Set the priority-inheritance protocol in a mutex attribute object.
 *
 * @param	attr      : Mutex attribute object.
 * @param	protocol  : PTHREAD_PRIO_NONE or PTHREAD_PRIO_INHERIT.
 *                     PTHREAD_PRIO_PROTECT returns ENOTSUP (v1.0).
 * @return	0 on success, EINVAL on bad arguments, ENOTSUP for ceiling protocol.
 */
int  pthread_mutexattr_setprotocol (pthread_mutexattr_t* attr, int protocol);

/**
 * @brief	Get the priority-inheritance protocol from a mutex attribute object.
 *
 * @param	attr      : Mutex attribute object (const).
 * @param	protocol  : Receives the protocol value.
 * @return	0 on success, EINVAL on bad arguments.
 */
int  pthread_mutexattr_getprotocol (const pthread_mutexattr_t* attr, int* protocol);

/**
 * @brief	Initialize a mutex.
 *
 * @param	mutex  : Mutex to initialize.
 * @param	attr   : Mutex attributes, or NULL for defaults.
 * @return	0 on success, EAGAIN if the mutex pool is exhausted, EINVAL on bad arguments.
 */
int  pthread_mutex_init    (pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);

/**
 * @brief	Destroy a mutex and return it to the pool.
 *
 * @param	mutex : Mutex to destroy.  Must not be locked.
 * @return	0 on success, EBUSY if still locked, EINVAL if invalid.
 */
int  pthread_mutex_destroy (pthread_mutex_t* mutex);

/**
 * @brief	Lock a mutex, blocking until it is available.
 *
 * @param	mutex : Mutex to lock.
 * @return	0 on success, EINVAL if invalid, EDEADLK if the caller already holds it.
 */
int  pthread_mutex_lock    (pthread_mutex_t* mutex);

/**
 * @brief	Attempt to lock a mutex without blocking.
 *
 * @param	mutex : Mutex to try to lock.
 * @return	0 on success, EBUSY if already locked, EINVAL if invalid.
 */
int  pthread_mutex_trylock (pthread_mutex_t* mutex);

/**
 * @brief	Lock a mutex with an absolute timeout.
 *
 * Blocks until the mutex is acquired or @p abs_timeout (CLOCK_MONOTONIC)
 * is reached.
 *
 * @param	mutex        : Mutex to lock.
 * @param	abs_timeout  : Absolute deadline as a timespec.
 * @return	0 on success, ETIMEDOUT on deadline expiry, EINVAL on bad arguments.
 */
int  pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec* abs_timeout);

/**
 * @brief	Unlock a mutex.
 *
 * Must be called by the owning thread.
 *
 * @param	mutex : Mutex to unlock.
 * @return	0 on success, EPERM if the caller is not the owner, EINVAL if invalid.
 */
int  pthread_mutex_unlock  (pthread_mutex_t* mutex);

// 
// Condition variable API
// 

/**
 * @brief	Initialize a condition variable attribute object.
 *
 * @param	attr : Condition variable attribute object to initialize.
 * @return	0 on success, EINVAL if @p attr is null.
 */
int  pthread_condattr_init    (pthread_condattr_t* attr);

/**
 * @brief	Destroy a condition variable attribute object.
 *
 * @param	attr : Condition variable attribute object to destroy.
 * @return	0 on success, EINVAL if @p attr is null or uninitialized.
 */
int  pthread_condattr_destroy (pthread_condattr_t* attr);

/**
 * @brief	Initialize a condition variable.
 *
 * @param	cond  : Condition variable to initialize.
 * @param	attr  : Attributes, or NULL for defaults.
 * @return	0 on success, EAGAIN if the pool is exhausted, EINVAL on bad arguments.
 */
int  pthread_cond_init      (pthread_cond_t* cond, const pthread_condattr_t* attr);

/**
 * @brief	Destroy a condition variable.
 *
 * @param	cond : Condition variable to destroy.  Must have no waiters.
 * @return	0 on success, EBUSY if threads are waiting, EINVAL if invalid.
 */
int  pthread_cond_destroy   (pthread_cond_t* cond);

/**
 * @brief	Atomically release @p mutex and block on @p cond.
 *
 * The mutex is unlocked and the caller blocks until pthread_cond_signal or
 * pthread_cond_broadcast is called.  The mutex is re-locked before returning.
 *
 * @param	cond   : Condition variable to wait on.
 * @param	mutex  : Mutex held by the caller (unlocked during the wait).
 * @return	0 on success, EINVAL on bad arguments.
 */
int  pthread_cond_wait      (pthread_cond_t* cond, pthread_mutex_t* mutex);

/**
 * @brief	Atomically release @p mutex and block on @p cond with a timeout.
 *
 * Like pthread_cond_wait but returns ETIMEDOUT if @p abs_timeout
 * (CLOCK_MONOTONIC) expires before the condition is signalled.
 *
 * @param	cond         : Condition variable to wait on.
 * @param	mutex        : Mutex held by the caller.
 * @param	abs_timeout  : Absolute deadline as a timespec.
 * @return	0 on success, ETIMEDOUT on deadline expiry, EINVAL on bad arguments.
 */
int  pthread_cond_timedwait (pthread_cond_t* cond, pthread_mutex_t* mutex,
                              const struct timespec* abs_timeout);

/**
 * @brief	Unblock at least one thread waiting on @p cond.
 *
 * @param	cond : Condition variable to signal.
 * @return	0 on success, EINVAL if invalid.
 */
int  pthread_cond_signal    (pthread_cond_t* cond);

/**
 * @brief	Unblock all threads waiting on @p cond.
 *
 * @param	cond : Condition variable to broadcast.
 * @return	0 on success, EINVAL if invalid.
 */
int  pthread_cond_broadcast (pthread_cond_t* cond);

//  Thread-local storage (single supported key, per-thread value) 

/**
 * @brief	Create a thread-local storage key.
 *
 * Only one key (value 0) is supported in this implementation.
 *
 * @param	key         : Receives the new key value.
 * @param	destructor  : Called with the thread's value on thread exit (may be NULL).
 * @return	0 on success, EAGAIN if the key pool is exhausted.
 */
int  pthread_key_create  (pthread_key_t* key, void (*destructor)(void*));

/**
 * @brief	Delete a thread-local storage key.
 *
 * @param	key : Key to delete.
 * @return	0 on success, EINVAL if the key is invalid.
 */
int  pthread_key_delete  (pthread_key_t key);

/**
 * @brief	Return the thread-specific value for @p key in the calling thread.
 *
 * @param	key : TLS key.
 * @return	The stored value, or NULL if no value has been set.
 */
void* pthread_getspecific(pthread_key_t key);

/**
 * @brief	Set the thread-specific value for @p key in the calling thread.
 *
 * @param	key    : TLS key.
 * @param	value  : Value to associate with the key in the calling thread.
 * @return	0 on success, EINVAL if the key is invalid.
 */
int  pthread_setspecific (pthread_key_t key, const void* value);

//  Scheduling 

/**
 * @brief	Set the scheduling policy and parameters for a thread.
 *
 * Maps @p policy and @p p->sched_priority to a TaktOS priority level.
 *
 * @param	thread  : Target thread ID.
 * @param	policy  : SCHED_FIFO or SCHED_RR.
 * @param	p       : Scheduling parameters containing the new priority.
 * @return	0 on success, ESRCH if not found, EINVAL on bad arguments.
 */
int  pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* p);

/**
 * @brief	Get the scheduling policy and parameters of a thread.
 *
 * @param	thread  : Target thread ID.
 * @param	policy  : Receives the current scheduling policy.
 * @param	p       : Receives the current scheduling parameters.
 * @return	0 on success, ESRCH if not found, EINVAL on bad arguments.
 */
int  pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* p);

/**
 * @brief	Return the maximum scheduling priority for @p policy.
 *
 * @param	policy : SCHED_FIFO or SCHED_RR.
 * @return	Maximum priority value, or -1 on invalid policy.
 */
int  sched_get_priority_max(int policy);

/**
 * @brief	Return the minimum scheduling priority for @p policy.
 *
 * @param	policy : SCHED_FIFO or SCHED_RR.
 * @return	Minimum priority value, or -1 on invalid policy.
 */
int  sched_get_priority_min(int policy);

#ifdef __cplusplus
}
#endif

#endif // __PTHREAD_H__
