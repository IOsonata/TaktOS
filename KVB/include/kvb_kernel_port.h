#ifndef KVB_KERNEL_PORT_H
#define KVB_KERNEL_PORT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "kvb_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KVB_OK = 0,
    KVB_ERR_TIMEOUT,
    KVB_ERR_INVALID_ARG,
    KVB_ERR_NOT_OWNER,
    KVB_ERR_WOULD_BLOCK,
    KVB_ERR_UNSUPPORTED,
    KVB_ERR_KERNEL
} KvbStatus;

/* KVB defines five canonical priority levels.  Each kernel port maps these
   onto its own native priority space.  Tests must not assume any particular
   numeric value.  Phase 2 priority-inheritance tests use LOW/NORMAL/HIGH;
   the test runner itself runs at HIGHEST.  LOWEST stays above each kernel's
   idle/system thread. */
typedef enum {
    KVB_PRIORITY_LOWEST  = 0,
    KVB_PRIORITY_LOW     = 1,
    KVB_PRIORITY_NORMAL  = 2,
    KVB_PRIORITY_HIGH    = 3,
    KVB_PRIORITY_HIGHEST = 4,
    KVB_PRIORITY_COUNT   = 5
} KvbPriority;

#if defined(KVB_PORT_TAKTOS) && (KVB_PORT_TAKTOS != 0)
#include "kvb_taktos_types.h"
#elif defined(KVB_PORT_FREERTOS) && (KVB_PORT_FREERTOS != 0)
#include "kvb_freertos_types.h"
#elif defined(KVB_PORT_THREADX) && (KVB_PORT_THREADX != 0)
#include "kvb_threadx_types.h"
#elif defined(KVB_PORT_ZEPHYR) && (KVB_PORT_ZEPHYR != 0)
#include "kvb_zephyr_types.h"
#else
typedef struct KvbThread {
    void *handle;
} KvbThread;

typedef struct KvbSemaphore {
    uintptr_t storage[8];
} KvbSemaphore;

typedef struct KvbMutex {
    uintptr_t storage[8];
} KvbMutex;

typedef struct KvbQueue {
    uintptr_t storage[12];
} KvbQueue;

typedef struct KvbTimer {
    uintptr_t storage[8];
} KvbTimer;
#endif

/* KVB_THREAD_BUF_SIZE(usable_bytes) returns the buffer size a test must
   allocate to obtain at least usable_bytes of usable thread stack on the
   selected kernel.  Most kernels store the TCB outside the stack buffer;
   for those, this macro is the identity.  TaktOS stores the TCB inside the
   caller-provided block, so the TaktOS types header overrides this macro to
   add the per-architecture overhead.  Tests that allocate per-thread stacks
   should always size their buffers via this macro. */
#ifndef KVB_THREAD_BUF_SIZE
#define KVB_THREAD_BUF_SIZE(usable_bytes) (usable_bytes)
#endif

/* Kernel ports can override these declaration/access macros when the native
   kernel requires a particular stack type or alignment. Tests must use these
   macros instead of declaring raw uint8_t stack buffers. */
#ifndef KVB_THREAD_STACK_DEFINE
#define KVB_THREAD_STACK_DEFINE(name, usable_bytes) \
    uint8_t name[KVB_THREAD_BUF_SIZE(usable_bytes)]
#endif

#ifndef KVB_THREAD_STACK_ARRAY_DEFINE
#define KVB_THREAD_STACK_ARRAY_DEFINE(name, count, usable_bytes) \
    uint8_t name[(count)][KVB_THREAD_BUF_SIZE(usable_bytes)]
#endif

#ifndef KVB_THREAD_STACK_MEM
#define KVB_THREAD_STACK_MEM(name) ((void *)(name))
#endif

#ifndef KVB_THREAD_STACK_SIZE
#define KVB_THREAD_STACK_SIZE(name) ((size_t)sizeof(name))
#endif

#ifndef KVB_THREAD_STACK_ARRAY_MEM
#define KVB_THREAD_STACK_ARRAY_MEM(name, index) ((void *)((name)[(index)]))
#endif

#ifndef KVB_THREAD_STACK_ARRAY_SIZE
#define KVB_THREAD_STACK_ARRAY_SIZE(name, index) ((size_t)sizeof((name)[(index)]))
#endif

