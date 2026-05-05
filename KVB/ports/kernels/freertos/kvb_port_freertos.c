#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "kvb.h"
#include "kvb_freertos_port.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#ifndef KVB_FREERTOS_RUNNER_STACK_WORDS
#define KVB_FREERTOS_RUNNER_STACK_WORDS \
    ((KVB_RUNNER_STACK_SIZE + sizeof(StackType_t) - 1u) / sizeof(StackType_t))
#endif

#if (configSUPPORT_STATIC_ALLOCATION == 1)
static StackType_t g_kvb_runner_stack[KVB_FREERTOS_RUNNER_STACK_WORDS]
    __attribute__((aligned(portBYTE_ALIGNMENT)));
static StaticTask_t g_kvb_runner_tcb __attribute__((aligned(portBYTE_ALIGNMENT)));
#endif
static KvbRunnerEntry g_runner_entry;
static void *g_runner_arg;

/* Map KVB's five canonical priority levels into FreeRTOS's native space.
   FreeRTOS uses higher numerical value = higher priority, with priority 0
   reserved for tskIDLE_PRIORITY.  KVB priorities therefore start at 1.  For
   a typical configMAX_PRIORITIES of 8 or more, the levels stay distinct. */
static UBaseType_t kvb_freertos_priority(KvbPriority p)
{
    const UBaseType_t max = (UBaseType_t)configMAX_PRIORITIES;
    UBaseType_t v;

    if (max < 2u) {
        return 0u;
    }

    switch (p) {
    case KVB_PRIORITY_LOWEST:
        v = 1u;
        break;
    case KVB_PRIORITY_LOW:
        v = 1u + (max - 2u) / 4u;
        break;
    case KVB_PRIORITY_NORMAL:
        v = 1u + (max - 2u) / 2u;
        break;
    case KVB_PRIORITY_HIGH:
        v = 1u + (3u * (max - 2u)) / 4u;
        break;
    case KVB_PRIORITY_HIGHEST:
        v = max - 1u;
        break;
    default:
        v = 1u + (max - 2u) / 2u;
        break;
    }

    if (v >= max) {
        v = max - 1u;
    }

    return v;
}

static TickType_t kvb_freertos_timeout(uint32_t timeout_ticks)
{
    if (timeout_ticks == KVB_WAIT_FOREVER) {
        return portMAX_DELAY;
    }

    return (TickType_t)timeout_ticks;
}

static KvbStatus kvb_freertos_bool_status(BaseType_t ok, uint32_t timeout_ticks)
{
    if (ok == pdTRUE) {
        return KVB_OK;
    }

    return (timeout_ticks == KVB_NO_WAIT) ? KVB_ERR_WOULD_BLOCK : KVB_ERR_TIMEOUT;
}

static void kvb_runner_task(void *arg)
{
    (void)arg;

    if (g_runner_entry != 0) {
        g_runner_entry(g_runner_arg);
    }

    for (;;) {
        vTaskDelay((TickType_t)KVB_TICK_HZ);
    }
}

static const KvbKernelFeatures g_freertos_features = {
    .kernel_name                          = "FreeRTOS",
    .kernel_version                       = tskKERNEL_VERSION_NUMBER,
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1) || (configSUPPORT_STATIC_ALLOCATION == 1)
    .supports_dynamic_threads             = true,
#else
    .supports_dynamic_threads             = false,
#endif
    .supports_thread_delete               = true,
    .supports_semaphore                   = true,
    .supports_mutex                       = true,
    .supports_mutex_timeout               = true,
    .supports_mutex_priority_inheritance  = true,
    .supports_mutex_priority_protect      = false,
#if (configUSE_RECURSIVE_MUTEXES == 1)
    .supports_recursive_mutex             = true,
#else
    .supports_recursive_mutex             = false,
