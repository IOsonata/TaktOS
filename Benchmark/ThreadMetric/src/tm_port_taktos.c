/**-------------------------------------------------------------------------
 * @file    tm_port_taktos.c
 *
 * @brief   Thread-Metric port for TaktOS (C API). Shared across every MCU.
 *
 * Mirror of tm_port_taktos.cpp, but uses the TaktOS C API
 * (TaktOSThreadCreate / TaktOSSemTake / ...) instead of the C++ objects.
 * Generates a separate binary with its own set of numbers  the C-API
 * path has slightly different inlining characteristics than the C++
 * method-dispatch path, so both are worth publishing.
 *
 * Per-MCU board.h supplies TM_CORE_CLOCK_HZ, TM_SW_IRQn, and the
 * arch-specific implementations of TmCauseInterrupt / TmSetKernelPriorities
 * / TmEnableSoftwareInterrupt. This file contains no architecture-specific
 * code  it builds for ARM Cortex-M and RISC-V RV32 alike.
 * -------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "tm_api.h"
#include "tm_port_common.h"
#include "board.h"           /* arch-encapsulating: TmCauseInterrupt, etc. */

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"

/* ----- TM (1high..31low) -> TaktOS (32high..1low) priority --------------- */
static inline uint8_t tm_to_taktos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (uint8_t)(32 - tm_priority);
}

/* ----- Storage --------------------------------------------------------- */
typedef struct {
    bool              allocated;
    bool              resume_before_start;
    uint8_t           tm_priority;
    void            (*entry_function)(void);
    TaktOSThread_t   *handle;
    uint8_t           mem[TAKTOS_THREAD_MEM_SIZE(TM_PORT_STACK_BYTES)]
                          __attribute__((aligned(4)));
} TmThreadDesc_t;

static TmThreadDesc_t g_tm_thread[TM_PORT_MAX_THREADS];
static bool           g_kernel_started = false;

static TaktOSQueue_t  g_tm_queue        [TM_PORT_MAX_QUEUES];
static uint8_t        g_tm_queue_storage[TM_PORT_MAX_QUEUES]
                                        [TM_PORT_QUEUE_DEPTH * TM_PORT_QUEUE_MSG_BYTES]
                                        __attribute__((aligned(4)));

static TaktOSSem_t    g_tm_sem  [TM_PORT_MAX_SEMAPHORES];
static TaktOSMutex_t  g_tm_mutex[TM_PORT_MAX_MUTEXES];

static uint8_t        g_tm_pool_area[TM_PORT_MAX_POOLS][TM_PORT_POOL_BYTES]
                                    __attribute__((aligned(sizeof(void*))));
static void          *g_tm_pool_free[TM_PORT_MAX_POOLS];

/* ----- ISR weak hooks (overridden by InterruptProcessing tests) -------- */
void tm_interrupt_handler(void)            __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void)            {}
void tm_interrupt_preemption_handler(void) {}

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
    if (t->handle != NULL) return TM_SUCCESS;
    t->handle = TaktOSThreadCreate(t->mem,
                                   (uint32_t)sizeof(t->mem),
                                   tm_thread_trampoline,
                                   (void*)(uintptr_t)thread_id,
                                   tm_to_taktos_priority(t->tm_priority));
    return (t->handle != NULL) ? TM_SUCCESS : TM_ERROR;
}

/* ----- Init ------------------------------------------------------------ */
void tm_initialize(void (*test_initialization_function)(void))
{
    for (int i = 0; i < TM_PORT_MAX_THREADS; ++i) {
        g_tm_thread[i].allocated           = false;
        g_tm_thread[i].resume_before_start = false;
        g_tm_thread[i].tm_priority         = 31u;
        g_tm_thread[i].entry_function      = NULL;
        g_tm_thread[i].handle              = NULL;
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
            (void)TaktOSThreadSuspend(g_tm_thread[i].handle);
        }
    }

    g_kernel_started = true;
    TaktOSStart();
}

