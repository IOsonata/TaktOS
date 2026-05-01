/**-------------------------------------------------------------------------
 * @file    kvb_threadx_types.h
 *
 * @brief   ThreadX-port-private type definitions for KVB.
 *
 * v13 redesign — opaque-handle pattern.
 *
 *    Earlier revisions of this header carried the kernel control blocks
 *    (TX_THREAD, TX_SEMAPHORE, TX_MUTEX, TX_QUEUE) inline in the public
 *    KvbThread / KvbSemaphore / KvbMutex / KvbQueue wrappers.  This is the
 *    same shape the FreeRTOS and TaktOS ports use, and it works fine for
 *    those kernels because their static-allocation control blocks are
 *    plain structs that the kernel never threads onto a global linked
 *    list.
 *
 *    ThreadX is different.  Every tx_*_create call hangs the supplied
 *    control block onto multiple kernel-global lists:
 *
 *        _tx_thread_priority_list[]   (per-priority ready queue)
 *        _tx_thread_created_ptr        (global thread registry)
 *        _tx_semaphore_created_ptr     (global semaphore registry)
 *        ... and so on
 *
 *    The control block address is stored in those lists for the entire
 *    lifetime of the object, even after suspend/terminate.  If the
 *    application places the wrapper on the stack (KvbThread threads[3]
 *    inside a test runner function), then on the FIRST tx_thread_create
 *    the kernel writes the stack address into _tx_thread_priority_list[].
 *    As soon as the test function's stack frame churns — even once,
 *    even slightly — those globals are pointing at stale stack memory
 *    that now contains return addresses, locals, etc.  The next kernel
 *    call walks the priority list, dereferences the pointer, and reads
 *    a Thumb function pointer instead of a TX_THREAD struct.  Hard
 *    fault.  Symptom observed:  _tx_thread_priority_list[16] ==
 *    0x0800233f  (a return-address LR value pushed onto the runner
 *    stack at the slot where KvbThread.tcb used to live).
 *
 *    The fix is the same pattern Microsoft's own Thread-Metric ThreadX
 *    port uses (Benchmark/ThreadMetric/src/tm_port_threadx.c):
 *    keep a port-private static array of TX_THREAD / TX_SEMAPHORE /
 *    TX_MUTEX / TX_QUEUE control blocks, allocate a slot from the
 *    array on create, free the slot on delete.  The user-facing
 *    KvbThread / etc. structs hold ONLY a pointer to the port-private
 *    slot — they are 4-byte handles that are safe to place anywhere,
 *    including on the stack.
 *
 *    Memory cost on STM32F0308 (M0, 8 KB SRAM):
 *      8 × TX_THREAD     ≈ 1120 B   (was 1120 B on caller stacks anyway)
 *      8 × TX_SEMAPHORE  ≈  320 B   (was on caller stacks)
 *      4 × TX_MUTEX      ≈  208 B   (was on caller stacks)
 *      4 × TX_QUEUE      ≈  256 B   (was on caller stacks)
 *      Tracking bytes    ≈   24 B
 *      ----------------------------
 *      Total static      ≈ 1928 B
 *
 *    Net: roughly equal total RAM use.  The bytes move from caller
 *    stack frames to a port-private BSS pool, which is exactly what
 *    ThreadX needs for correctness.
 * -------------------------------------------------------------------------*/
#ifndef KVB_THREADX_TYPES_H
#define KVB_THREADX_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef KVB_THREADX_ALIGN
#define KVB_THREADX_ALIGN 8u
#endif

/* User-facing wrappers are opaque handles — a single pointer to a
 * port-private control block.  Safe to place on the stack, in BSS,
 * or anywhere else; the kernel never sees the wrapper address, only
 * the address of the port-private slot. */

typedef struct KvbThread {
    void *internal;
} KvbThread;

typedef struct KvbSemaphore {
    void *internal;
} KvbSemaphore;

typedef struct KvbMutex {
    void *internal;
} KvbMutex;

typedef struct KvbQueue {
    void *internal;
} KvbQueue;

typedef struct KvbTimer {
    void *internal;
} KvbTimer;

/* Stack buffer macros — unchanged from earlier revisions.  KVB
 * still requires the user to supply the worker thread stacks
 * (no benefit to pooling those — they are large and per-test). */

#define KVB_THREAD_BUF_SIZE(usable_bytes) (usable_bytes)

#define KVB_THREADX_STACK_BYTES_ALIGNED(usable_bytes) \
    (((usable_bytes) + KVB_THREADX_ALIGN - 1u) & ~((size_t)KVB_THREADX_ALIGN - 1u))

#define KVB_THREAD_STACK_DEFINE(name, usable_bytes) \
    uint8_t name[KVB_THREADX_STACK_BYTES_ALIGNED(usable_bytes)] \
        __attribute__((aligned(KVB_THREADX_ALIGN)))
#define KVB_THREAD_STACK_ARRAY_DEFINE(name, count, usable_bytes) \
    uint8_t name[(count)][KVB_THREADX_STACK_BYTES_ALIGNED(usable_bytes)] \
        __attribute__((aligned(KVB_THREADX_ALIGN)))

#define KVB_THREAD_STACK_MEM(name) ((void *)(name))
#define KVB_THREAD_STACK_SIZE(name) ((size_t)sizeof(name))
#define KVB_THREAD_STACK_ARRAY_MEM(name, index) ((void *)((name)[(index)]))
#define KVB_THREAD_STACK_ARRAY_SIZE(name, index) ((size_t)sizeof((name)[(index)]))

/* Queue storage stays caller-supplied — the message ring buffer can be
 * arbitrarily large and is per-test, so the same reasoning as worker
 * stacks applies (no benefit to pooling, large per-instance). */
#define KVB_QUEUE_STORAGE_DEFINE(name, message_size, message_count) \
    uint8_t name[(message_size) * (message_count)] \
        __attribute__((aligned(KVB_THREADX_ALIGN)))
#define KVB_QUEUE_STORAGE_MEM(name) ((void *)(name))

#ifdef __cplusplus
}
#endif

#endif /* KVB_THREADX_TYPES_H */
