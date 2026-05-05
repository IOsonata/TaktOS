#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include "kvb.h"
#include "kvb_zephyr_port.h"

#include <zephyr/kernel.h>
#include <zephyr/version.h>

#ifndef KVB_ZEPHYR_NUM_PREEMPT
#if defined(CONFIG_NUM_PREEMPT_PRIORITIES)
#define KVB_ZEPHYR_NUM_PREEMPT CONFIG_NUM_PREEMPT_PRIORITIES
#else
#define KVB_ZEPHYR_NUM_PREEMPT 15
#endif
#endif

/* Map KVB's five canonical priority levels into Zephyr's preemptive
   priority space.  Zephyr uses lower numerical value = higher priority for
   preemptive threads (priority 0 is the highest preemptive priority). */
static int kvb_zephyr_priority(KvbPriority p)
{
    const int max = KVB_ZEPHYR_NUM_PREEMPT;
    int v;

    if (max <= 1) {
        return 0;
    }

    switch (p) {
    case KVB_PRIORITY_LOWEST:
        v = max - 1;
        break;
    case KVB_PRIORITY_LOW:
        v = (3 * max) / 4;
        break;
    case KVB_PRIORITY_NORMAL:
        v = max / 2;
        break;
    case KVB_PRIORITY_HIGH:
        v = max / 4;
        break;
    case KVB_PRIORITY_HIGHEST:
        v = 0;
        break;
    default:
        v = max / 2;
        break;
    }

    if (v < 0) {
        v = 0;
    }
    if (v >= max) {
        v = max - 1;
    }

    return v;
}

static k_timeout_t kvb_zephyr_timeout(uint32_t timeout_ticks)
{
    if (timeout_ticks == KVB_WAIT_FOREVER) {
        return K_FOREVER;
    }

    if (timeout_ticks == KVB_NO_WAIT) {
        return K_NO_WAIT;
    }

    return K_TICKS(timeout_ticks);
}

static KvbStatus kvb_from_zephyr_status(int st, uint32_t timeout_ticks)
{
    if (st == 0) {
        return KVB_OK;
    }

    if (st == -EAGAIN || st == -EBUSY || st == -ENOMEM) {
        return (timeout_ticks == KVB_NO_WAIT) ? KVB_ERR_WOULD_BLOCK : KVB_ERR_TIMEOUT;
    }

    if (st == -EINVAL) {
        return KVB_ERR_INVALID_ARG;
    }

    if (st == -EPERM) {
        return KVB_ERR_NOT_OWNER;
    }

    return KVB_ERR_KERNEL;
}

static void kvb_zephyr_thread_entry(void *p1, void *p2, void *p3)
{
    KvbThread *thread = (KvbThread *)p1;
    (void)p2;
    (void)p3;

    if (thread != 0 && thread->entry != 0) {
        thread->entry(thread->arg);
    }

    for (;;) {
        k_sleep(K_TICKS(KVB_TICK_HZ));
    }
}

static const KvbKernelFeatures g_zephyr_features = {
    .kernel_name                          = "Zephyr",
    .kernel_version                       = KERNEL_VERSION_STRING,
    .supports_dynamic_threads             = true,
    .supports_thread_delete               = true,
    .supports_semaphore                   = true,
    .supports_mutex                       = true,
    .supports_mutex_timeout               = true,
    .supports_mutex_priority_inheritance  = true,
    .supports_mutex_priority_protect      = false,
    /* k_mutex is recursive by default — same thread may re-acquire. */
    .supports_recursive_mutex             = true,
    .supports_queue                       = true,
    .supports_timer                       = false,
    .supports_fixed_pool                  = false,
#ifdef CONFIG_HEAP_MEM_POOL_SIZE
    .supports_general_allocator           = (CONFIG_HEAP_MEM_POOL_SIZE > 0),
#else
    .supports_general_allocator           = false,
#endif
    .supports_isr_kernel_api              = true,
#if defined(CONFIG_HW_STACK_PROTECTION) || defined(CONFIG_STACK_SENTINEL) || defined(CONFIG_MPU_STACK_GUARD)
    .supports_stack_overflow_detection    = true,
#else
    .supports_stack_overflow_detection    = false,
#endif
    .validates_null_args                  = true,
#ifdef CONFIG_OBJ_CORE
    .validates_invalid_objects            = true
#else
    .validates_invalid_objects            = false
#endif
};

const KvbKernelFeatures *kvb_kernel_features(void)
{
    return &g_zephyr_features;
}

