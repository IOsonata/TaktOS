/**-------------------------------------------------------------------------
 * @file    tm_port_zephyr.c
 *
 * @brief   Thread-Metric port for Zephyr. Shared across every MCU.
 *
 * Implements the shared 23-function tm_api.h using Zephyr primitives
 * (k_thread, k_sem, k_mutex, k_msgq, k_mem_slab, irq_offload).
 *
 * Apples-to-apples with the FreeRTOS / TaktOS / ThreadX ports:
 *   - Same shared test files (basic_processing.c, mutex_barging_test.c, ...)
 *   - Same shared tm_api.h (void-entry thread signature)
 *   - Same TM_PORT_MAX_THREADS (10), TM_PORT_STACK_BYTES (1024), TICK_HZ (1000)
 *   - Direct forward, no defensive checks  matches tm_port_taktos / tm_port_threadx
 *
 * Zephyr's k_thread_create takes void(void*,void*,void*) entries; the shared
 * test files use void(void) entries. We bridge with a per-thread trampoline
 * that calls the void-entry function. Same pattern as the FreeRTOS port.
 * -------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/kernel.h>

#include "tm_api.h"
#include "tm_port_common.h"

/* Stack size matches every other port (1024 bytes). */
#define TM_ZEPHYR_STACK_SIZE  TM_PORT_STACK_BYTES

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
#error "*** Tests are only designed for single processor systems! ***"
#endif

/* ----- Storage --------------------------------------------------------- */
static struct k_thread g_tm_thread[TM_PORT_MAX_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(g_tm_stack, TM_PORT_MAX_THREADS, TM_ZEPHYR_STACK_SIZE);
static void          (*g_tm_entry[TM_PORT_MAX_THREADS])(void);

static struct k_sem    g_tm_sem  [TM_PORT_MAX_SEMAPHORES];
static struct k_mutex  g_tm_mutex[TM_PORT_MAX_MUTEXES];

static struct k_msgq   g_tm_msgq[TM_PORT_MAX_QUEUES];
static char            g_tm_msgq_buf[TM_PORT_MAX_QUEUES]
                                    [TM_PORT_QUEUE_DEPTH * TM_PORT_QUEUE_MSG_BYTES]
                                    __aligned(4);

static struct k_mem_slab g_tm_slab[TM_PORT_MAX_POOLS];
static char              g_tm_slab_buf[TM_PORT_MAX_POOLS]
                                      [TM_PORT_BLOCK_COUNT * TM_PORT_BLOCK_BYTES]
                                      __aligned(4);

/* ----- ISR weak hooks (overridden by InterruptProcessing tests) -------- */
void tm_interrupt_handler(void)            __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void)            {}
void tm_interrupt_preemption_handler(void) {}

/* ----- Trampoline: Zephyr 3-arg entry -> shared void-entry ------------- */
static void tm_thread_trampoline(void *p1, void *p2, void *p3)
{
    int thread_id = (int)(uintptr_t)p1;
    (void)p2; (void)p3;
    g_tm_entry[thread_id]();
}

/* ----- Init ------------------------------------------------------------ */
void tm_initialize(void (*test_initialization_function)(void))
{
    /* Zephyr is already running when main() is reached. Just invoke the
     * test setup function; it creates+resumes the test threads via the
     * shared API below. */
    test_initialization_function();
}

/* ----- Thread ---------------------------------------------------------- */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    g_tm_entry[thread_id] = entry_function;

    k_tid_t tid = k_thread_create(&g_tm_thread[thread_id],
                                  g_tm_stack[thread_id],
                                  TM_ZEPHYR_STACK_SIZE,
                                  tm_thread_trampoline,
                                  (void*)(uintptr_t)thread_id, NULL, NULL,
                                  priority,    /* Zephyr: lower number = higher prio */
                                  0,
                                  K_FOREVER);  /* created suspended  matches others */

    /* Match upstream-Zephyr port semantics: created threads start in
     * sleeping state (K_FOREVER); flip to suspended so subsequent
     * tm_thread_resume / tm_thread_suspend work uniformly. */
    k_thread_suspend(&g_tm_thread[thread_id]);
    k_wakeup       (&g_tm_thread[thread_id]);

    return (tid == &g_tm_thread[thread_id]) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_resume(int thread_id)
{
    k_thread_resume(&g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

int tm_thread_suspend(int thread_id)
{
    k_thread_suspend(&g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

void tm_thread_relinquish(void) { k_yield(); }

void tm_thread_sleep(int seconds)
{
    k_sleep(K_SECONDS(seconds));
}

void tm_thread_sleep_ticks(int ticks)
{
    k_sleep(K_TICKS(ticks));
}

/* ----- Queue ----------------------------------------------------------- */
int tm_queue_create(int queue_id)
{
    k_msgq_init(&g_tm_msgq[queue_id],
                g_tm_msgq_buf[queue_id],
                TM_PORT_QUEUE_MSG_BYTES,
                TM_PORT_QUEUE_DEPTH);
    return TM_SUCCESS;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    return (k_msgq_put(&g_tm_msgq[queue_id], message_ptr, K_NO_WAIT) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    return (k_msgq_get(&g_tm_msgq[queue_id], message_ptr, K_NO_WAIT) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

/* ----- Semaphore ------------------------------------------------------- */
int tm_semaphore_create(int semaphore_id)
{
    /* binary semaphore initialized in 'available' state to match other ports */
    return (k_sem_init(&g_tm_sem[semaphore_id], 1, 1) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    return (k_sem_take(&g_tm_sem[semaphore_id], K_NO_WAIT) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    k_sem_give(&g_tm_sem[semaphore_id]);
    return TM_SUCCESS;
}

/* ----- Mutex ----------------------------------------------------------- */
int tm_mutex_create(int mutex_id)
{
    return (k_mutex_init(&g_tm_mutex[mutex_id]) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_lock(int mutex_id)
{
    return (k_mutex_lock(&g_tm_mutex[mutex_id], K_FOREVER) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    return (k_mutex_unlock(&g_tm_mutex[mutex_id]) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

/* ----- Memory pool (Zephyr native k_mem_slab) -------------------------- */
int tm_memory_pool_create(int pool_id)
{
    return (k_mem_slab_init(&g_tm_slab[pool_id],
                            g_tm_slab_buf[pool_id],
                            TM_PORT_BLOCK_BYTES,
                            TM_PORT_BLOCK_COUNT) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    return (k_mem_slab_alloc(&g_tm_slab[pool_id],
                             (void**)memory_ptr,
                             K_NO_WAIT) == 0)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    k_mem_slab_free(&g_tm_slab[pool_id], (void*)memory_ptr);
    return TM_SUCCESS;
}

/* ----- TM5 software interrupt ----------------------------------------- */
static void tm_zephyr_isr(const void *arg)
{
    (void)arg;
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

void tm_cause_interrupt(void)
{
    /* Zephyr's portable software interrupt facility. Requires
     * CONFIG_IRQ_OFFLOAD=y in prj.conf. */
    irq_offload(tm_zephyr_isr, NULL);
}
