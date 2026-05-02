/**-------------------------------------------------------------------------
 * @file    kvb_port_threadx.c
 *
 * @brief   KVB API -> Eclipse ThreadX API mapping.
 *
 * v13 redesign — see kvb_threadx_types.h for the full root-cause writeup.
 *
 * Key change vs earlier revisions:  TX_THREAD / TX_SEMAPHORE / TX_MUTEX /
 * TX_QUEUE control blocks live in port-private BSS pools, allocated by
 * slot index when the user calls kvb_thread_create / kvb_sem_create /
 * etc.  KvbThread / KvbSemaphore / KvbMutex / KvbQueue are 4-byte
 * opaque handles that point into the pool, so they are safe to place
 * on the stack — exactly the storage shape used by Microsoft's
 * Thread-Metric ThreadX reference port (tm_port_threadx.c, shared
 * across every ThreadX-validated MCU in this repo).
 *
 * Pool sizing rationale (STM32F0308, 8 KB SRAM):
 *
 *   THREAD pool   = KVB_WORKER_THREAD_COUNT + 4
 *                   = 3 workers + 1 mutex-owner + 3 spare = 7 typical, 8 here
 *   SEM pool      = 8   (most tests use 2–3, headroom for SYNC_*)
 *   MUTEX pool    = 4   (SYNC_MUTEX_FAST + SYNC_MUTEX_OWNERSHIP simultaneously)
 *   QUEUE pool    = 4   (IPC_QUEUE_FAST + future tests)
 *
 * Differences vs the FreeRTOS port worth noting:
 *
 *   - ThreadX startup uses a callback model.  tx_kernel_enter() never
 *     returns; it calls tx_application_define() during init, where the
 *     application creates its initial threads.
 *
 *   - ThreadX mutex unlock-by-non-owner returns TX_NOT_OWNED cleanly
 *     instead of asserting (FreeRTOS V11.3 asserts here, which forced
 *     the FreeRTOS port to pre-check ownership on the fast path at
 *     ~30 cycles per unlock).  ThreadX therefore needs no such pre-
 *     check, and SYNC_MUTEX_FAST_001 measures the unmodified ThreadX
 *     lock/unlock fast path.
 *
 *   - ThreadX priority space is 0..(TX_MAX_PRIORITIES-1), with 0 = highest
 *     and (TX_MAX_PRIORITIES-1) = lowest.  Inverted from FreeRTOS.
 * -------------------------------------------------------------------------*/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "kvb.h"
#include "kvb_threadx_port.h"   /* declares kvb_threadx_start_default + sentinel */

#include "tx_api.h"

#ifndef KVB_THREADX_RUNNER_STACK_BYTES
#define KVB_THREADX_RUNNER_STACK_BYTES KVB_RUNNER_STACK_SIZE
#endif

/* -------------------------------------------------------------------------- *
 * Pool sizing
 * -------------------------------------------------------------------------- */

#ifndef KVB_THREADX_THREAD_POOL_SIZE
#define KVB_THREADX_THREAD_POOL_SIZE  ((KVB_WORKER_THREAD_COUNT) + 4u)
#endif

#ifndef KVB_THREADX_SEM_POOL_SIZE
#define KVB_THREADX_SEM_POOL_SIZE     8u
#endif

#ifndef KVB_THREADX_MUTEX_POOL_SIZE
#define KVB_THREADX_MUTEX_POOL_SIZE   4u
#endif

#ifndef KVB_THREADX_QUEUE_POOL_SIZE
#define KVB_THREADX_QUEUE_POOL_SIZE   4u
#endif

/* -------------------------------------------------------------------------- *
 * Runner thread (always present, owns the test loop)
 * -------------------------------------------------------------------------- */

static uint8_t s_kvb_runner_stack[KVB_THREADX_RUNNER_STACK_BYTES]
    __attribute__((aligned(KVB_THREADX_ALIGN)));
static TX_THREAD s_kvb_runner_tcb __attribute__((aligned(KVB_THREADX_ALIGN)));
static KvbRunnerEntry s_runner_entry;
static void *s_runner_arg;

/* -------------------------------------------------------------------------- *
 * Port-private pools — TX_THREAD / TX_SEMAPHORE / TX_MUTEX / TX_QUEUE
 *
 * Each pool entry has a `used` flag.  Allocation is O(N) linear scan
 * under a critical section.  N is small (4–8), so the cost is dominated
 * by the kernel call that follows.  Linear scan keeps the implementation
 * trivial and avoids any free-list pointer that could itself land in a
 * surprising memory location.
 * -------------------------------------------------------------------------- */

