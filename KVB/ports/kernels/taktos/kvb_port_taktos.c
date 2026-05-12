#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "kvb.h"
#include "kvb_taktos_port.h"

#include "TaktOS.h"
#include "TaktKernel.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"

/* The runner's buffer must include the per-arch TCB+guard+frame overhead so
   that the runner sees KVB_RUNNER_STACK_SIZE bytes of usable stack — the
   same usable size the FreeRTOS/ThreadX/Zephyr ports give their runners. */
static KVB_THREAD_STACK_DEFINE(g_kvb_runner_stack, KVB_RUNNER_STACK_SIZE);
static KvbRunnerEntry g_runner_entry;
static void *g_runner_arg;

/* Map KVB's five canonical priority levels into TaktOS's native space.
   TaktOS uses higher numerical value = higher priority, with 0 reserved
   for the idle thread.  The values below give five distinct levels with
   meaningful separation. */
static uint8_t kvb_taktos_priority(KvbPriority p)
{
    static const uint8_t map[KVB_PRIORITY_COUNT] = { 4u, 8u, 16u, 24u, 28u };

    if ((unsigned)p >= (unsigned)KVB_PRIORITY_COUNT) {
        return map[KVB_PRIORITY_NORMAL];
    }

    return map[p];
}

static KvbStatus kvb_from_taktos_error(TaktOSErr_t err)
{
    switch (err) {
    case TAKTOS_OK:
        return KVB_OK;
    case TAKTOS_ERR_TIMEOUT:
        return KVB_ERR_TIMEOUT;
    case TAKTOS_ERR_FULL:
    case TAKTOS_ERR_EMPTY:
    case TAKTOS_ERR_BUSY:
        return KVB_ERR_WOULD_BLOCK;
    case TAKTOS_ERR_INVALID:
    case TAKTOS_ERR_ALIGN:
        return KVB_ERR_INVALID_ARG;
    default:
        return KVB_ERR_KERNEL;
    }
}

static void kvb_runner_trampoline(void *arg)
{
    (void)arg;

    if (g_runner_entry != 0) {
        g_runner_entry(g_runner_arg);
    }

    for (;;) {
        (void)TaktOSThreadSleepTicks(TaktOSCurrentThread(), KVB_TICK_HZ);
    }
}

static const KvbKernelFeatures g_taktos_features = {
    .kernel_name                          = "TaktOS",
    .kernel_version                       = "private-dev",
    .supports_dynamic_threads             = true,
    .supports_thread_delete               = true,
    .supports_semaphore                   = true,
    .supports_mutex                       = true,
    .supports_mutex_timeout               = true,
    .supports_mutex_priority_inheritance  = false,
    .supports_mutex_priority_protect      = true,
    .supports_recursive_mutex             = false,
    .supports_queue                       = true,
    .supports_timer                       = false,
    .supports_fixed_pool                  = false,
    .supports_general_allocator           = false,
    .supports_isr_kernel_api              = true,
    .supports_stack_overflow_detection    = true,
    .validates_null_args                  = true,
    .validates_invalid_objects            = true
};

const KvbKernelFeatures *kvb_kernel_features(void)
{
    return &g_taktos_features;
}

