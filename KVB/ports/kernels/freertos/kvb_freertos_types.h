#ifndef KVB_FREERTOS_TYPES_H
#define KVB_FREERTOS_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- *
 * Alignment notes
 *
 *  FreeRTOS requires every static buffer it consumes (StaticTask_t,
 *  StaticSemaphore_t, StaticQueue_t, StackType_t[]) to be aligned to
 *  portBYTE_ALIGNMENT, which on Cortex-M is 8 bytes.
 *
 *  The compiler picks struct alignment from the strictest member.
 *  StaticTask_t / StaticSemaphore_t / StaticQueue_t are declared by
 *  FreeRTOS as opaque blobs holding only `uint32_t`, `void *` and
 *  similar fields — none of which require more than 4-byte alignment
 *  on a 32-bit ARM target.  As a result, plain `KvbThread` and friends
 *  end up only 4-byte aligned, and an array of them lands on a
 *  4-byte-but-not-8-byte boundary roughly half the time.
 *
 *  The Cortex-M0 (ARMv6-M) base ISA hard-faults on misaligned word
 *  accesses, so feeding such a buffer to xTaskCreateStatic / friends
 *  produces a HardFault inside the kernel as soon as it does its
 *  first 64-bit-aligned manipulation of the buffer.  This matches
 *  FreeRTOS bug #144 ("pxPortInitialiseStack wrong behavior with
 *  configSUPPORT_STATIC_ALLOCATION causes cortex-m0 to go into
 *  HardFault when creating the second task") and the wider
 *  "static buffer alignment" guidance from the FreeRTOS support
 *  forum.
 *
 *  The fix is to attach __attribute__((aligned(portBYTE_ALIGNMENT)))
 *  to every KVB type that wraps a FreeRTOS static buffer, and to the
 *  stack-buffer array macros.  This survives nesting in a struct,
 *  embedding in another array, and being placed on a function's
 *  local stack frame — the compiler propagates the alignment
 *  requirement out to every container.
 * -------------------------------------------------------------------------- */

typedef struct KvbThread {
    TaskHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticTask_t tcb;
#endif
} __attribute__((aligned(portBYTE_ALIGNMENT))) KvbThread;

typedef struct KvbSemaphore {
    SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticSemaphore_t storage;
#endif
} __attribute__((aligned(portBYTE_ALIGNMENT))) KvbSemaphore;

typedef struct KvbMutex {
    SemaphoreHandle_t handle;
    bool recursive;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticSemaphore_t storage;
#endif
} __attribute__((aligned(portBYTE_ALIGNMENT))) KvbMutex;

typedef struct KvbQueue {
    QueueHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticQueue_t storage;
#endif
} __attribute__((aligned(portBYTE_ALIGNMENT))) KvbQueue;

typedef struct KvbTimer {
    void *reserved;
} KvbTimer;

/* FreeRTOS keeps the TCB outside the stack buffer (StaticTask_t lives in
   KvbThread).  The KVB-supplied buffer is therefore pure stack. */
#define KVB_THREAD_BUF_SIZE(usable_bytes) (usable_bytes)

/* Round up the requested usable byte count to a multiple of
 * portBYTE_ALIGNMENT BEFORE dividing into StackType_t words.  Combined
 * with the aligned attribute on the array base below, this makes every
 * row of an N-thread stack array land on an 8-byte boundary on Cortex-M,
 * which FreeRTOS requires for its 64-bit-aligned operations on the
 * stack interior. */
#define KVB_FREERTOS_STACK_BYTES_ALIGNED(usable_bytes) \
    (((usable_bytes) + (size_t)portBYTE_ALIGNMENT - 1u) & ~((size_t)portBYTE_ALIGNMENT - 1u))
#define KVB_FREERTOS_STACK_WORDS(usable_bytes) \
    (KVB_FREERTOS_STACK_BYTES_ALIGNED(usable_bytes) / sizeof(StackType_t))

#define KVB_THREAD_STACK_DEFINE(name, usable_bytes) \
    StackType_t name[KVB_FREERTOS_STACK_WORDS(usable_bytes)] \
        __attribute__((aligned(portBYTE_ALIGNMENT)))
#define KVB_THREAD_STACK_ARRAY_DEFINE(name, count, usable_bytes) \
    StackType_t name[(count)][KVB_FREERTOS_STACK_WORDS(usable_bytes)] \
        __attribute__((aligned(portBYTE_ALIGNMENT)))

#define KVB_THREAD_STACK_MEM(name) ((void *)(name))
#define KVB_THREAD_STACK_SIZE(name) ((size_t)sizeof(name))
#define KVB_THREAD_STACK_ARRAY_MEM(name, index) ((void *)((name)[(index)]))
#define KVB_THREAD_STACK_ARRAY_SIZE(name, index) ((size_t)sizeof((name)[(index)]))

#define KVB_QUEUE_STORAGE_DEFINE(name, message_size, message_count) \
    uint8_t name[(message_size) * (message_count)] \
        __attribute__((aligned(portBYTE_ALIGNMENT)))
#define KVB_QUEUE_STORAGE_MEM(name) ((void *)(name))

#ifdef __cplusplus
}
#endif

#endif /* KVB_FREERTOS_TYPES_H */
