/**---------------------------------------------------------------------------
@file   gthr-taktos.h

@brief  Draft libstdc++ gthr port for TaktOS

This header is the first-draft bridge needed when building libstdc++ for a
TaktOS target with native thread support enabled.  It maps libstdc++'s gthr
abstraction to the existing TaktOS PSE51 pthread layer.

Important: this file does not make an already-built single-threaded embedded
libstdc++ expose std::thread.  It is intended to be used when configuring or
patching libstdc++ so that its gthr layer includes this file for TaktOS.

QM - outside cert boundary.
----------------------------------------------------------------------------*/
#ifndef _GLIBCXX_GTHREAD_TAKTOS_H
#define _GLIBCXX_GTHREAD_TAKTOS_H

#include "posix/pthread.h"
#include "posix/time.h"
#include "TaktKernel.h"
#include "TaktOSThread.h"

#define __GTHREADS 1
#define __GTHREADS_CXX0X 1

typedef pthread_t       __gthread_t;
typedef pthread_key_t   __gthread_key_t;
typedef pthread_once_t  __gthread_once_t;
typedef pthread_mutex_t __gthread_mutex_t;
typedef pthread_cond_t  __gthread_cond_t;
typedef struct timespec __gthread_time_t;

#define __GTHREAD_ONCE_INIT   PTHREAD_ONCE_INIT
#define __GTHREAD_MUTEX_INIT  PTHREAD_MUTEX_INITIALIZER
#define __GTHREAD_COND_INIT   PTHREAD_COND_INITIALIZER

struct __gthread_recursive_mutex_t {
    __gthread_mutex_t  mutex;
    __gthread_t        owner;
    unsigned int       depth;
    int                initialized;
};

#define __GTHREAD_RECURSIVE_MUTEX_INIT { PTHREAD_MUTEX_INITIALIZER, -1, 0u, 0 }

static inline int __gthread_active_p(void)
{
    return 1;
}

static inline int __gthread_create(__gthread_t *__threadid,
                                   void *(*__func)(void *), void *__args)
{
    return pthread_create(__threadid, nullptr, __func, __args);
}

static inline int __gthread_join(__gthread_t __threadid, void **__value_ptr)
{
    return pthread_join(__threadid, __value_ptr);
}

static inline int __gthread_detach(__gthread_t __threadid)
{
    return pthread_detach(__threadid);
}

static inline int __gthread_equal(__gthread_t __t1, __gthread_t __t2)
{
    return pthread_equal(__t1, __t2);
}

static inline __gthread_t __gthread_self(void)
{
    return pthread_self();
}

static inline int __gthread_yield(void)
{
    TaktOSThreadYield();
    return 0;
}

static inline int __gthread_once(__gthread_once_t *__once, void (*__func)(void))
{
    return pthread_once(__once, __func);
}

static inline int __gthread_key_create(__gthread_key_t *__key,
                                       void (*__dtor)(void *))
{
    return pthread_key_create(__key, __dtor);
}

static inline int __gthread_key_delete(__gthread_key_t __key)
{
    return pthread_key_delete(__key);
}

static inline void *__gthread_getspecific(__gthread_key_t __key)
{
    return pthread_getspecific(__key);
}

static inline int __gthread_setspecific(__gthread_key_t __key,
                                        const void *__ptr)
{
    return pthread_setspecific(__key, __ptr);
}

static inline void __gthread_mutex_init_function(__gthread_mutex_t *__mutex)
{
    (void)pthread_mutex_init(__mutex, nullptr);
}

static inline int __gthread_mutex_destroy(__gthread_mutex_t *__mutex)
{
    return pthread_mutex_destroy(__mutex);
}

static inline int __gthread_mutex_lock(__gthread_mutex_t *__mutex)
{
    return pthread_mutex_lock(__mutex);
}

static inline int __gthread_mutex_trylock(__gthread_mutex_t *__mutex)
{
    return pthread_mutex_trylock(__mutex);
}

static inline int __gthread_mutex_timedlock(__gthread_mutex_t *__mutex,
                                            const __gthread_time_t *__abs_timeout)
{
    return pthread_mutex_timedlock(__mutex, __abs_timeout);
}

static inline int __gthread_mutex_unlock(__gthread_mutex_t *__mutex)
{
    return pthread_mutex_unlock(__mutex);
}

static inline void __gthread_recursive_mutex_init_function(
    __gthread_recursive_mutex_t *__mutex)
{
    (void)pthread_mutex_init(&__mutex->mutex, nullptr);
    __mutex->owner = -1;
    __mutex->depth = 0u;
    __mutex->initialized = 1;
}

static inline int __gthread_recursive_mutex_destroy(
    __gthread_recursive_mutex_t *__mutex)
{
    return pthread_mutex_destroy(&__mutex->mutex);
}