#endif
    .supports_queue                       = true,
    .supports_timer                       = false,
    .supports_fixed_pool                  = false,
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    .supports_general_allocator           = true,
#else
    .supports_general_allocator           = false,
#endif
    .supports_isr_kernel_api              = true,
#if (configCHECK_FOR_STACK_OVERFLOW > 0)
    .supports_stack_overflow_detection    = true,
#else
    .supports_stack_overflow_detection    = false,
#endif
    .validates_null_args                  = true,
    .validates_invalid_objects            = false
};

const KvbKernelFeatures *kvb_kernel_features(void)
{
    return &g_freertos_features;
}

KvbStatus kvb_kernel_start(KvbRunnerEntry runner, void *arg)
{
    TaskHandle_t h;

    if (runner == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    g_runner_entry = runner;
    g_runner_arg = arg;

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    h = xTaskCreateStatic(kvb_runner_task,
                          "kvb_runner",
                          (uint32_t)KVB_FREERTOS_RUNNER_STACK_WORDS,
                          0,
                          kvb_freertos_priority(KVB_PRIORITY_HIGHEST),
                          g_kvb_runner_stack,
                          &g_kvb_runner_tcb);
    if (h == NULL) {
        return KVB_ERR_KERNEL;
    }
#elif (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    if (xTaskCreate(kvb_runner_task,
                    "kvb_runner",
                    (uint16_t)KVB_FREERTOS_RUNNER_STACK_WORDS,
                    0,
                    kvb_freertos_priority(KVB_PRIORITY_HIGHEST),
                    &h) != pdPASS) {
        return KVB_ERR_KERNEL;
    }
#else
    (void)h;
    return KVB_ERR_UNSUPPORTED;