KvbStatus kvb_kernel_start(KvbRunnerEntry runner, void *arg)
{
    hTaktOSThread_t h;

    if (runner == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    g_runner_entry = runner;
    g_runner_arg = arg;

    TaktOSCfg_t cfg = {
        .KernClockHz     = KVB_CORE_CLOCK_HZ,
        .TickHz          = KVB_TICK_HZ,
    };
    if (TaktOSInit(&cfg) != TAKTOS_OK) {
        return KVB_ERR_KERNEL;
    }

    h = TaktOSThreadCreate(KVB_THREAD_STACK_MEM(g_kvb_runner_stack),
                           (uint32_t)KVB_THREAD_STACK_SIZE(g_kvb_runner_stack),
                           kvb_runner_trampoline,
                           0,
                           kvb_taktos_priority(KVB_PRIORITY_HIGHEST));

    if (h == 0) {
        return KVB_ERR_KERNEL;
    }

    TaktOSStart();

    return KVB_ERR_KERNEL;
}

KvbStatus kvb_thread_create(
    KvbThread *thread,
    const char *name,
    KvbThreadEntry entry,
    void *arg,
    void *stack_mem,
    size_t stack_size,
    KvbPriority priority)
{
    hTaktOSThread_t h;

    (void)name;

    if (thread == 0 || entry == 0 || stack_mem == 0 || stack_size == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

    h = TaktOSThreadCreate(stack_mem,
                           (uint32_t)stack_size,
                           entry,
                           arg,
                           kvb_taktos_priority(priority));

    if (h == 0) {
        thread->handle = 0;
        return KVB_ERR_KERNEL;
    }

    thread->handle = h;
    return KVB_OK;
}

KvbStatus kvb_thread_yield(void)
{
    TaktOSThreadYield();
    return KVB_OK;
}

uint64_t kvb_kernel_tick_count(void)
{
    return (uint64_t)TaktOSTickCount();
}

KvbStatus kvb_thread_sleep_ticks(uint32_t ticks)
{
    hTaktOSThread_t cur = TaktOSCurrentThread();

    if (cur == 0) {
        return KVB_ERR_KERNEL;
    }

    return kvb_from_taktos_error(TaktOSThreadSleepTicks(cur, ticks));
}

KvbStatus kvb_thread_delete(KvbThread *thread)
{
    if (thread == 0 || thread->handle == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_taktos_error(TaktOSThreadDestroy(thread->handle));
}

KvbStatus kvb_sem_create(KvbSemaphore *sem, uint32_t initial_count, uint32_t max_count)
{
    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_taktos_error(TaktOSSemInit(&sem->sem, initial_count, max_count));
}

KvbStatus kvb_sem_wait(KvbSemaphore *sem, uint32_t timeout_ticks)
{
    bool blocking;

    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    blocking = (timeout_ticks != KVB_NO_WAIT);
    return kvb_from_taktos_error(TaktOSSemTake(&sem->sem, blocking, timeout_ticks));
}

KvbStatus kvb_sem_post(KvbSemaphore *sem)
{
    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_taktos_error(TaktOSSemGive(&sem->sem, false));
}

KvbStatus kvb_sem_post_from_isr(KvbSemaphore *sem)
{
    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_taktos_error(TaktOSSemGive(&sem->sem, false));
}

KvbStatus kvb_sem_delete(KvbSemaphore *sem)
{
    (void)sem;
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_mutex_create(KvbMutex *mutex, uint32_t flags)
{
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    /* TaktOS v1.0 mutex semantics:
     *   - non-recursive (one acquire per task; second acquire by same task
     *     returns ERR_BUSY/TIMEOUT)
     *   - non-PI (priority inheritance is a v1.x roadmap item; ownerBasePri
     *     is reserved in the TCB but not yet boosted on contention — see
     *     src/taktos_mutex.cpp file header)
     *
     * For IPCP / priority-protect mutexes, callers must use
     * kvb_mutex_create_protect() and supply a ceiling — those are surfaced
     * by the supports_mutex_priority_protect feature flag.
     *
     * Reject flag combinations the plain mutex does not implement so that
     * tests cannot silently exercise emulated behaviour.  The supports_mutex_*
     * fields in g_taktos_features below report this directly so a test
     * can skip itself if it needs PI or recursion. */
    if (((flags & KVB_MUTEX_RECURSIVE) != 0u) ||
        ((flags & KVB_MUTEX_PI_REQUIRED) != 0u) ||
        ((flags & KVB_MUTEX_PROTECT) != 0u)) {
        return KVB_ERR_UNSUPPORTED;
    }

    return kvb_from_taktos_error(TaktOSMutexInit(&mutex->mutex));
}

KvbStatus kvb_mutex_create_protect(KvbMutex *mutex, KvbPriority ceiling)
{
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_taktos_error(
        TaktOSMutexInitProtect(&mutex->mutex, kvb_taktos_priority(ceiling)));
}

KvbStatus kvb_mutex_lock(KvbMutex *mutex, uint32_t timeout_ticks)
{
    bool blocking;

    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    blocking = (timeout_ticks != KVB_NO_WAIT);
    return kvb_from_taktos_error(TaktOSMutexLock(&mutex->mutex, blocking, timeout_ticks));
}

KvbStatus kvb_mutex_unlock(KvbMutex *mutex)
{
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    {
        TaktOSErr_t err = TaktOSMutexUnlock(&mutex->mutex);
        if (err == TAKTOS_ERR_INVALID) {
            return KVB_ERR_NOT_OWNER;
        }
        return kvb_from_taktos_error(err);
    }
}

KvbStatus kvb_mutex_delete(KvbMutex *mutex)
{
    (void)mutex;
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_queue_create(
    KvbQueue *queue,
    void *storage,
    size_t message_size,
    uint32_t message_count)
{
    if (queue == 0 || storage == 0 || message_size == 0u || message_count == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_taktos_error(TaktOSQueueInit(&queue->queue,
                                                 storage,
                                                 (uint32_t)message_size,
                                                 message_count));
}

KvbStatus kvb_queue_send(KvbQueue *queue, const void *msg, uint32_t timeout_ticks)
{
    bool blocking;

    if (queue == 0 || msg == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    blocking = (timeout_ticks != KVB_NO_WAIT);
    return kvb_from_taktos_error(TaktOSQueueSend(&queue->queue, msg, blocking, timeout_ticks));
}

KvbStatus kvb_queue_recv(KvbQueue *queue, void *msg, uint32_t timeout_ticks)
{
    bool blocking;

    if (queue == 0 || msg == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    blocking = (timeout_ticks != KVB_NO_WAIT);
    return kvb_from_taktos_error(TaktOSQueueReceive(&queue->queue, msg, blocking, timeout_ticks));
}

KvbStatus kvb_queue_delete(KvbQueue *queue)
{
    (void)queue;
    return KVB_ERR_UNSUPPORTED;
}

int kvb_taktos_start_default(void)
{
    KvbStatus st = kvb_kernel_start(kvb_runner_main, 0);
    return (st == KVB_OK) ? 0 : -1;
}
