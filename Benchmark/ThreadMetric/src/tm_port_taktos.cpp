/**-------------------------------------------------------------------------
 * @file    tm_port_taktos.cpp
 *
 * @brief   Thread-Metric port for TaktOS (C++ API). Shared across every MCU.
 *
 * Mirror of tm_port_taktos.c, but uses TaktOS C++ objects with method
 * dispatch (g_tm_queue[id].Send(...), g_tm_sem[id].Take(...), ...)
 * instead of the C function-call API. Generates a separate binary with
 * its own set of numbers  the C++ method-dispatch path has slightly
 * different inlining characteristics than the C function-call path,
 * so both are worth publishing.
 *
 * Per-MCU board.h supplies TM_CORE_CLOCK_HZ, TM_SW_IRQn, and the
 * arch-specific implementations of TmCauseInterrupt / TmSetKernelPriorities
 * / TmEnableSoftwareInterrupt. This file contains no architecture-specific
 * code  it builds for ARM Cortex-M and RISC-V RV32 alike.
 * -------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#include "tm_api.h"
#include "tm_port_common.h"
#include "board.h"           /* arch-encapsulating: TmCauseInterrupt, etc. */

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"

/* ----- TM (1high..31low) -> TaktOS (32high..1low) priority -------------- */
static inline uint8_t tm_to_taktos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (uint8_t)(32 - tm_priority);
}

/* ----- Storage --------------------------------------------------------- */
#pragma pack(push,4)
typedef struct {
    bool          allocated;
    bool          resume_before_start;
    bool          materialized;
    uint8_t       tm_priority;
    void        (*entry_function)(void);
    TaktOSThread  thread;
    uint8_t       mem[TAKTOS_THREAD_MEM_SIZE(TM_PORT_STACK_BYTES)]
                      __attribute__((aligned(4)));
} TmThreadDesc_t;
#pragma pack(pop)

static TmThreadDesc_t g_tm_thread[TM_PORT_MAX_THREADS];
static bool           g_kernel_started = false;

static TaktOSQueue    g_tm_queue        [TM_PORT_MAX_QUEUES];
static uint8_t        g_tm_queue_storage[TM_PORT_MAX_QUEUES]
                                        [TM_PORT_QUEUE_DEPTH * TM_PORT_QUEUE_MSG_BYTES]
                                        __attribute__((aligned(4)));

static TaktOSSem      g_tm_sem  [TM_PORT_MAX_SEMAPHORES];
static TaktOSMutex    g_tm_mutex[TM_PORT_MAX_MUTEXES];

static uint8_t        g_tm_pool_area[TM_PORT_MAX_POOLS][TM_PORT_POOL_BYTES]
                                    __attribute__((aligned(sizeof(void*))));
static void          *g_tm_pool_free[TM_PORT_MAX_POOLS];

/* ----- ISR weak hooks (overridden by InterruptProcessing tests) -------- */
extern "C" void tm_interrupt_handler(void)            __attribute__((weak));
extern "C" void tm_interrupt_preemption_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_handler(void)            {}
extern "C" void tm_interrupt_preemption_handler(void) {}

/* ----- Helpers --------------------------------------------------------- */
static void tm_thread_trampoline(void *param)
{
    int thread_id = (int)(uintptr_t)param;
    if (g_tm_thread[thread_id].entry_function)
        g_tm_thread[thread_id].entry_function();
    for (;;) {}
}

static int tm_materialize_thread(int thread_id)
{
    TmThreadDesc_t *t = &g_tm_thread[thread_id];
    if (t->materialized) return TM_SUCCESS;
    if (t->thread.Create(t->mem, (uint32_t)sizeof(t->mem),
                         tm_thread_trampoline, (void*)(uintptr_t)thread_id,
                         tm_to_taktos_priority(t->tm_priority)) == NULL)
        return TM_ERROR;
    t->materialized = true;
    return TM_SUCCESS;
}

/* ----- Init ------------------------------------------------------------ */
extern "C" void tm_initialize(void (*test_initialization_function)(void))
{
    for (int i = 0; i < TM_PORT_MAX_THREADS; ++i) {
        g_tm_thread[i].allocated           = false;
        g_tm_thread[i].resume_before_start = false;
        g_tm_thread[i].materialized        = false;
        g_tm_thread[i].tm_priority         = 31u;
        g_tm_thread[i].entry_function      = NULL;
    }
    for (int i = 0; i < TM_PORT_MAX_POOLS; ++i) g_tm_pool_free[i] = NULL;

    g_kernel_started = false;

    if (TaktOSInit(TM_CORE_CLOCK_HZ, TM_PORT_TICK_HZ,
                   TAKTOS_TICK_CLOCK_PROCESSOR, 0u) != TAKTOS_OK) {
        tm_check_fail("FATAL: TaktOSInit failed\n");
    }

    TmSetKernelPriorities();
    TmEnableSoftwareInterrupt();

    test_initialization_function();

    for (int i = 0; i < TM_PORT_MAX_THREADS; ++i) {
        if (g_tm_thread[i].allocated &&
            tm_materialize_thread(i) != TM_SUCCESS) {
            tm_check_fail("FATAL: thread creation failed\n");
        }
    }
    for (int i = 0; i < TM_PORT_MAX_THREADS; ++i) {
        if (g_tm_thread[i].allocated && !g_tm_thread[i].resume_before_start) {
            (void)g_tm_thread[i].thread.Suspend();
        }
    }

    g_kernel_started = true;
    TaktOSStart();
}

