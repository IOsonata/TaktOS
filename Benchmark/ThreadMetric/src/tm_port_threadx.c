/**-------------------------------------------------------------------------
 * @file    tm_port_threadx.c
 *
 * @brief   Thread-Metric port for ThreadX. Shared across every MCU.
 *
 * Per-MCU board.h supplies TM_SW_IRQn and the arch-specific
 * implementations of TmCauseInterrupt / TmEnableSoftwareInterrupt.
 * libIOsonata_<MCU>.a supplies startup, vector table, clock, UART.
 * tm_report.cpp owns console output.
 *
 * Shape: direct forward, matching Microsoft's Zephyr reference.
 *
 * FIX vs. per-MCU bundle: tm_cause_interrupt() was unimplemented  it
 * returned without doing anything, making every ThreadX TM5 result
 * invalid. Now routes through board.h's TmCauseInterrupt(), identical
 * to the TaktOS / FreeRTOS / Zephyr ports.
 *
 * Stack: TM_PORT_STACK_BYTES (1024). Matches TaktOS / FreeRTOS / Zephyr.
 * Prior per-MCU value of 2096 bytes was a workaround for an unrelated
 * issue and introduced a 2 asymmetry with the other RTOSes.
 *
 * No architecture-specific code in this file  builds for ARM Cortex-M
 * and RISC-V RV32 alike (both arches need TX assembly support files at
 * link time, but those are out-of-tree and selected by Eclipse project).
 * -------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* TX_DISABLE_ERROR_CHECKING and the rest of the ThreadX feature switches are
 * set centrally in include/tx_user.h. Do NOT redefine them here  the build
 * passes -DTX_INCLUDE_USER_DEFINE_FILE so tx_api.h pulls them in. */
#include "tx_api.h"

#include "tm_api.h"
#include "tm_port_common.h"
#include "board.h"           /* TM_SW_IRQn */

#define TM_STACK_ULONGS \
    ((TM_PORT_STACK_BYTES + sizeof(ULONG) - 1u) / sizeof(ULONG))

/* ThreadX queue message size is in ULONGs  Microsoft passes 4 ULONGs per
 * message in the reference test harness. Same here. */
#define TM_QUEUE_MSG_ULONGS     (TM_PORT_QUEUE_MSG_BYTES / sizeof(ULONG))
#define TM_QUEUE_STORAGE_ULONGS (TM_PORT_QUEUE_DEPTH * TM_QUEUE_MSG_ULONGS)

#define TM_POOL_STORAGE_ULONGS  (TM_PORT_POOL_BYTES / sizeof(ULONG))

static TX_THREAD     g_tm_thread   [TM_PORT_MAX_THREADS];
static ULONG         g_tm_stack    [TM_PORT_MAX_THREADS][TM_STACK_ULONGS]
                                     __attribute__((aligned(8)));
static VOID        (*g_tm_entry    [TM_PORT_MAX_THREADS])(void);

static TX_QUEUE      g_tm_queue         [TM_PORT_MAX_QUEUES];
static ULONG         g_tm_queue_storage [TM_PORT_MAX_QUEUES][TM_QUEUE_STORAGE_ULONGS]
                                         __attribute__((aligned(8)));

static TX_SEMAPHORE  g_tm_sem  [TM_PORT_MAX_SEMAPHORES];
static TX_MUTEX      g_tm_mutex[TM_PORT_MAX_MUTEXES];

static TX_BLOCK_POOL g_tm_pool     [TM_PORT_MAX_POOLS];
static ULONG         g_tm_pool_area[TM_PORT_MAX_POOLS][TM_POOL_STORAGE_ULONGS]
                                    __attribute__((aligned(8)));

static VOID        (*g_tm_init_fn)(void) = TX_NULL;


void tm_interrupt_handler(void)            __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void)            {}
void tm_interrupt_preemption_handler(void) {}


static UINT tm_map_prio(int tm_prio)
{
    return (UINT)tm_prio;
}

static VOID tm_thread_trampoline(ULONG param)
{
    UINT thread_id = (UINT)param;
    if (g_tm_entry[thread_id] != TX_NULL) g_tm_entry[thread_id]();
    for (;;) {}
}


/* ----- Init ------------------------------------------------------------- */
void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;
    if (g_tm_init_fn != TX_NULL) g_tm_init_fn();
}