/* ----- Thread ---------------------------------------------------------- */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    TmThreadDesc_t *t = &g_tm_thread[thread_id];
    t->allocated      = true;
    t->tm_priority    = (uint8_t)priority;
    t->entry_function = entry_function;

    if (g_kernel_started) {
        if (tm_materialize_thread(thread_id) != TM_SUCCESS) return TM_ERROR;
        return TaktOSThreadSuspend(t->handle);
    }
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (__builtin_expect(g_kernel_started, 1))
        return TaktOSThreadResume(g_tm_thread[thread_id].handle);
    g_tm_thread[thread_id].resume_before_start = true;
    return TM_SUCCESS;
}

int tm_thread_suspend(int thread_id)
{
    if (__builtin_expect(g_kernel_started, 1))
        return TaktOSThreadSuspend(g_tm_thread[thread_id].handle);
    g_tm_thread[thread_id].resume_before_start = false;
    return TM_SUCCESS;
}

void tm_thread_relinquish(void) { TaktOSThreadYield(); }

void tm_thread_sleep(int seconds)
{
    (void)TaktOSThreadSleep(TaktOSCurrentThread(),
                            (uint32_t)seconds * TM_PORT_TICK_HZ);
}

void tm_thread_sleep_ticks(int ticks)
{
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), (uint32_t)ticks);
}

/* ----- Queue ----------------------------------------------------------- */
int tm_queue_create(int queue_id)
{
    return TaktOSQueueInit(&g_tm_queue[queue_id],
                           g_tm_queue_storage[queue_id],
                           TM_PORT_QUEUE_MSG_BYTES,
                           TM_PORT_QUEUE_DEPTH);
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    return TaktOSQueueSend(&g_tm_queue[queue_id], message_ptr, false, TAKTOS_NO_WAIT);
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    return TaktOSQueueReceive(&g_tm_queue[queue_id], message_ptr,
                              false, TAKTOS_NO_WAIT);
}

/* ----- Semaphore ------------------------------------------------------- */
int tm_semaphore_create(int semaphore_id)
{
    return TaktOSSemInit(&g_tm_sem[semaphore_id], 1u, 1u);
}

int tm_semaphore_get(int semaphore_id)
{
    return TaktOSSemTake(&g_tm_sem[semaphore_id], false, TAKTOS_NO_WAIT);
}

int tm_semaphore_put(int semaphore_id)
{
    return TaktOSSemGive(&g_tm_sem[semaphore_id], false);
}

/* ----- Mutex ----------------------------------------------------------- */
int tm_mutex_create(int mutex_id)
{
    return TaktOSMutexInit(&g_tm_mutex[mutex_id]);
}

int tm_mutex_lock(int mutex_id)
{
    return TaktOSMutexLock(&g_tm_mutex[mutex_id], true, TAKTOS_WAIT_FOREVER);
}

int tm_mutex_unlock(int mutex_id)
{
    return TaktOSMutexUnlock(&g_tm_mutex[mutex_id]);
}

/* ----- Memory pool (hand-rolled linked list, identical across ports) --- */
int tm_memory_pool_create(int pool_id)
{
    uint8_t *base = g_tm_pool_area[pool_id];
    g_tm_pool_free[pool_id] = base;
    for (uint32_t i = 0; i < TM_PORT_BLOCK_COUNT - 1u; ++i)
        *(void**)(base + i * TM_PORT_BLOCK_BYTES) =
            (void*)(base + (i + 1u) * TM_PORT_BLOCK_BYTES);
    *(void**)(base + (TM_PORT_BLOCK_COUNT - 1u) * TM_PORT_BLOCK_BYTES) = NULL;
    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    void *b = g_tm_pool_free[pool_id];
    if (!b) return TM_ERROR;
    g_tm_pool_free[pool_id] = *(void**)b;
    *memory_ptr = (unsigned char*)b;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    *(void**)memory_ptr = g_tm_pool_free[pool_id];
    g_tm_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

/* ----- TM5 software interrupt (arch-encapsulated by board.h) ----------- */
void tm_cause_interrupt(void)
{
    TmCauseInterrupt();
}

void tm_sw_irq_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}