typedef struct {
    TX_THREAD       tcb;
    KvbThreadEntry  entry;
    void           *arg;
    bool            used;
} kvb_threadx_thread_slot_t;

typedef struct {
    TX_SEMAPHORE    sem;
    bool            used;
} kvb_threadx_sem_slot_t;

typedef struct {
    TX_MUTEX        mtx;
    bool            recursive;
    bool            used;
} kvb_threadx_mutex_slot_t;

typedef struct {
    TX_QUEUE        q;
    bool            used;
} kvb_threadx_queue_slot_t;

static kvb_threadx_thread_slot_t s_thread_pool[KVB_THREADX_THREAD_POOL_SIZE]
    __attribute__((aligned(KVB_THREADX_ALIGN)));
static kvb_threadx_sem_slot_t    s_sem_pool   [KVB_THREADX_SEM_POOL_SIZE]
    __attribute__((aligned(KVB_THREADX_ALIGN)));
static kvb_threadx_mutex_slot_t  s_mutex_pool [KVB_THREADX_MUTEX_POOL_SIZE]
    __attribute__((aligned(KVB_THREADX_ALIGN)));
static kvb_threadx_queue_slot_t  s_queue_pool [KVB_THREADX_QUEUE_POOL_SIZE]
    __attribute__((aligned(KVB_THREADX_ALIGN)));

static kvb_threadx_thread_slot_t *kvb_threadx_alloc_thread_slot(void)
{
    UINT i, old_posture;
    kvb_threadx_thread_slot_t *result = NULL;

    old_posture = tx_interrupt_control(TX_INT_DISABLE);
    for (i = 0u; i < KVB_THREADX_THREAD_POOL_SIZE; ++i) {
        if (!s_thread_pool[i].used) {
            s_thread_pool[i].used = true;
            result = &s_thread_pool[i];
            break;
        }
    }
    (void)tx_interrupt_control(old_posture);
    return result;
}

static void kvb_threadx_free_thread_slot(kvb_threadx_thread_slot_t *slot)
{
    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
    slot->used  = false;
    slot->entry = NULL;
    slot->arg   = NULL;
    (void)tx_interrupt_control(old_posture);
}

static kvb_threadx_sem_slot_t *kvb_threadx_alloc_sem_slot(void)
{
    UINT i, old_posture;
    kvb_threadx_sem_slot_t *result = NULL;

    old_posture = tx_interrupt_control(TX_INT_DISABLE);
    for (i = 0u; i < KVB_THREADX_SEM_POOL_SIZE; ++i) {
        if (!s_sem_pool[i].used) {
            s_sem_pool[i].used = true;
            result = &s_sem_pool[i];
            break;
        }
    }
    (void)tx_interrupt_control(old_posture);
    return result;
}

static void kvb_threadx_free_sem_slot(kvb_threadx_sem_slot_t *slot)
{
    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
    slot->used = false;
    (void)tx_interrupt_control(old_posture);
}

static kvb_threadx_mutex_slot_t *kvb_threadx_alloc_mutex_slot(void)
{
    UINT i, old_posture;
    kvb_threadx_mutex_slot_t *result = NULL;

    old_posture = tx_interrupt_control(TX_INT_DISABLE);
    for (i = 0u; i < KVB_THREADX_MUTEX_POOL_SIZE; ++i) {
        if (!s_mutex_pool[i].used) {
            s_mutex_pool[i].used = true;
            result = &s_mutex_pool[i];
            break;
        }
    }
    (void)tx_interrupt_control(old_posture);
    return result;
}

static void kvb_threadx_free_mutex_slot(kvb_threadx_mutex_slot_t *slot)
{
    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
    slot->used = false;
    (void)tx_interrupt_control(old_posture);
}

static kvb_threadx_queue_slot_t *kvb_threadx_alloc_queue_slot(void)
{
    UINT i, old_posture;
    kvb_threadx_queue_slot_t *result = NULL;

    old_posture = tx_interrupt_control(TX_INT_DISABLE);
    for (i = 0u; i < KVB_THREADX_QUEUE_POOL_SIZE; ++i) {
        if (!s_queue_pool[i].used) {
            s_queue_pool[i].used = true;
            result = &s_queue_pool[i];
            break;
        }
    }
    (void)tx_interrupt_control(old_posture);
    return result;
}