static inline int __gthread_recursive_mutex_lock(
    __gthread_recursive_mutex_t *__mutex)
{
    if (__mutex->initialized == 0)
    {
        __gthread_recursive_mutex_init_function(__mutex);
    }

    __gthread_t self = pthread_self();
    uint32_t state = TaktOSEnterCritical();
    if (__mutex->depth != 0u && pthread_equal(__mutex->owner, self))
    {
        ++__mutex->depth;
        TaktOSExitCritical(state);
        return 0;
    }
    TaktOSExitCritical(state);

    int rc = pthread_mutex_lock(&__mutex->mutex);
    if (rc == 0)
    {
        state = TaktOSEnterCritical();
        __mutex->owner = self;
        __mutex->depth = 1u;
        TaktOSExitCritical(state);
    }
    return rc;
}

static inline int __gthread_recursive_mutex_trylock(
    __gthread_recursive_mutex_t *__mutex)
{
    if (__mutex->initialized == 0)
    {
        __gthread_recursive_mutex_init_function(__mutex);
    }

    __gthread_t self = pthread_self();
    uint32_t state = TaktOSEnterCritical();
    if (__mutex->depth != 0u && pthread_equal(__mutex->owner, self))
    {
        ++__mutex->depth;
        TaktOSExitCritical(state);
        return 0;
    }
    TaktOSExitCritical(state);

    int rc = pthread_mutex_trylock(&__mutex->mutex);
    if (rc == 0)
    {
        state = TaktOSEnterCritical();
        __mutex->owner = self;
        __mutex->depth = 1u;
        TaktOSExitCritical(state);
    }
    return rc;
}

static inline int __gthread_recursive_mutex_timedlock(
    __gthread_recursive_mutex_t *__mutex,
    const __gthread_time_t *__abs_timeout)
{
    if (__mutex->initialized == 0)
    {
        __gthread_recursive_mutex_init_function(__mutex);
    }

    __gthread_t self = pthread_self();
    uint32_t state = TaktOSEnterCritical();
    if (__mutex->depth != 0u && pthread_equal(__mutex->owner, self))
    {
        ++__mutex->depth;
        TaktOSExitCritical(state);
        return 0;
    }
    TaktOSExitCritical(state);

    int rc = pthread_mutex_timedlock(&__mutex->mutex, __abs_timeout);
    if (rc == 0)
    {
        state = TaktOSEnterCritical();
        __mutex->owner = self;
        __mutex->depth = 1u;
        TaktOSExitCritical(state);
    }
    return rc;
}

static inline int __gthread_recursive_mutex_unlock(
    __gthread_recursive_mutex_t *__mutex)
{
    uint32_t state = TaktOSEnterCritical();
    if (__mutex->depth == 0u || !pthread_equal(__mutex->owner, pthread_self()))
    {
        TaktOSExitCritical(state);
        return EPERM;
    }

    --__mutex->depth;
    if (__mutex->depth != 0u)
    {
        TaktOSExitCritical(state);
        return 0;
    }

    __mutex->owner = -1;
    TaktOSExitCritical(state);
    return pthread_mutex_unlock(&__mutex->mutex);
}

static inline void __gthread_cond_init_function(__gthread_cond_t *__cond)
{
    (void)pthread_cond_init(__cond, nullptr);
}

static inline int __gthread_cond_broadcast(__gthread_cond_t *__cond)
{
    return pthread_cond_broadcast(__cond);
}

static inline int __gthread_cond_signal(__gthread_cond_t *__cond)
{
    return pthread_cond_signal(__cond);
}

static inline int __gthread_cond_wait(__gthread_cond_t *__cond,
                                      __gthread_mutex_t *__mutex)
{
    return pthread_cond_wait(__cond, __mutex);
}

static inline int __gthread_cond_timedwait(__gthread_cond_t *__cond,
                                           __gthread_mutex_t *__mutex,
                                           const __gthread_time_t *__abs_timeout)
{
    return pthread_cond_timedwait(__cond, __mutex, __abs_timeout);
}

static inline int __gthread_cond_wait_recursive(__gthread_cond_t *__cond,
                                                __gthread_recursive_mutex_t *__mutex)
{
    if (__cond == nullptr || __mutex == nullptr)
    {
        return EINVAL;
    }

    uint32_t state = TaktOSEnterCritical();
    if (__mutex->depth == 0u || !pthread_equal(__mutex->owner, pthread_self()))
    {
        TaktOSExitCritical(state);
        return EPERM;
    }

    const unsigned int saved_depth = __mutex->depth;
    __mutex->owner = -1;
    __mutex->depth = 0u;
    TaktOSExitCritical(state);

    int rc = pthread_cond_wait(__cond, &__mutex->mutex);

    state = TaktOSEnterCritical();
    __mutex->owner = pthread_self();
    __mutex->depth = saved_depth;
    TaktOSExitCritical(state);

    return rc;
}

static inline int __gthread_cond_destroy(__gthread_cond_t *__cond)
{
    return pthread_cond_destroy(__cond);
}

#endif /* _GLIBCXX_GTHREAD_TAKTOS_H */
