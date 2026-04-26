#include <threads.h>

static mtx_t gLock;
static cnd_t gCond;
static int   gReady;
static int   gValue;

static int worker(void *arg)
{
    (void)arg;

    mtx_lock(&gLock);
    gValue = 1234;
    gReady = 1;
    cnd_signal(&gCond);
    mtx_unlock(&gLock);

    return 7;
}

void std_c_threads_example(void)
{
    thrd_t t;
    int result = 0;

    mtx_init(&gLock, mtx_plain);
    cnd_init(&gCond);

    thrd_create(&t, worker, NULL);

    mtx_lock(&gLock);
    while (!gReady)
    {
        cnd_wait(&gCond, &gLock);
    }
    mtx_unlock(&gLock);

    thrd_join(t, &result);

    cnd_destroy(&gCond);
    mtx_destroy(&gLock);

    (void)gValue;
    (void)result;
}