#endif

    vTaskStartScheduler();
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
    uint32_t stack_words;

    if (thread == 0 || entry == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    if (name == 0) {
        name = "kvb_thread";
    }

    stack_words = (uint32_t)((stack_size + sizeof(StackType_t) - 1u) / sizeof(StackType_t));
    if (stack_words == 0u) {
        return KVB_ERR_INVALID_ARG;
    }
    if (stack_words < (uint32_t)configMINIMAL_STACK_SIZE) {
        stack_words = (uint32_t)configMINIMAL_STACK_SIZE;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    if (stack_mem == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    thread->handle = xTaskCreateStatic((TaskFunction_t)entry,
                                       name,
                                       stack_words,
                                       arg,
                                       kvb_freertos_priority(priority),
                                       (StackType_t *)stack_mem,
                                       &thread->tcb);
    return (thread->handle != NULL) ? KVB_OK : KVB_ERR_KERNEL;
#elif (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    (void)stack_mem;
    if (xTaskCreate((TaskFunction_t)entry,
                    name,
                    (uint16_t)stack_words,
                    arg,
                    kvb_freertos_priority(priority),
                    &thread->handle) != pdPASS) {
        thread->handle = NULL;
        return KVB_ERR_KERNEL;
    }
    return KVB_OK;
#else
    (void)stack_mem;
    return KVB_ERR_UNSUPPORTED;
#endif
}

KvbStatus kvb_thread_yield(void)
{
    taskYIELD();
    return KVB_OK;
}

uint64_t kvb_kernel_tick_count(void)
{
    return (uint64_t)xTaskGetTickCount();
}

KvbStatus kvb_thread_sleep_ticks(uint32_t ticks)
{
    vTaskDelay((TickType_t)ticks);
    return KVB_OK;
}

KvbStatus kvb_thread_delete(KvbThread *thread)
{
    if (thread == 0 || thread->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    vTaskDelete(thread->handle);
    thread->handle = NULL;
    return KVB_OK;
}

KvbStatus kvb_sem_create(KvbSemaphore *sem, uint32_t initial_count, uint32_t max_count)
{
    if (sem == 0 || max_count == 0u || initial_count > max_count) {
        return KVB_ERR_INVALID_ARG;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    sem->handle = xSemaphoreCreateCountingStatic((UBaseType_t)max_count,
                                                 (UBaseType_t)initial_count,
                                                 &sem->storage);
#elif (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    sem->handle = xSemaphoreCreateCounting((UBaseType_t)max_count,
                                           (UBaseType_t)initial_count);
#else
    sem->handle = NULL;
    return KVB_ERR_UNSUPPORTED;
#endif

    return (sem->handle != NULL) ? KVB_OK : KVB_ERR_KERNEL;
}

KvbStatus kvb_sem_wait(KvbSemaphore *sem, uint32_t timeout_ticks)
{
    if (sem == 0 || sem->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_freertos_bool_status(xSemaphoreTake(sem->handle, kvb_freertos_timeout(timeout_ticks)),
                                    timeout_ticks);
}

KvbStatus kvb_sem_post(KvbSemaphore *sem)
{
    if (sem == 0 || sem->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    return (xSemaphoreGive(sem->handle) == pdTRUE) ? KVB_OK : KVB_ERR_KERNEL;
}

KvbStatus kvb_sem_post_from_isr(KvbSemaphore *sem)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    BaseType_t ok;

    if (sem == 0 || sem->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    ok = xSemaphoreGiveFromISR(sem->handle, &higher_priority_task_woken);
    if (ok != pdTRUE) {
        return KVB_ERR_KERNEL;
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
    return KVB_OK;
}

KvbStatus kvb_sem_delete(KvbSemaphore *sem)
{
    if (sem == 0 || sem->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    vSemaphoreDelete(sem->handle);
    sem->handle = NULL;
    return KVB_OK;
}

KvbStatus kvb_mutex_create(KvbMutex *mutex, uint32_t flags)
{
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    mutex->recursive = ((flags & KVB_MUTEX_RECURSIVE) != 0u);

    /* If the build did not enable recursive mutexes but the test asked
       for one, surface that as UNSUPPORTED rather than silently returning
       a non-recursive mutex. */
    if (mutex->recursive) {
#if (configUSE_RECURSIVE_MUTEXES != 1)
        return KVB_ERR_UNSUPPORTED;
#endif
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (mutex->recursive) {
        mutex->handle = xSemaphoreCreateRecursiveMutexStatic(&mutex->storage);
    } else
#endif
    {
        mutex->handle = xSemaphoreCreateMutexStatic(&mutex->storage);
    }
#elif (configSUPPORT_DYNAMIC_ALLOCATION == 1)
#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (mutex->recursive) {
        mutex->handle = xSemaphoreCreateRecursiveMutex();
    } else
#endif
    {
        mutex->handle = xSemaphoreCreateMutex();
    }
#else
    mutex->handle = NULL;
    return KVB_ERR_UNSUPPORTED;
#endif

    return (mutex->handle != NULL) ? KVB_OK : KVB_ERR_KERNEL;
}

KvbStatus kvb_mutex_create_protect(KvbMutex *mutex, KvbPriority ceiling)
{
    /* FreeRTOS V11.3 does not implement a POSIX-PRIO_PROTECT-equivalent
     * priority-ceiling mutex.  Its mutexes are priority-inheritance only. */
    (void)ceiling;
    if (mutex == 0) {
        return KVB_ERR_INVALID_ARG;
    }
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_mutex_lock(KvbMutex *mutex, uint32_t timeout_ticks)
{
    BaseType_t ok;

    if (mutex == 0 || mutex->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (mutex->recursive) {
        ok = xSemaphoreTakeRecursive(mutex->handle, kvb_freertos_timeout(timeout_ticks));
    } else
#endif
    {
        ok = xSemaphoreTake(mutex->handle, kvb_freertos_timeout(timeout_ticks));
    }

    return kvb_freertos_bool_status(ok, timeout_ticks);
}

KvbStatus kvb_mutex_unlock(KvbMutex *mutex)
{
    BaseType_t ok;

    if (mutex == 0 || mutex->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (mutex->recursive) {
        ok = xSemaphoreGiveRecursive(mutex->handle);
        return (ok == pdTRUE) ? KVB_OK : KVB_ERR_NOT_OWNER;
    }
#endif

    /* FreeRTOS V11.3 asserts inside xQueueGenericSend when a non-owner task
     * tries to give a mutex that has a current holder.  The assert path
     * (configASSERT firing -> taskDISABLE_INTERRUPTS + for(;;){}) freezes
     * the entire system instead of returning pdFAIL.  KVB's
     * SYNC_MUTEX_OWNERSHIP_001 test deliberately drives this case to verify
     * the kernel rejects non-owner unlock, so we have to detect ownership
     * ourselves and return KVB_ERR_NOT_OWNER without ever entering the
     * asserting code path.
     *
     * INCLUDE_xSemaphoreGetMutexHolder must be 1 for this to compile.  The
     * holder pointer is read with the scheduler suspended internally so
     * the value is consistent for the duration of the comparison. */
#if (INCLUDE_xSemaphoreGetMutexHolder == 1)
    {
        TaskHandle_t holder = xSemaphoreGetMutexHolder(mutex->handle);
        TaskHandle_t self   = xTaskGetCurrentTaskHandle();
        if (holder != self) {
            return KVB_ERR_NOT_OWNER;
        }
    }
#endif

    ok = xSemaphoreGive(mutex->handle);
    return (ok == pdTRUE) ? KVB_OK : KVB_ERR_NOT_OWNER;
}

KvbStatus kvb_mutex_delete(KvbMutex *mutex)
{
    if (mutex == 0 || mutex->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    vSemaphoreDelete(mutex->handle);
    mutex->handle = NULL;
    return KVB_OK;
}

KvbStatus kvb_queue_create(KvbQueue *queue, void *storage, size_t message_size, uint32_t message_count)
{
    if (queue == 0 || message_size == 0u || message_count == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    if (storage == 0) {
        return KVB_ERR_INVALID_ARG;
    }
    queue->handle = xQueueCreateStatic((UBaseType_t)message_count,
                                       (UBaseType_t)message_size,
                                       (uint8_t *)storage,
                                       &queue->storage);
#elif (configSUPPORT_DYNAMIC_ALLOCATION == 1)
    (void)storage;
    queue->handle = xQueueCreate((UBaseType_t)message_count,
                                 (UBaseType_t)message_size);
#else
    (void)storage;
    queue->handle = NULL;
    return KVB_ERR_UNSUPPORTED;
#endif

    return (queue->handle != NULL) ? KVB_OK : KVB_ERR_KERNEL;
}

KvbStatus kvb_queue_send(KvbQueue *queue, const void *msg, uint32_t timeout_ticks)
{
    if (queue == 0 || queue->handle == NULL || msg == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_freertos_bool_status(xQueueSendToBack(queue->handle, msg,
                                                    kvb_freertos_timeout(timeout_ticks)),
                                    timeout_ticks);
}

KvbStatus kvb_queue_recv(KvbQueue *queue, void *msg, uint32_t timeout_ticks)
{
    if (queue == 0 || queue->handle == NULL || msg == 0) {
        return KVB_ERR_INVALID_ARG;
    }

    return kvb_freertos_bool_status(xQueueReceive(queue->handle, msg,
                                                  kvb_freertos_timeout(timeout_ticks)),
                                    timeout_ticks);
}

KvbStatus kvb_queue_delete(KvbQueue *queue)
{
    if (queue == 0 || queue->handle == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    vQueueDelete(queue->handle);
    queue->handle = NULL;
    return KVB_OK;
}

int kvb_freertos_start_default(void)
{
    KvbStatus st = kvb_kernel_start(kvb_runner_main, 0);
    return (st == KVB_OK) ? 0 : -1;
}
