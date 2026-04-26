/**---------------------------------------------------------------------------
@file   threads.cpp

@brief  TaktOS C11/C17/C23 <threads.h> implementation

This file maps ISO C thread functions to the existing TaktOS PSE51 pthread
layer.  It uses a small static trampoline pool because thrd_start_t returns
int while pthread start functions return void *.

QM - outside cert boundary.
----------------------------------------------------------------------------*/
#include "threads.h"

#include "TaktKernel.h"
#include "TaktOSThread.h"

#include <stdint.h>
#include <string.h>

#ifndef TAKT_CTHREAD_MAX_START_ARGS
#define TAKT_CTHREAD_MAX_START_ARGS TAKT_POSIX_MAX_THREADS
#endif

typedef struct {
    thrd_start_t start;
    void        *arg;
    int          in_use;
} TaktCThreadStartArg;

static TaktCThreadStartArg s_start_args[TAKT_CTHREAD_MAX_START_ARGS];

static TaktCThreadStartArg *alloc_start_arg(thrd_start_t start, void *arg)
{
    uint32_t state = TaktOSEnterCritical();
    for (uint32_t i = 0u; i < TAKT_CTHREAD_MAX_START_ARGS; ++i)
    {
        if (s_start_args[i].in_use == 0)
        {
            s_start_args[i].start  = start;
            s_start_args[i].arg    = arg;
            s_start_args[i].in_use = 1;
            TaktOSExitCritical(state);
            return &s_start_args[i];
        }
    }
    TaktOSExitCritical(state);
    return nullptr;
}

static void free_start_arg(TaktCThreadStartArg *slot)
{
    uint32_t state = TaktOSEnterCritical();
    slot->start  = nullptr;
    slot->arg    = nullptr;
    slot->in_use = 0;
    TaktOSExitCritical(state);
}

static void *thread_start_trampoline(void *arg)
{
    TaktCThreadStartArg *slot = static_cast<TaktCThreadStartArg *>(arg);
    thrd_start_t start = slot->start;
    void *user_arg = slot->arg;
    free_start_arg(slot);

    int result = start(user_arg);
    return reinterpret_cast<void *>(static_cast<intptr_t>(result));
}

static int pthread_status_to_thrd(int status)
{
    switch (status)
    {
        case 0:          return thrd_success;
        case ENOMEM:     return thrd_nomem;
        case EAGAIN:     return thrd_nomem;
        case EBUSY:      return thrd_busy;
        case ETIMEDOUT:  return thrd_timedout;
        default:         return thrd_error;
    }
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    if (thr == nullptr || func == nullptr)
    {
        return thrd_error;
    }

    TaktCThreadStartArg *slot = alloc_start_arg(func, arg);
    if (slot == nullptr)
    {
        return thrd_nomem;
    }

    int status = pthread_create(thr, nullptr, thread_start_trampoline, slot);
    if (status != 0)
    {
        free_start_arg(slot);
    }
    return pthread_status_to_thrd(status);
}

int thrd_equal(thrd_t lhs, thrd_t rhs)
{
    return pthread_equal(lhs, rhs);
}

thrd_t thrd_current(void)
{
    return pthread_self();
}

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    return nanosleep(duration, remaining);
}

void thrd_yield(void)
{
    TaktOSThreadYield();
}

[[noreturn]]
void thrd_exit(int res)
{
    pthread_exit(reinterpret_cast<void *>(static_cast<intptr_t>(res)));
    while (true) { }
}

int thrd_detach(thrd_t thr)
{
    return pthread_status_to_thrd(pthread_detach(thr));
}

int thrd_join(thrd_t thr, int *res)
{
    void *retval = nullptr;
    int status = pthread_join(thr, &retval);
    if (status != 0)
    {
        return pthread_status_to_thrd(status);
    }
    if (res != nullptr)
    {
        *res = static_cast<int>(reinterpret_cast<intptr_t>(retval));
    }
    return thrd_success;
}

int mtx_init(mtx_t *mtx, int type)
{
    if (mtx == nullptr)
    {
        return thrd_error;
    }
    if ((type & mtx_recursive) != 0)
    {
        /* The current PSE51 pthread layer does not implement recursive mutex. */
        return thrd_error;
    }

    memset(mtx, 0, sizeof(*mtx));
    int status = pthread_mutex_init(&mtx->native, nullptr);
    if (status != 0)
    {
        return pthread_status_to_thrd(status);
    }

    mtx->type = type;
    mtx->initialized = 1;
    return thrd_success;
}

void mtx_destroy(mtx_t *mtx)
{
    if (mtx == nullptr || mtx->initialized == 0)
    {
        return;
    }
    (void)pthread_mutex_destroy(&mtx->native);
    mtx->initialized = 0;
}

int mtx_lock(mtx_t *mtx)
{
    if (mtx == nullptr || mtx->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_mutex_lock(&mtx->native));
}

int mtx_trylock(mtx_t *mtx)
{
    if (mtx == nullptr || mtx->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_mutex_trylock(&mtx->native));
}

int mtx_timedlock(mtx_t *mtx, const struct timespec *time_point)
{
    if (mtx == nullptr || mtx->initialized == 0 || time_point == nullptr)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_mutex_timedlock(&mtx->native, time_point));
}

int mtx_unlock(mtx_t *mtx)
{
    if (mtx == nullptr || mtx->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_mutex_unlock(&mtx->native));
}

int cnd_init(cnd_t *cond)
{
    if (cond == nullptr)
    {
        return thrd_error;
    }

    memset(cond, 0, sizeof(*cond));
    int status = pthread_cond_init(&cond->native, nullptr);
    if (status != 0)
    {
        return pthread_status_to_thrd(status);
    }
    cond->initialized = 1;
    return thrd_success;
}

void cnd_destroy(cnd_t *cond)
{
    if (cond == nullptr || cond->initialized == 0)
    {
        return;
    }
    (void)pthread_cond_destroy(&cond->native);
    cond->initialized = 0;
}

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
    if (cond == nullptr || mtx == nullptr || cond->initialized == 0 || mtx->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_cond_wait(&cond->native, &mtx->native));
}

int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *time_point)
{
    if (cond == nullptr || mtx == nullptr || time_point == nullptr ||
        cond->initialized == 0 || mtx->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_cond_timedwait(&cond->native, &mtx->native, time_point));
}

int cnd_signal(cnd_t *cond)
{
    if (cond == nullptr || cond->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_cond_signal(&cond->native));
}

int cnd_broadcast(cnd_t *cond)
{
    if (cond == nullptr || cond->initialized == 0)
    {
        return thrd_error;
    }
    return pthread_status_to_thrd(pthread_cond_broadcast(&cond->native));
}

void call_once(once_flag *flag, void (*func)(void))
{
    if (flag == nullptr || func == nullptr)
    {
        return;
    }
    (void)pthread_once(flag, func);
}

int tss_create(tss_t *key, tss_dtor_t destructor)
{
    return pthread_status_to_thrd(pthread_key_create(key, destructor));
}

void tss_delete(tss_t key)
{
    (void)pthread_key_delete(key);
}

void *tss_get(tss_t key)
{
    return pthread_getspecific(key);
}

int tss_set(tss_t key, void *val)
{
    return pthread_status_to_thrd(pthread_setspecific(key, val));
}