/* ----- Thread ---------------------------------------------------------- */
extern "C" int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    TmThreadDesc_t *t = &g_tm_thread[thread_id];
    t->allocated      = true;
    t->tm_priority    = (uint8_t)priority;
    t->entry_function = entry_function;

    if (g_kernel_started) {
        if (tm_materialize_thread(thread_id) != TM_SUCCESS) return TM_ERROR;
        return t->thread.Suspend();
    }
    return TM_SUCCESS;
}

extern "C" int tm_thread_resume(int thread_id)
{
    if (__builtin_expect(g_kernel_started, 1))
        return g_tm_thread[thread_id].thread.Resume();
    g_tm_thread[thread_id].resume_before_start = true;
    return TM_SUCCESS;
}

extern "C" int tm_thread_suspend(int thread_id)
{
    if (__builtin_expect(g_kernel_started, 1))
        return g_tm_thread[thread_id].thread.Suspend();
    g_tm_thread[thread_id].resume_before_start = false;
    return TM_SUCCESS;
}

extern "C" void tm_thread_relinquish(void) { TaktOSThreadYield(); }

extern "C" void tm_thread_sleep(int seconds)
{
    (void)TaktOSThreadSleep(TaktOSCurrentThread(),
                            (uint32_t)seconds * TM_PORT_TICK_HZ);
}

extern "C" void tm_thread_sleep_ticks(int ticks)
{
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), (uint32_t)ticks);
}

/* ----- Queue ----------------------------------------------------------- */
extern "C" int tm_queue_create(int queue_id)
{
    return g_tm_queue[queue_id].Init(g_tm_queue_storage[queue_id],
                                     TM_PORT_QUEUE_MSG_BYTES,
                                     TM_PORT_QUEUE_DEPTH);
}

extern "C" int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    return g_tm_queue[queue_id].Send(message_ptr, false, TAKTOS_NO_WAIT);
}

extern "C" int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    return g_tm_queue[queue_id].Receive(message_ptr, false, TAKTOS_NO_WAIT);
}

/* ----- Semaphore ------------------------------------------------------- */
extern "C" int tm_semaphore_create(int semaphore_id)
{
    return g_tm_sem[semaphore_id].Init(1u, 1u);
}

extern "C" int tm_semaphore_get(int semaphore_id)
{
    return g_tm_sem[semaphore_id].Take(false, TAKTOS_NO_WAIT);
}

extern "C" int tm_semaphore_put(int semaphore_id)
{
    return g_tm_sem[semaphore_id].Give(false);
}

/* ----- Mutex ----------------------------------------------------------- */
extern "C" int tm_mutex_create(int mutex_id)
{
    return g_tm_mutex[mutex_id].Init();
}

extern "C" int tm_mutex_lock(int mutex_id)
{
    return g_tm_mutex[mutex_id].Lock(true, TAKTOS_WAIT_FOREVER);
}

extern "C" int tm_mutex_unlock(int mutex_id)
{
    return g_tm_mutex[mutex_id].Unlock();
}

/* ----- Memory pool (hand-rolled linked list, identical across ports) --- */
extern "C" int tm_memory_pool_create(int pool_id)
{
    uint8_t *base = g_tm_pool_area[pool_id];
    g_tm_pool_free[pool_id] = base;
    for (uint32_t i = 0; i < TM_PORT_BLOCK_COUNT - 1u; ++i)
        *(void**)(base + i * TM_PORT_BLOCK_BYTES) =
            (void*)(base + (i + 1u) * TM_PORT_BLOCK_BYTES);
    *(void**)(base + (TM_PORT_BLOCK_COUNT - 1u) * TM_PORT_BLOCK_BYTES) = NULL;
    return TM_SUCCESS;
}

extern "C" int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    void *b = g_tm_pool_free[pool_id];
    if (!b) return TM_ERROR;
    g_tm_pool_free[pool_id] = *(void**)b;
    *memory_ptr = (unsigned char*)b;
    return TM_SUCCESS;
}

extern "C" int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    *(void**)memory_ptr = g_tm_pool_free[pool_id];
    g_tm_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

/* ----- TM5 software interrupt (arch-encapsulated by board.h) ----------- */
extern "C" void tm_cause_interrupt(void)
{
    TmCauseInterrupt();
}

extern "C" void tm_sw_irq_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}
