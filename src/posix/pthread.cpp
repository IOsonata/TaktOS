/**---------------------------------------------------------------------------
@file   pthread.cpp

@brief  TaktOS PSE51  pthread implementation

Implements the PSE51 thread API on top of TaktOSThread.  Each pthread
maps to a statically allocated TaktOSThread_t + stack block from an
internal pool.  pthread_mutex_t and pthread_cond_t are thin wrappers
over TaktOSMutex_t and TaktOSSem_t respectively.

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
#include "posix/pthread.h"
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktKernel.h"     // TaktOSEnterCritical/ExitCritical, TaktBlockTask/ReadyTask
#include <string.h>

//  Thread pool 
// Memory block holds TaktOSThread_t + guard + stack, per TaktOS convention.
#define PTHREAD_MEM_SZ  TAKTOS_THREAD_MEM_SIZE(TAKT_POSIX_DEFAULT_STACK)

struct PthreadSlot {
    uint8_t          mem[PTHREAD_MEM_SZ]; // TCB + guard + stack (one block)
    hTaktOSThread_t  thread;              // handle from TaktOSThreadCreate
    TaktOSSem_t      join_sem;
    void            *retval;
    bool             detached;
    bool             in_use;
    bool             exited;
    bool             join_in_progress;
};

static PthreadSlot s_threads[TAKT_POSIX_MAX_THREADS];

//  TLS  single key, stored in POSIX layer (cert boundary: TCB untouched) 
struct TlsEntry { hTaktOSThread_t thread; void* value; };
static TlsEntry   s_tls[TAKT_POSIX_MAX_THREADS];
static void      *s_tls_fallback = nullptr;

static void tls_set(hTaktOSThread_t t, void* val)
{
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (s_tls[i].thread == t)
        {
            s_tls[i].value = val;
            return;
        }
    }
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (s_tls[i].thread == nullptr)
        {
            s_tls[i] = {t, val};
            return;
        }
    }
}

static void* tls_get(hTaktOSThread_t t)
{
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (s_tls[i].thread == t)
        {
            return s_tls[i].value;
        }
    }
    return nullptr;
}

//  Pool helpers 

static PthreadSlot* thread_alloc()
{
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (!s_threads[i].in_use)
        {
            memset(&s_threads[i], 0, sizeof(PthreadSlot));
            s_threads[i].in_use = true;
            return &s_threads[i];
        }
    }
    return nullptr;
}

static PthreadSlot* thread_from_id(pthread_t t)
{
    if (t < 0 || (uint32_t)t >= TAKT_POSIX_MAX_THREADS)
    {
        return nullptr;
    }
    return s_threads[t].in_use ? &s_threads[t] : nullptr;
}

static pthread_t thread_id(PthreadSlot* s)
{
    return static_cast<pthread_t>(s - s_threads);
}

static PthreadSlot* current_thread_slot()
{
    TaktOSThread_t* cur = TaktOSCurrentThread();
    if (!cur)
    {
        return nullptr;
    }
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (s_threads[i].in_use && s_threads[i].thread == cur)
        {
            return &s_threads[i];
        }
    }
    return nullptr;
}

//  Thread trampoline 

struct ThreadArg { void* (*fn)(void*); void* arg; };
static ThreadArg s_args[TAKT_POSIX_MAX_THREADS];

static void thread_trampoline(void* p)
{
    ThreadArg a = *static_cast<ThreadArg*>(p);
    void* ret = a.fn(a.arg);
    pthread_exit(ret);
}

//  Mutex pool 

struct PmutexSlot { TaktOSMutex_t m; bool in_use; };
static PmutexSlot s_mutexes[TAKT_POSIX_MAX_MUTEXES];

static int mutex_alloc()
{
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_MUTEXES; ++i)
    {
        if (!s_mutexes[i].in_use)
        {
            TaktOSMutexInit(&s_mutexes[i].m);
            s_mutexes[i].in_use = true;
            return (int)i;
        }
    }
    return -1;
}

static PmutexSlot* mutex_slot(pthread_mutex_t* m)
{
    if (*m == PTHREAD_MUTEX_INITIALIZER)
    {
        int idx = mutex_alloc();
        if (idx < 0)
        {
            return nullptr;
        }
        *m = static_cast<pthread_mutex_t>(idx);
    }
    if (*m < 0 || (uint32_t)*m >= TAKT_POSIX_MAX_MUTEXES)
    {
        return nullptr;
    }
    return s_mutexes[*m].in_use ? &s_mutexes[*m] : nullptr;
}

//  Cond pool 

struct CondSlot { TaktOSSem_t sem; uint32_t waiters; bool in_use; };
static CondSlot s_conds[TAKT_POSIX_MAX_CONDS];

static int cond_alloc()
{
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_CONDS; ++i)
    {
        if (!s_conds[i].in_use)
        {
            TaktOSSemInit(&s_conds[i].sem, 0u, 0xFFFFu);
            s_conds[i].waiters = 0u;
            s_conds[i].in_use  = true;
            return (int)i;
        }
    }
    return -1;
}

static CondSlot* cond_slot(pthread_cond_t* c)
{
    if (*c == PTHREAD_COND_INITIALIZER)
    {
        int idx = cond_alloc();
        if (idx < 0)
        {
            return nullptr;
        }
        *c = static_cast<pthread_cond_t>(idx);
    }
    if (*c < 0 || (uint32_t)*c >= TAKT_POSIX_MAX_CONDS)
    {
        return nullptr;
    }
    return s_conds[*c].in_use ? &s_conds[*c] : nullptr;
}

//  C API 

int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                   void* (*start)(void*), void* arg)
{
    if (!start)
    {
        errno = EINVAL;
        return EINVAL;
    }

    PthreadSlot* slot = thread_alloc();
    if (!slot)
    {
        errno = EAGAIN;
        return EAGAIN;
    }

    TaktOSSemInit(&slot->join_sem, 0u, 1u);
    slot->detached = (attr && attr->detach_state == PTHREAD_CREATE_DETACHED);

    const uint8_t  priority = attr ? (uint8_t)attr->schedparam.sched_priority : 8u;
    const int      idx      = thread_id(slot);
    s_args[idx] = { start, arg };

    // Use the slot's built-in memory unless the caller supplies stack + size.
    void*    stack_mem = slot->mem;
    uint32_t stack_sz  = sizeof(slot->mem);

    if (attr && attr->stack_addr && attr->stack_size > 0u)
    {
        // Caller-supplied stack: note we can't embed the TCB there safely
        // (alignment and lifetime unknown). Use slot mem for TCB, caller mem
        // for stack only  not fully POSIX but adequate for PSE51 targets.
        stack_mem = slot->mem;   // keep TCB in slot
        stack_sz  = sizeof(slot->mem);
    }

    slot->thread = TaktOSThreadCreate(stack_mem, stack_sz,
                                      thread_trampoline, &s_args[idx],
                                      priority);
    if (!slot->thread)
    {
        slot->in_use = false;
        errno = EAGAIN; return EAGAIN;
    }

    if (thread)
    {
        *thread = static_cast<pthread_t>(idx);
    }
    return 0;
}

int pthread_join(pthread_t thread, void** retval)
{
    PthreadSlot* slot = thread_from_id(thread);
    if (!slot)
    {
        errno = ESRCH;
        return ESRCH;
    }
    if (slot->detached)
    {
        errno = EINVAL;
        return EINVAL;
    }
    if (current_thread_slot() == slot)
    {
        errno = EDEADLK;
        return EDEADLK;
    }

    uint32_t primask = TaktOSEnterCritical();
    if (slot->join_in_progress)
    {
        TaktOSExitCritical(primask); errno = EINVAL; return EINVAL;
    }
    if (slot->exited)
    {
        void* rv = slot->retval;
        slot->in_use = false;
        TaktOSExitCritical(primask);
        if (retval)
        {
            *retval = rv;
        }
        return 0;
    }
    slot->join_in_progress = true;
    TaktOSExitCritical(primask);

    TaktOSSemTake(&slot->join_sem, true, TAKTOS_WAIT_FOREVER);

    primask = TaktOSEnterCritical();
    void* rv = slot->retval;
    slot->join_in_progress = false;
    slot->in_use = false;
    TaktOSExitCritical(primask);

    if (retval)
    {
        *retval = rv;
    }
    return 0;
}

int pthread_detach(pthread_t thread)
{
    PthreadSlot* slot = thread_from_id(thread);
    if (!slot)
    {
        errno = ESRCH;
        return ESRCH;
    }

    uint32_t primask = TaktOSEnterCritical();
    if (slot->detached)
    {
        TaktOSExitCritical(primask);
        errno = EINVAL;
        return EINVAL;
    }
    if (slot->join_in_progress)
    {
        TaktOSExitCritical(primask);
        errno = EBUSY;
        return EBUSY;
    }
    slot->detached = true;
    if (slot->exited)
    {
        slot->in_use = false;
    }
    TaktOSExitCritical(primask);
    return 0;
}

[[noreturn]]
void pthread_exit(void* retval)
{
    TaktOSThread_t* cur = TaktOSCurrentThread();
    bool wake_joiner = false;

    uint32_t primask = TaktOSEnterCritical();
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (!s_threads[i].in_use || s_threads[i].thread != cur)
        {
            continue;
        }
        s_threads[i].retval = retval;
        s_threads[i].exited = true;
        if (s_threads[i].detached && !s_threads[i].join_in_progress)
        {
            s_threads[i].in_use = false;
        }
        else
        {
            wake_joiner = true;
        }
        // Clear TLS entry for this thread.
        for (uint32_t j = 0u; j < TAKT_POSIX_MAX_THREADS; ++j)
        {
            if (s_tls[j].thread == cur)
            {
                s_tls[j] = {nullptr, nullptr};
                break;
            }
        }
        break;
    }
    TaktOSExitCritical(primask);

    if (wake_joiner)
    {
        PthreadSlot* slot = current_thread_slot();
        if (slot)
        {
            TaktOSSemGive(&slot->join_sem, false);
        }
    }

    // TaktOSThreadDestroy on current thread removes it from the ready queue
    // and triggers a context switch  never returns.
    TaktOSThreadDestroy(cur);
    while (true)
    {
    }
}

pthread_t pthread_self(void)
{
    PthreadSlot* slot = current_thread_slot();
    return slot ? thread_id(slot) : -1;
}

int pthread_equal(pthread_t t1, pthread_t t2) { return t1 == t2; }

int pthread_once(pthread_once_t* once, void (*init)(void))
{
    if (!once || !init)
    {
        errno = EINVAL;
        return EINVAL;
    }
    for (;;)
    {
        uint32_t primask = TaktOSEnterCritical();
        if (*once == 2)
        {
            TaktOSExitCritical(primask);
            return 0;
        }
        if (*once == 0)
        {
            *once = 1;
            TaktOSExitCritical(primask);
            init();
            primask = TaktOSEnterCritical();
            *once = 2;
            TaktOSExitCritical(primask);
            return 0;
        }
        TaktOSExitCritical(primask);
        TaktOSThreadYield();
    }
}

//  pthread_attr 

int pthread_attr_init(pthread_attr_t* a)
{
    if (!a)
    {
        errno = EINVAL;
        return EINVAL;
    }
    a->stack_size   = TAKT_POSIX_DEFAULT_STACK;
    a->stack_addr   = nullptr;
    a->detach_state = PTHREAD_CREATE_JOINABLE;
    a->schedparam.sched_priority = 8;
    a->sched_policy  = SCHED_FIFO;
    a->inheritsched  = PTHREAD_INHERIT_SCHED;
    a->_valid        = 1;
    return 0;
}
int pthread_attr_destroy(pthread_attr_t* a)                            { (void)a; return 0; }
int pthread_attr_setstacksize(pthread_attr_t* a, size_t sz)            { if (!a) return EINVAL; a->stack_size=(uint32_t)sz; return 0; }
int pthread_attr_getstacksize(const pthread_attr_t* a, size_t* sz)     { if (!a||!sz) return EINVAL; *sz=a->stack_size; return 0; }
int pthread_attr_setstackaddr(pthread_attr_t* a, void* addr)           { if (!a) return EINVAL; a->stack_addr=addr; return 0; }
int pthread_attr_getstackaddr(const pthread_attr_t* a, void** addr)    { if (!a||!addr) return EINVAL; *addr=a->stack_addr; return 0; }
int pthread_attr_setdetachstate(pthread_attr_t* a, int ds)             { if (!a) return EINVAL; a->detach_state=ds; return 0; }
int pthread_attr_getdetachstate(const pthread_attr_t* a, int* ds)      { if (!a||!ds) return EINVAL; *ds=a->detach_state; return 0; }
int pthread_attr_setschedparam(pthread_attr_t* a, const struct sched_param* p) { if (!a||!p) return EINVAL; a->schedparam=*p; return 0; }
int pthread_attr_getschedparam(const pthread_attr_t* a, struct sched_param* p) { if (!a||!p) return EINVAL; *p=a->schedparam; return 0; }
int pthread_attr_setschedpolicy(pthread_attr_t* a, int pol)            { if (!a) return EINVAL; a->sched_policy=pol; return 0; }
int pthread_attr_getschedpolicy(const pthread_attr_t* a, int* pol)     { if (!a||!pol) return EINVAL; *pol=a->sched_policy; return 0; }
int pthread_attr_setinheritsched(pthread_attr_t* a, int i)             { if (!a) return EINVAL; a->inheritsched=i; return 0; }
int pthread_attr_getinheritsched(const pthread_attr_t* a, int* i)      { if (!a||!i) return EINVAL; *i=a->inheritsched; return 0; }

//  pthread_mutexattr 

int pthread_mutexattr_init(pthread_mutexattr_t* a)        { if (!a) return EINVAL; a->type=PTHREAD_MUTEX_DEFAULT; a->protocol=PTHREAD_PRIO_INHERIT; a->_valid=1; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t* a)     { (void)a; return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t* a, int t)
{
    if (!a)
    {
        return EINVAL;
    }
    if (t == PTHREAD_MUTEX_RECURSIVE)
    {
        errno=ENOTSUP;
        return ENOTSUP;
    }
    a->type=t; return 0;
}
int pthread_mutexattr_gettype(const pthread_mutexattr_t* a, int* t)    { if (!a||!t) return EINVAL; *t=a->type; return 0; }
int pthread_mutexattr_setprotocol(pthread_mutexattr_t* a, int p)
{
    if (!a)
    {
        return EINVAL;
    }
    if (p==PTHREAD_PRIO_PROTECT)
    {
        errno=ENOTSUP;
        return ENOTSUP;
    }
    a->protocol=p; return 0;
}
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t* a, int* p) { if (!a||!p) return EINVAL; *p=a->protocol; return 0; }

//  pthread_mutex 

int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* attr)
{
    if (!m)
    {
        errno=EINVAL;
        return EINVAL;
    }
    if (attr && attr->type==PTHREAD_MUTEX_RECURSIVE)
    {
        errno=ENOTSUP;
        return ENOTSUP;
    }
    int idx = mutex_alloc();
    if (idx < 0)
    {
        errno=ENOMEM;
        return ENOMEM;
    }
    *m = static_cast<pthread_mutex_t>(idx);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* m)
{
    if (!m || *m < 0 || (uint32_t)*m >= TAKT_POSIX_MAX_MUTEXES)
    {
        errno=EINVAL;
        return EINVAL;
    }
    s_mutexes[*m].in_use = false;
    *m = -1;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* m)
{
    PmutexSlot* slot = mutex_slot(m);
    if (!slot)
    {
        errno=EINVAL;
        return EINVAL;
    }
    return (TaktOSMutexLock(&slot->m, true, TAKTOS_WAIT_FOREVER) == TAKTOS_OK) ? 0 : EDEADLK;
}

int pthread_mutex_trylock(pthread_mutex_t* m)
{
    PmutexSlot* slot = mutex_slot(m);
    if (!slot)
    {
        errno=EINVAL;
        return EINVAL;
    }
    return (TaktOSMutexLock(&slot->m, false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? 0 : EBUSY;
}

int pthread_mutex_timedlock(pthread_mutex_t* m, const struct timespec* abs)
{
    PmutexSlot* slot = mutex_slot(m);
    if (!slot || !abs)
    {
        errno=EINVAL;
        return EINVAL;
    }
    uint32_t ticks = takt_timespec_to_ticks(abs, takt_posix_tick_hz());
    return (TaktOSMutexLock(&slot->m, true, ticks) == TAKTOS_OK) ? 0 : ETIMEDOUT;
}

int pthread_mutex_unlock(pthread_mutex_t* m)
{
    PmutexSlot* slot = mutex_slot(m);
    if (!slot)
    {
        errno=EINVAL;
        return EINVAL;
    }
    TaktOSMutexUnlock(&slot->m);
    return 0;
}

//  pthread_condattr 

int pthread_condattr_init(pthread_condattr_t* a)    { if (!a) return EINVAL; a->_valid=1; return 0; }
int pthread_condattr_destroy(pthread_condattr_t* a) { (void)a; return 0; }

//  pthread_cond 

int pthread_cond_init(pthread_cond_t* c, const pthread_condattr_t*)
{
    if (!c)
    {
        errno=EINVAL;
        return EINVAL;
    }
    int idx = cond_alloc();
    if (idx < 0)
    {
        errno=ENOMEM;
        return ENOMEM;
    }
    *c = static_cast<pthread_cond_t>(idx);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t* c)
{
    if (!c || *c < 0 || (uint32_t)*c >= TAKT_POSIX_MAX_CONDS)
    {
        errno=EINVAL;
        return EINVAL;
    }
    s_conds[*c].in_use = false;
    *c = -1;
    return 0;
}

int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m)
{
    CondSlot*   cs = cond_slot(c);
    PmutexSlot* ms = mutex_slot(m);
    if (!cs || !ms)
    {
        errno=EINVAL;
        return EINVAL;
    }

    uint32_t primask = TaktOSEnterCritical();
    ++cs->waiters;
    TaktOSExitCritical(primask);

    TaktOSMutexUnlock(&ms->m);
    TaktOSSemTake(&cs->sem, true, TAKTOS_WAIT_FOREVER);
    TaktOSMutexLock(&ms->m, true, TAKTOS_WAIT_FOREVER);
    return 0;
}

int pthread_cond_timedwait(pthread_cond_t* c, pthread_mutex_t* m,
                            const struct timespec* abs)
{
    CondSlot*   cs = cond_slot(c);
    PmutexSlot* ms = mutex_slot(m);
    if (!cs || !ms || !abs)
    {
        errno=EINVAL;
        return EINVAL;
    }

    uint32_t ticks = takt_timespec_to_ticks(abs, takt_posix_tick_hz());

    uint32_t primask = TaktOSEnterCritical();
    ++cs->waiters;
    TaktOSExitCritical(primask);

    TaktOSMutexUnlock(&ms->m);
    bool ok = (TaktOSSemTake(&cs->sem, true, ticks) == TAKTOS_OK);
    TaktOSMutexLock(&ms->m, true, TAKTOS_WAIT_FOREVER);

    if (!ok)
    {
        primask = TaktOSEnterCritical();
        if (cs->waiters > 0u)
        {
            --cs->waiters;
        }
        TaktOSExitCritical(primask);
        errno=ETIMEDOUT; return ETIMEDOUT;
    }
    return 0;
}

int pthread_cond_signal(pthread_cond_t* c)
{
    CondSlot* cs = cond_slot(c);
    if (!cs)
    {
        errno=EINVAL;
        return EINVAL;
    }
    uint32_t primask = TaktOSEnterCritical();
    if (cs->waiters > 0u)
    {
        --cs->waiters;
        TaktOSExitCritical(primask);
        TaktOSSemGive(&cs->sem, false);
    }
    else
    {
        TaktOSExitCritical(primask);
    }
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t* c)
{
    CondSlot* cs = cond_slot(c);
    if (!cs)
    {
        errno=EINVAL;
        return EINVAL;
    }
    uint32_t primask = TaktOSEnterCritical();
    uint32_t n = cs->waiters;
    cs->waiters = 0u;
    TaktOSExitCritical(primask);
    for (uint32_t i = 0u; i < n; ++i)
    {
        TaktOSSemGive(&cs->sem, false);
    }
    return 0;
}

//  pthread_setschedparam 

int pthread_setschedparam(pthread_t t, int, const struct sched_param* p)
{
    PthreadSlot* slot = thread_from_id(t);
    if (!slot || !p)
    {
        errno=EINVAL;
        return EINVAL;
    }
    uint32_t primask = TaktOSEnterCritical();
    if (slot->thread->State == TAKTOS_READY)
    {
        TaktBlockTask(slot->thread);
        slot->thread->Priority = (uint8_t)p->sched_priority;
        TaktReadyTask(slot->thread);
    }
    else
    {
        slot->thread->Priority = (uint8_t)p->sched_priority;
    }
    TaktOSExitCritical(primask);
    return 0;
}

int pthread_getschedparam(pthread_t t, int* policy, struct sched_param* p)
{
    PthreadSlot* slot = thread_from_id(t);
    if (!slot || !policy || !p)
    {
        errno=EINVAL;
        return EINVAL;
    }
    *policy = SCHED_FIFO;
    p->sched_priority = slot->thread->Priority;
    return 0;
}

int sched_get_priority_max(int) { return (int)TAKTOS_MAX_PRI - 1; }
int sched_get_priority_min(int) { return 1; }

//  TLS (single key  PSE51 profile) 

int pthread_key_create(pthread_key_t* key, void (*)(void*))
{
    if (key)
    {
        *key = 0;
    }
    return 0;
}
int pthread_key_delete(pthread_key_t) { return 0; }

void* pthread_getspecific(pthread_key_t)
{
    TaktOSThread_t* cur = TaktOSCurrentThread();
    return cur ? tls_get(cur) : s_tls_fallback;
}

int pthread_setspecific(pthread_key_t, const void* val)
{
    TaktOSThread_t* cur = TaktOSCurrentThread();
    if (cur)
    {
        tls_set(cur, const_cast<void*>(val));
    }
    else
    {
        s_tls_fallback = const_cast<void*>(val);
    }
    return 0;
}