static void kvb_threadx_free_queue_slot(kvb_threadx_queue_slot_t *slot)
{
    UINT old_posture = tx_interrupt_control(TX_INT_DISABLE);
    slot->used = false;
    (void)tx_interrupt_control(old_posture);
}

/* -------------------------------------------------------------------------- *
 * Priority and timeout mappings
 * -------------------------------------------------------------------------- */

static UINT kvb_threadx_priority(KvbPriority p)
{
    /* TX_MAX_PRIORITIES - 1 is the lowest valid priority.  We reserve
     * priority (TX_MAX_PRIORITIES - 1) for any "idle-like" caller and
     * map KVB_PRIORITY_LOWEST to one above it, so KVB callers never
     * trample priority slot 0 or TX_MAX_PRIORITIES-1. */
    const UINT max = (UINT)TX_MAX_PRIORITIES;
    UINT v;

    if (max < 5u) {
        return (p == KVB_PRIORITY_HIGHEST || p == KVB_PRIORITY_HIGH) ? 0u : (max - 1u);
    }

    switch (p) {
    case KVB_PRIORITY_HIGHEST: v = 1u; break;
    case KVB_PRIORITY_HIGH:    v = 1u + (max - 4u) / 4u; break;
    case KVB_PRIORITY_NORMAL:  v = max / 2u; break;
    case KVB_PRIORITY_LOW:     v = max - 2u - (max - 4u) / 4u; break;
    case KVB_PRIORITY_LOWEST:  v = max - 2u; break;
    default:                   v = max / 2u; break;
    }

    if (v >= max - 1u) {
        v = max - 2u;
    }
    if (v == 0u) {
        v = 1u;
    }
    return v;
}

static ULONG kvb_threadx_timeout(uint32_t timeout_ticks)
{
    if (timeout_ticks == KVB_WAIT_FOREVER) {
        return TX_WAIT_FOREVER;
    }
    if (timeout_ticks == KVB_NO_WAIT) {
        return TX_NO_WAIT;
    }
    return (ULONG)timeout_ticks;
}

static KvbStatus kvb_threadx_status(UINT rc, uint32_t timeout_ticks)
{
    switch (rc) {
    case TX_SUCCESS:
        return KVB_OK;
    case TX_NO_INSTANCE:
    case TX_QUEUE_EMPTY:
    case TX_QUEUE_FULL:
        return (timeout_ticks == KVB_NO_WAIT) ? KVB_ERR_WOULD_BLOCK : KVB_ERR_TIMEOUT;
    case TX_NOT_AVAILABLE:
        return (timeout_ticks == KVB_NO_WAIT) ? KVB_ERR_WOULD_BLOCK : KVB_ERR_TIMEOUT;
    case TX_NOT_OWNED:
        return KVB_ERR_NOT_OWNER;
    case TX_PTR_ERROR:
    case TX_THREAD_ERROR:
    case TX_PRIORITY_ERROR:
    case TX_SIZE_ERROR:
    case TX_QUEUE_ERROR:
    case TX_SEMAPHORE_ERROR:
    case TX_MUTEX_ERROR:
        return KVB_ERR_INVALID_ARG;
    case TX_CALLER_ERROR:
        return KVB_ERR_KERNEL;
    default:
        return KVB_ERR_KERNEL;
    }
}

/* -------------------------------------------------------------------------- *
 * Thread entry trampolines
 * -------------------------------------------------------------------------- */

static void kvb_threadx_real_entry(ULONG entry_input)
{
    kvb_threadx_thread_slot_t *slot =
        (kvb_threadx_thread_slot_t *)(uintptr_t)entry_input;
    if (slot != NULL && slot->entry != NULL) {
        slot->entry(slot->arg);
    }
    tx_thread_relinquish();
    for (;;) {
        tx_thread_sleep(1000u);
    }
}

static void kvb_runner_entry(ULONG entry_input)
{
    (void)entry_input;
    if (s_runner_entry != NULL) {
        s_runner_entry(s_runner_arg);
    }
    for (;;) {
        tx_thread_sleep(1000u);
    }
}

/* -------------------------------------------------------------------------- *
 * tx_application_define — entry point from tx_kernel_enter
 * -------------------------------------------------------------------------- */