KvbStatus kvb_kernel_start(KvbRunnerEntry runner, void *arg)
{
    if (runner == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    /* Zephyr starts the kernel before main(); the KVB runner therefore
       executes inside the application thread.  Lift that thread to the
       canonical RUNNER priority so its scheduling relationship with KVB
       worker threads matches the other ports. */
    k_thread_priority_set(k_current_get(), kvb_zephyr_priority(KVB_PRIORITY_HIGHEST));

    runner(arg);
    return KVB_OK;
}

KvbStatus kvb_thread_create(KvbThread *thread, const char *name, KvbThreadEntry entry,
                            void *arg, void *stack_mem, size_t stack_size, KvbPriority priority)
{
    if (thread == 0 || entry == 0 || stack_mem == 0 || stack_size == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

    thread->entry = entry;
    thread->arg = arg;
    thread->tid = k_thread_create(&thread->thread,
                                  (k_thread_stack_t *)stack_mem,
                                  stack_size,
                                  kvb_zephyr_thread_entry,
                                  thread,
                                  NULL,
                                  NULL,
                                  kvb_zephyr_priority(priority),
                                  0,
                                  K_NO_WAIT);

#if defined(CONFIG_THREAD_NAME)
    if (name != 0 && thread->tid != NULL) {
        (void)k_thread_name_set(thread->tid, name);
    }
#else
    (void)name;
#endif

    return (thread->tid != NULL) ? KVB_OK : KVB_ERR_KERNEL;
}

KvbStatus kvb_thread_yield(void)
{
    k_yield();
    return KVB_OK;
}

uint64_t kvb_kernel_tick_count(void)
{
    return (uint64_t)k_uptime_ticks();
}

KvbStatus kvb_thread_sleep_ticks(uint32_t ticks)
{
    k_sleep(K_TICKS(ticks));
    return KVB_OK;
}

KvbStatus kvb_thread_delete(KvbThread *thread)
{
    if (thread == 0 || thread->tid == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    k_thread_abort(thread->tid);
    thread->tid = NULL;
    return KVB_OK;
}

KvbStatus kvb_sem_create(KvbSemaphore *sem, uint32_t initial_count, uint32_t max_count)
{
    if (sem == 0 || initial_count > max_count || max_count == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_zephyr_status(k_sem_init(&sem->sem, initial_count, max_count), KVB_NO_WAIT);
}

KvbStatus kvb_sem_wait(KvbSemaphore *sem, uint32_t timeout_ticks)
{
    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_zephyr_status(k_sem_take(&sem->sem, kvb_zephyr_timeout(timeout_ticks)),
                                  timeout_ticks);
}

KvbStatus kvb_sem_post(KvbSemaphore *sem)
{
    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    k_sem_give(&sem->sem);
    return KVB_OK;
}

KvbStatus kvb_sem_post_from_isr(KvbSemaphore *sem)
{
    if (sem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    k_sem_give(&sem->sem);
    return KVB_OK;
}

KvbStatus kvb_sem_delete(KvbSemaphore *sem)
{
    (void)sem;
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_mutex_create(KvbMutex *mutex, uint32_t flags)
{
    /* k_mutex supports recursive locking by the owning thread, and KVB's
       feature descriptor advertises that.  PI is also unconditionally
       enabled in the Zephyr build.  No flag combination needs rejection. */
    (void)flags;

    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    k_mutex_init(&mutex->mutex);
    return KVB_OK;
}

KvbStatus kvb_mutex_create_protect(KvbMutex *mutex, KvbPriority ceiling)
{
    /* Zephyr's k_mutex is priority-inheritance only; there is no
     * per-mutex ceiling equivalent to POSIX PRIO_PROTECT. */
    (void)ceiling;
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_mutex_lock(KvbMutex *mutex, uint32_t timeout_ticks)
{
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_zephyr_status(k_mutex_lock(&mutex->mutex,
                                               kvb_zephyr_timeout(timeout_ticks)),
                                  timeout_ticks);
}

KvbStatus kvb_mutex_unlock(KvbMutex *mutex)
{
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_zephyr_status(k_mutex_unlock(&mutex->mutex), KVB_NO_WAIT);
}

KvbStatus kvb_mutex_delete(KvbMutex *mutex)
{
    (void)mutex;
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_queue_create(KvbQueue *queue, void *storage, size_t message_size, uint32_t message_count)
{
    if (queue == 0 || storage == 0 || message_size == 0u || message_count == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

    k_msgq_init(&queue->msgq, (char *)storage, message_size, message_count);
    return KVB_OK;
}

KvbStatus kvb_queue_send(KvbQueue *queue, const void *msg, uint32_t timeout_ticks)
{
    if (queue == 0 || msg == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_zephyr_status(k_msgq_put(&queue->msgq, msg, kvb_zephyr_timeout(timeout_ticks)),
                                  timeout_ticks);
}

KvbStatus kvb_queue_recv(KvbQueue *queue, void *msg, uint32_t timeout_ticks)
{
    if (queue == 0 || msg == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_from_zephyr_status(k_msgq_get(&queue->msgq, msg, kvb_zephyr_timeout(timeout_ticks)),
                                  timeout_ticks);
}

KvbStatus kvb_queue_delete(KvbQueue *queue)
{
    (void)queue;
    return KVB_ERR_UNSUPPORTED;
}

int kvb_zephyr_start_default(void)
{
    KvbStatus st = kvb_kernel_start(kvb_runner_main, 0);
    return (st == KVB_OK) ? 0 : -1;
}
