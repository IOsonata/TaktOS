#ifndef KVB_ZEPHYR_TYPES_H
#define KVB_ZEPHYR_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KvbThread {
    struct k_thread thread;
    k_tid_t tid;
    void (*entry)(void *arg);
    void *arg;
} KvbThread;

typedef struct KvbSemaphore {
    struct k_sem sem;
} KvbSemaphore;

typedef struct KvbMutex {
    struct k_mutex mutex;
} KvbMutex;

typedef struct KvbQueue {
    struct k_msgq msgq;
} KvbQueue;

typedef struct KvbTimer {
    void *reserved;
} KvbTimer;

/* Zephyr stores struct k_thread outside the stack region (it lives in
   KvbThread).  The KVB-supplied buffer is therefore pure stack.  Note that
   Zephyr ordinarily expects stack arrays declared with K_THREAD_STACK_DEFINE
   for proper alignment and userspace handling; KVB tests run in supervisor
   mode and rely on the platform's natural alignment. */
#define KVB_THREAD_BUF_SIZE(usable_bytes) (usable_bytes)
#define KVB_THREAD_STACK_DEFINE(name, usable_bytes) \
    K_THREAD_STACK_DEFINE(name, usable_bytes)
#define KVB_THREAD_STACK_ARRAY_DEFINE(name, count, usable_bytes) \
    K_THREAD_STACK_ARRAY_DEFINE(name, count, usable_bytes)
#define KVB_THREAD_STACK_MEM(name) ((void *)(name))
#define KVB_THREAD_STACK_SIZE(name) ((size_t)K_THREAD_STACK_SIZEOF(name))
#define KVB_THREAD_STACK_ARRAY_MEM(name, index) ((void *)((name)[(index)]))
#define KVB_THREAD_STACK_ARRAY_SIZE(name, index) ((size_t)K_THREAD_STACK_SIZEOF((name)[(index)]))
#define KVB_QUEUE_STORAGE_DEFINE(name, message_size, message_count) \
    char name[(message_size) * (message_count)]
#define KVB_QUEUE_STORAGE_MEM(name) ((void *)(name))

#ifdef __cplusplus
}
#endif

#endif /* KVB_ZEPHYR_TYPES_H */