extern void kvb_threadx_install_stack_error_handler(void);

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    kvb_threadx_install_stack_error_handler();

    (void)tx_thread_create(&s_kvb_runner_tcb,
                           "kvb_runner",
                           kvb_runner_entry,
                           0u,
                           s_kvb_runner_stack,
                           sizeof(s_kvb_runner_stack),
                           kvb_threadx_priority(KVB_PRIORITY_HIGHEST),
                           kvb_threadx_priority(KVB_PRIORITY_HIGHEST),
                           TX_NO_TIME_SLICE,
                           TX_AUTO_START);
}

/* -------------------------------------------------------------------------- *
 * Kernel features advertisement
 * -------------------------------------------------------------------------- */

static const KvbKernelFeatures g_threadx_features = {
    .kernel_name                          = "ThreadX",
    .kernel_version                       = "Eclipse ThreadX 6.x",
    .supports_dynamic_threads             = true,
    .supports_thread_delete               = true,
    .supports_semaphore                   = true,
    .supports_mutex                       = true,
    .supports_mutex_timeout               = true,
    .supports_mutex_priority_inheritance  = true,
    .supports_mutex_priority_protect      = false,
    .supports_recursive_mutex             = true,
    .supports_queue                       = true,
    .supports_timer                       = false,
    .supports_fixed_pool                  = true,
    .supports_general_allocator           = false,
    .supports_isr_kernel_api              = true,
    .supports_stack_overflow_detection    = true,
    .validates_null_args                  = true,
    .validates_invalid_objects            = true
};

const KvbKernelFeatures *kvb_kernel_features(void)
{
    return &g_threadx_features;
}

