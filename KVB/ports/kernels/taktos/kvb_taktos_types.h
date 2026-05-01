#ifndef KVB_TAKTOS_TYPES_H
#define KVB_TAKTOS_TYPES_H

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KvbThread {
    hTaktOSThread_t handle;
} KvbThread;

typedef struct KvbSemaphore {
    TaktOSSem_t sem;
} KvbSemaphore;

typedef struct KvbMutex {
    TaktOSMutex_t mutex;
} KvbMutex;

typedef struct KvbQueue {
    TaktOSQueue_t queue;
} KvbQueue;

typedef struct KvbTimer {
    void *reserved;
} KvbTimer;

/* TaktOS lays the TCB, guard region, top alignment, and initial frame inside
   the caller-provided thread block.  TAKTOS_THREAD_MEM_SIZE(usable) returns
   the minimum memory needed to obtain `usable` bytes of usable stack.

   Two alignment rules apply, both rooted in the architecture's worst-case
   constraint TAKTOS_STACK_GUARD_ALIGN (256 bytes on ARMv6-M, 32 bytes on
   ARMv7-M / ARMv8-M, 8 bytes on RISC-V):

     1. Each individual thread block must START at an address that is a
        multiple of TAKTOS_STACK_GUARD_ALIGN.  Otherwise pStackBottom
        rounding inside TaktOSThreadCreate may push the stack floor past
        the stack top, and on M0 the very first 32-bit member write into
        the TCB hard-faults at any non-4-aligned address.

     2. When tests allocate an ARRAY of thread blocks, each element's
        size must also be a multiple of TAKTOS_STACK_GUARD_ALIGN so that
        every row stays aligned, not just row 0.  TAKTOS_THREAD_MEM_SIZE
        returns the minimum, which is not generally a multiple of the
        guard alignment.  KVB_THREAD_BUF_SIZE rounds it up.

   Tests must always size their per-thread buffers via KVB_THREAD_BUF_SIZE
   and declare them via KVB_THREAD_STACK_DEFINE / KVB_THREAD_STACK_ARRAY_DEFINE
   so the usable stack is identical across all kernel ports AND every
   element is correctly aligned for the active TaktOS arch. */

/* Round x UP to the next multiple of align (align must be a power of two). */
#define KVB_TAKTOS_ROUND_UP(x, align)  (((x) + ((align) - 1u)) & ~((align) - 1u))

#define KVB_THREAD_BUF_SIZE(usable_bytes) \
    KVB_TAKTOS_ROUND_UP(TAKTOS_THREAD_MEM_SIZE(usable_bytes), TAKTOS_STACK_GUARD_ALIGN)

#if defined(__GNUC__)
#define KVB_TAKTOS_THREAD_ALIGNED __attribute__((aligned(TAKTOS_STACK_GUARD_ALIGN)))
#else
#define KVB_TAKTOS_THREAD_ALIGNED
#endif

#define KVB_THREAD_STACK_DEFINE(name, usable_bytes) \
    uint8_t name[KVB_THREAD_BUF_SIZE(usable_bytes)] KVB_TAKTOS_THREAD_ALIGNED
#define KVB_THREAD_STACK_ARRAY_DEFINE(name, count, usable_bytes) \
    uint8_t name[(count)][KVB_THREAD_BUF_SIZE(usable_bytes)] KVB_TAKTOS_THREAD_ALIGNED
#define KVB_THREAD_STACK_MEM(name) ((void *)(name))
#define KVB_THREAD_STACK_SIZE(name) ((size_t)sizeof(name))
#define KVB_THREAD_STACK_ARRAY_MEM(name, index) ((void *)((name)[(index)]))
#define KVB_THREAD_STACK_ARRAY_SIZE(name, index) ((size_t)sizeof((name)[(index)]))
#define KVB_QUEUE_STORAGE_DEFINE(name, message_size, message_count) \
    uint32_t name[(((message_size) * (message_count)) + sizeof(uint32_t) - 1u) / sizeof(uint32_t)]
#define KVB_QUEUE_STORAGE_MEM(name) ((void *)(name))

#ifdef __cplusplus
}
#endif

#endif /* KVB_TAKTOS_TYPES_H */
