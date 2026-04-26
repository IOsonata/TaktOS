/**---------------------------------------------------------------------------
@file   threads.h

@brief  TaktOS C11/C17/C23 <threads.h> compatibility layer

This header provides the ISO C thread API on top of the existing TaktOS
PSE51 pthread layer.  It is intentionally a small standards front end, not a
second threading implementation.  Thread storage, stacks, mutexes, condition
variables, and TLS keys are still allocated from the static POSIX/TaktOS pools.

Build note: place TaktOS/include before the toolchain system include path so
#include <threads.h> resolves to this file on freestanding embedded targets.

QM - outside cert boundary.

@author Nguyen Hoan Hoang
@date   Apr. 2026

@license MIT
----------------------------------------------------------------------------*/
#ifndef __TAKT_C_THREADS_H__
#define __TAKT_C_THREADS_H__

#include <stdint.h>
#include <stddef.h>

#include "posix/pthread.h"
#include "posix/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* C11 7.26.1 status codes.  Values are implementation-defined. */
enum {
    thrd_success  = 0,
    thrd_nomem    = 1,
    thrd_timedout = 2,
    thrd_busy     = 3,
    thrd_error    = 4
};

/* C11 mutex type flags.  Keep these as a bit mask for mtx_init(). */
enum {
    mtx_plain     = 0,
    mtx_timed     = 1,
    mtx_recursive = 2
};

typedef pthread_t       thrd_t;
typedef pthread_once_t  once_flag;
typedef pthread_key_t   tss_t;

typedef int  (*thrd_start_t)(void *);
typedef void (*tss_dtor_t)(void *);

#define ONCE_FLAG_INIT  PTHREAD_ONCE_INIT
#define TSS_DTOR_ITERATIONS  4

typedef struct {
    pthread_mutex_t native;
    int             type;
    int             initialized;
} mtx_t;

typedef struct {
    pthread_cond_t  native;
    int             initialized;
} cnd_t;

/* Thread functions */
int    thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int    thrd_equal(thrd_t lhs, thrd_t rhs);
thrd_t thrd_current(void);
int    thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void   thrd_yield(void);
void   thrd_exit(int res) __attribute__((noreturn));
int    thrd_detach(thrd_t thr);
int    thrd_join(thrd_t thr, int *res);

/* Mutex functions */
int  mtx_init(mtx_t *mtx, int type);
void mtx_destroy(mtx_t *mtx);
int  mtx_lock(mtx_t *mtx);
int  mtx_trylock(mtx_t *mtx);
int  mtx_timedlock(mtx_t *mtx, const struct timespec *time_point);
int  mtx_unlock(mtx_t *mtx);

/* Condition variable functions */
int  cnd_init(cnd_t *cond);
void cnd_destroy(cnd_t *cond);
int  cnd_wait(cnd_t *cond, mtx_t *mtx);
int  cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *time_point);
int  cnd_signal(cnd_t *cond);
int  cnd_broadcast(cnd_t *cond);

/* One-time initialization */
void call_once(once_flag *flag, void (*func)(void));

/* Thread-specific storage */
int   tss_create(tss_t *key, tss_dtor_t destructor);
void  tss_delete(tss_t key);
void *tss_get(tss_t key);
int   tss_set(tss_t key, void *val);

#ifdef __cplusplus
}
#endif

#endif /* __TAKT_C_THREADS_H__ */