KvbStatus kvb_kernel_start(KvbRunnerEntry runner, void *arg)
{
    if (runner == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    s_runner_entry = runner;
    s_runner_arg   = arg;

    tx_kernel_enter();      /* never returns */
    return KVB_ERR_KERNEL;  /* unreachable */
}

/* -------------------------------------------------------------------------- *
 * Threads
 * -------------------------------------------------------------------------- */

KvbStatus kvb_thread_create(
    KvbThread     *thread,
    const char    *name,
    KvbThreadEntry entry,
    void          *arg,
    void          *stack_mem,
    size_t         stack_size,
    KvbPriority    priority)
{
    UINT rc;
    UINT pri;
    kvb_threadx_thread_slot_t *slot;

    if (thread == NULL || entry == NULL || stack_mem == NULL || stack_size == 0u) {
        return KVB_ERR_INVALID_ARG;
    }
    if (name == NULL) {
        name = "kvb_thread";
    }

    slot = kvb_threadx_alloc_thread_slot();
    if (slot == NULL) {
        return KVB_ERR_KERNEL;
    }
    slot->entry = entry;
    slot->arg   = arg;

    pri = kvb_threadx_priority(priority);
    rc = tx_thread_create(&slot->tcb,
                          (CHAR *)name,
                          kvb_threadx_real_entry,
                          (ULONG)(uintptr_t)slot,
                          stack_mem,
                          (ULONG)stack_size,
                          pri,
                          pri,
                          TX_NO_TIME_SLICE,
                          TX_AUTO_START);
    if (rc != TX_SUCCESS) {
        kvb_threadx_free_thread_slot(slot);
        thread->internal = NULL;
        return kvb_threadx_status(rc, 0u);
    }

    thread->internal = slot;
    return KVB_OK;
}

KvbStatus kvb_thread_yield(void)
{
    tx_thread_relinquish();
    return KVB_OK;
}

uint64_t kvb_kernel_tick_count(void)
{
    return (uint64_t)tx_time_get();
}

KvbStatus kvb_thread_sleep_ticks(uint32_t ticks)
{
    UINT rc = tx_thread_sleep((ULONG)ticks);
    return (rc == TX_SUCCESS) ? KVB_OK : KVB_ERR_KERNEL;
}

KvbStatus kvb_thread_delete(KvbThread *thread)
{
    UINT rc;
    kvb_threadx_thread_slot_t *slot;

    if (thread == NULL || thread->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_thread_slot_t *)thread->internal;

    /* Terminate first — tx_thread_delete refuses non-terminated threads. */
    (void)tx_thread_terminate(&slot->tcb);
    rc = tx_thread_delete(&slot->tcb);

    /* Always release the slot, even on partial failure — the caller has
     * already lost the handle, and leaking the slot would cause
     * pool exhaustion across repeated test runs. */
    kvb_threadx_free_thread_slot(slot);
    thread->internal = NULL;

    return (rc == TX_SUCCESS) ? KVB_OK : KVB_ERR_KERNEL;
}

/* -------------------------------------------------------------------------- *
 * Semaphores
 * -------------------------------------------------------------------------- */

KvbStatus kvb_sem_create(KvbSemaphore *sem, uint32_t initial_count, uint32_t max_count)
{
    UINT rc;
    kvb_threadx_sem_slot_t *slot;

    (void)max_count;
    if (sem == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    if (max_count != 0u && initial_count > max_count) {
        return KVB_ERR_INVALID_ARG;
    }

    slot = kvb_threadx_alloc_sem_slot();
    if (slot == NULL) {
        return KVB_ERR_KERNEL;
    }

    rc = tx_semaphore_create(&slot->sem, "kvb_sem", (ULONG)initial_count);
    if (rc != TX_SUCCESS) {
        kvb_threadx_free_sem_slot(slot);
        sem->internal = NULL;
        return kvb_threadx_status(rc, 0u);
    }

    sem->internal = slot;
    return KVB_OK;
}

KvbStatus kvb_sem_wait(KvbSemaphore *sem, uint32_t timeout_ticks)
{
    UINT rc;
    kvb_threadx_sem_slot_t *slot;

    if (sem == NULL || sem->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_sem_slot_t *)sem->internal;

    rc = tx_semaphore_get(&slot->sem, kvb_threadx_timeout(timeout_ticks));
    return kvb_threadx_status(rc, timeout_ticks);
}

KvbStatus kvb_sem_post(KvbSemaphore *sem)
{
    UINT rc;
    kvb_threadx_sem_slot_t *slot;

    if (sem == NULL || sem->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_sem_slot_t *)sem->internal;

    rc = tx_semaphore_put(&slot->sem);
    return (rc == TX_SUCCESS) ? KVB_OK : kvb_threadx_status(rc, 0u);
}

KvbStatus kvb_sem_delete(KvbSemaphore *sem)
{
    UINT rc;
    kvb_threadx_sem_slot_t *slot;

    if (sem == NULL || sem->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_sem_slot_t *)sem->internal;

    rc = tx_semaphore_delete(&slot->sem);
    kvb_threadx_free_sem_slot(slot);
    sem->internal = NULL;

    return (rc == TX_SUCCESS) ? KVB_OK : KVB_ERR_KERNEL;
}

/* -------------------------------------------------------------------------- *
 * Mutexes
 * -------------------------------------------------------------------------- */

KvbStatus kvb_mutex_create(KvbMutex *mutex, uint32_t flags)
{
    UINT rc;
    kvb_threadx_mutex_slot_t *slot;

    if (mutex == NULL) {
        return KVB_ERR_INVALID_ARG;
    }

    slot = kvb_threadx_alloc_mutex_slot();
    if (slot == NULL) {
        return KVB_ERR_KERNEL;
    }
    slot->recursive = ((flags & KVB_MUTEX_RECURSIVE) != 0u);

    /* ThreadX mutexes are recursive intrinsically; the recursive flag is
     * accepted unconditionally.  Priority inheritance is enabled
     * unconditionally to match KVB's mutex semantics — PI is a property
     * of "mutex" in KVB (advertised via supports_mutex_priority_inheritance),
     * not a per-call flag.  Same shape as the FreeRTOS port. */
    rc = tx_mutex_create(&slot->mtx, "kvb_mutex", TX_INHERIT);
    if (rc != TX_SUCCESS) {
        kvb_threadx_free_mutex_slot(slot);
        mutex->internal = NULL;
        return kvb_threadx_status(rc, 0u);
    }

    mutex->internal = slot;
    return KVB_OK;
}

KvbStatus kvb_mutex_create_protect(KvbMutex *mutex, KvbPriority ceiling)
{
    /* Eclipse ThreadX implements priority inheritance via TX_INHERIT only.
     * It has no per-mutex ceiling field equivalent to POSIX PRIO_PROTECT. */
    (void)ceiling;
    if (mutex == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    return KVB_ERR_UNSUPPORTED;
}

KvbStatus kvb_mutex_lock(KvbMutex *mutex, uint32_t timeout_ticks)
{
    UINT rc;
    kvb_threadx_mutex_slot_t *slot;

    if (mutex == NULL || mutex->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_mutex_slot_t *)mutex->internal;

    rc = tx_mutex_get(&slot->mtx, kvb_threadx_timeout(timeout_ticks));
    return kvb_threadx_status(rc, timeout_ticks);
}

KvbStatus kvb_mutex_unlock(KvbMutex *mutex)
{
    UINT rc;
    kvb_threadx_mutex_slot_t *slot;

    if (mutex == NULL || mutex->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_mutex_slot_t *)mutex->internal;

    /* No ownership pre-check needed; tx_mutex_put returns TX_NOT_OWNED
     * cleanly when called from a non-owner.  Compare with the FreeRTOS
     * port which has to pre-check at ~30 cycles per call to avoid
     * a configASSERT halt. */
    rc = tx_mutex_put(&slot->mtx);
    return kvb_threadx_status(rc, 0u);
}

KvbStatus kvb_mutex_delete(KvbMutex *mutex)
{
    UINT rc;
    kvb_threadx_mutex_slot_t *slot;

    if (mutex == NULL || mutex->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_mutex_slot_t *)mutex->internal;

    rc = tx_mutex_delete(&slot->mtx);
    kvb_threadx_free_mutex_slot(slot);
    mutex->internal = NULL;

    return (rc == TX_SUCCESS) ? KVB_OK : KVB_ERR_KERNEL;
}

/* -------------------------------------------------------------------------- *
 * Queues
 * -------------------------------------------------------------------------- */

KvbStatus kvb_queue_create(KvbQueue *queue, void *storage, size_t message_size, uint32_t message_count)
{
    UINT rc;
    UINT msg_words;
    kvb_threadx_queue_slot_t *slot;

    if (queue == NULL || storage == NULL || message_size == 0u || message_count == 0u) {
        return KVB_ERR_INVALID_ARG;
    }

    /* ThreadX message size is in 32-bit words; valid values are 1, 2, 4,
     * 8, 16.  KVB expresses message size in bytes; convert and validate. */
    if ((message_size & 3u) != 0u) {
        return KVB_ERR_INVALID_ARG;
    }
    msg_words = (UINT)(message_size / 4u);
    if (msg_words != 1u && msg_words != 2u && msg_words != 4u &&
        msg_words != 8u && msg_words != 16u) {
        return KVB_ERR_INVALID_ARG;
    }

    slot = kvb_threadx_alloc_queue_slot();
    if (slot == NULL) {
        return KVB_ERR_KERNEL;
    }

    rc = tx_queue_create(&slot->q, "kvb_queue",
                         msg_words,
                         storage,
                         (ULONG)(message_size * message_count));
    if (rc != TX_SUCCESS) {
        kvb_threadx_free_queue_slot(slot);
        queue->internal = NULL;
        return kvb_threadx_status(rc, 0u);
    }

    queue->internal = slot;
    return KVB_OK;
}

KvbStatus kvb_queue_send(KvbQueue *queue, const void *msg, uint32_t timeout_ticks)
{
    UINT rc;
    kvb_threadx_queue_slot_t *slot;

    if (queue == NULL || queue->internal == NULL || msg == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_queue_slot_t *)queue->internal;

    rc = tx_queue_send(&slot->q, (VOID *)msg, kvb_threadx_timeout(timeout_ticks));
    return kvb_threadx_status(rc, timeout_ticks);
}

KvbStatus kvb_queue_recv(KvbQueue *queue, void *msg, uint32_t timeout_ticks)
{
    UINT rc;
    kvb_threadx_queue_slot_t *slot;

    if (queue == NULL || queue->internal == NULL || msg == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_queue_slot_t *)queue->internal;

    rc = tx_queue_receive(&slot->q, msg, kvb_threadx_timeout(timeout_ticks));
    return kvb_threadx_status(rc, timeout_ticks);
}

KvbStatus kvb_queue_delete(KvbQueue *queue)
{
    UINT rc;
    kvb_threadx_queue_slot_t *slot;

    if (queue == NULL || queue->internal == NULL) {
        return KVB_ERR_INVALID_ARG;
    }
    slot = (kvb_threadx_queue_slot_t *)queue->internal;

    rc = tx_queue_delete(&slot->q);
    kvb_threadx_free_queue_slot(slot);
    queue->internal = NULL;

    return (rc == TX_SUCCESS) ? KVB_OK : KVB_ERR_KERNEL;
}

/* -------------------------------------------------------------------------- *
 * Default starter — used by the per-MCU main.cpp if no custom entry needed
 * -------------------------------------------------------------------------- */

int kvb_threadx_start_default(void)
{
    KvbStatus st = kvb_kernel_start(kvb_runner_main, 0);
    return (st == KVB_OK) ? 0 : -1;
}