/* Queue backing storage also has native alignment requirements on some kernels. */
#ifndef KVB_QUEUE_STORAGE_DEFINE
#define KVB_QUEUE_STORAGE_DEFINE(name, message_size, message_count) \
    uint8_t name[(message_size) * (message_count)]
#endif

#ifndef KVB_QUEUE_STORAGE_MEM
#define KVB_QUEUE_STORAGE_MEM(name) ((void *)(name))
#endif

typedef struct {
    const char *kernel_name;
    const char *kernel_version;
    bool supports_dynamic_threads;
    bool supports_thread_delete;
    bool supports_semaphore;
    bool supports_mutex;
    bool supports_mutex_timeout;
    bool supports_mutex_priority_inheritance;
    bool supports_mutex_priority_protect;     /* IPCP / POSIX PRIO_PROTECT */
    bool supports_recursive_mutex;
    bool supports_queue;
    bool supports_timer;
    bool supports_fixed_pool;
    bool supports_general_allocator;
    bool supports_isr_kernel_api;
    bool supports_stack_overflow_detection;
    bool validates_null_args;
    bool validates_invalid_objects;
} KvbKernelFeatures;

typedef void (*KvbThreadEntry)(void *arg);
typedef void (*KvbRunnerEntry)(void *arg);

#define KVB_WAIT_FOREVER 0xFFFFFFFFu
#define KVB_NO_WAIT      0u

#define KVB_MUTEX_NORMAL      0x00000000u
#define KVB_MUTEX_RECURSIVE   0x00000001u
#define KVB_MUTEX_PI_REQUIRED 0x00000002u
#define KVB_MUTEX_PROTECT     0x00000004u   /* request IPCP / priority-ceiling mutex */

const KvbKernelFeatures *kvb_kernel_features(void);

KvbStatus kvb_kernel_start(KvbRunnerEntry runner, void *arg);

KvbStatus kvb_thread_create(
    KvbThread *thread,
    const char *name,
    KvbThreadEntry entry,
    void *arg,
    void *stack_mem,
    size_t stack_size,
    KvbPriority priority);

KvbStatus kvb_thread_yield(void);
uint64_t kvb_kernel_tick_count(void);
KvbStatus kvb_thread_sleep_ticks(uint32_t ticks);
KvbStatus kvb_thread_delete(KvbThread *thread);

KvbStatus kvb_sem_create(KvbSemaphore *sem, uint32_t initial_count, uint32_t max_count);
KvbStatus kvb_sem_wait(KvbSemaphore *sem, uint32_t timeout_ticks);
KvbStatus kvb_sem_post(KvbSemaphore *sem);
KvbStatus kvb_sem_delete(KvbSemaphore *sem);

KvbStatus kvb_mutex_create(KvbMutex *mutex, uint32_t flags);

/**
 * Create an IPCP / priority-ceiling mutex.
 *
 * Kernels that do not implement priority-ceiling mutexes return
 * KVB_ERR_UNSUPPORTED.  The advertised feature flag
 * KvbKernelFeatures.supports_mutex_priority_protect indicates whether this
 * call will succeed before the test attempts it.
 *
 * The ceiling is given in KvbPriority canonical units; each kernel port
 * maps it to the kernel's native priority space the same way thread
 * priorities are mapped.
 */
KvbStatus kvb_mutex_create_protect(KvbMutex *mutex, KvbPriority ceiling);

KvbStatus kvb_mutex_lock(KvbMutex *mutex, uint32_t timeout_ticks);
KvbStatus kvb_mutex_unlock(KvbMutex *mutex);
KvbStatus kvb_mutex_delete(KvbMutex *mutex);

KvbStatus kvb_queue_create(
    KvbQueue *queue,
    void *storage,
    size_t message_size,
    uint32_t message_count);

KvbStatus kvb_queue_send(KvbQueue *queue, const void *msg, uint32_t timeout_ticks);
KvbStatus kvb_queue_recv(KvbQueue *queue, void *msg, uint32_t timeout_ticks);
KvbStatus kvb_queue_delete(KvbQueue *queue);

#ifdef __cplusplus
}
#endif

#endif /* KVB_KERNEL_PORT_H */