void tm_initialize(void (*test_initialization_function)(void))
{
    for (UINT i = 0; i < TM_PORT_MAX_THREADS; ++i) g_tm_entry[i] = TX_NULL;

    /* Note: ThreadX sets PendSV/SysTick priorities itself in tx_initialize_low_level.S,
     * so TmSetKernelPriorities() is redundant here. Only the SW IRQ enable is needed. */
    TmEnableSoftwareInterrupt();

    g_tm_init_fn = test_initialization_function;
    tx_kernel_enter();   /* never returns */
}


/* ----- Thread ----------------------------------------------------------- */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    g_tm_entry[thread_id] = entry_function;
    UINT status = tx_thread_create(&g_tm_thread[thread_id],
                                   (CHAR*)"TM",
                                   tm_thread_trampoline,
                                   (ULONG)thread_id,
                                   g_tm_stack[thread_id],
                                   sizeof(g_tm_stack[thread_id]),
                                   tm_map_prio(priority),
                                   tm_map_prio(priority),
                                   TX_NO_TIME_SLICE,
                                   TX_DONT_START);
    return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_resume(int thread_id)
{
    return (tx_thread_resume(&g_tm_thread[thread_id]) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_suspend(int thread_id)
{
    return (tx_thread_suspend(&g_tm_thread[thread_id]) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

void tm_thread_relinquish(void) { tx_thread_relinquish(); }

void tm_thread_sleep(int seconds)
{
    tx_thread_sleep((ULONG)seconds * TM_PORT_TICK_HZ);
}

void tm_thread_sleep_ticks(int ticks)
{
    tx_thread_sleep((ULONG)ticks);
}


/* ----- Queue ------------------------------------------------------------ */
int tm_queue_create(int queue_id)
{
    UINT status = tx_queue_create(&g_tm_queue[queue_id],
                                  (CHAR*)"Q",
                                  (UINT)TM_QUEUE_MSG_ULONGS,
                                  g_tm_queue_storage[queue_id],
                                  sizeof(g_tm_queue_storage[queue_id]));
    return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    return (tx_queue_send(&g_tm_queue[queue_id], message_ptr, TX_NO_WAIT) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    return (tx_queue_receive(&g_tm_queue[queue_id], message_ptr, TX_NO_WAIT) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}


/* ----- Semaphore -------------------------------------------------------- */
int tm_semaphore_create(int semaphore_id)
{
    return (tx_semaphore_create(&g_tm_sem[semaphore_id], (CHAR*)"S", 1) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    return (tx_semaphore_get(&g_tm_sem[semaphore_id], TX_NO_WAIT) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    return (tx_semaphore_put(&g_tm_sem[semaphore_id]) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}


/* ----- Mutex ------------------------------------------------------------ */
int tm_mutex_create(int mutex_id)
{
    return (tx_mutex_create(&g_tm_mutex[mutex_id], (CHAR*)"M", TX_INHERIT) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_lock(int mutex_id)
{
    return (tx_mutex_get(&g_tm_mutex[mutex_id], TX_WAIT_FOREVER) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    return (tx_mutex_put(&g_tm_mutex[mutex_id]) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}


/* ----- Memory pool (ThreadX native TX_BLOCK_POOL) ----------------------- */
int tm_memory_pool_create(int pool_id)
{
    return (tx_block_pool_create(&g_tm_pool[pool_id],
                                 (CHAR*)"P",
                                 TM_PORT_BLOCK_BYTES,
                                 g_tm_pool_area[pool_id],
                                 sizeof(g_tm_pool_area[pool_id])) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    return (tx_block_allocate(&g_tm_pool[pool_id], (VOID**)memory_ptr, TX_NO_WAIT) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    (void)pool_id;
    return (tx_block_release((VOID*)memory_ptr) == TX_SUCCESS)
           ? TM_SUCCESS : TM_ERROR;
}


/* ----- TM5 software interrupt (arch-encapsulated by board.h) -----------
 * FIX: the per-MCU ThreadX bundle left this as an empty function. The
 * TM5 test would then measure zero context cost for "interrupt with
 * preemption"  a silently invalid result. Now fires the same SW IRQ
 * path as TaktOS / FreeRTOS / Zephyr ports via board.h.                 */
void tm_cause_interrupt(void)
{
    TmCauseInterrupt();
}

void tm_sw_irq_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}
