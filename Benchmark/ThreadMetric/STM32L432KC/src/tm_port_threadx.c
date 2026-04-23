#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifndef TX_DISABLE_ERROR_CHECKING
#define TX_DISABLE_ERROR_CHECKING
#endif
#include "tx_api.h"
#include "tm_api.h"

// -----------------------------------------------------------------------------
// ThreadX bare-metal STM32L432KC port for the official Thread-Metric harness.
//
// Target: NUCLEO-L432KC, STM32L432KCU6, Cortex-M4F @ 80 MHz.
// UART  : USART2 on PA2 (AF7), 115200 8N1 (in tm_console_stm32l432kc.cpp - shared).
// Tick  : 1000 Hz (ThreadX owns SysTick via threadx_initialize_low_level.S).
// SW IRQ: TIM7_IRQn (IRQ 55) - pended only; TIM7 is never clocked.
//
// ThreadX supplies its own SysTick_Handler, SVC_Handler, and PendSV_Handler
// (__tx_SysTickHandler / __tx_SVCallHandler / __tx_PendSVHandler) in its
// Cortex-M4 port's tx_thread_schedule.S, tx_thread_context_save.S, and
// threadx_initialize_low_level.S. These are strong symbols that override the
// weak aliases in startup_stm32l432kc.S at link time.
// -----------------------------------------------------------------------------

#define TM_THREADX_MAX_THREADS       15
#define TM_THREADX_MAX_QUEUES        1
#define TM_THREADX_MAX_SEMAPHORES    1
#define TM_THREADX_MAX_MUTEXES       1
#define TM_THREADX_MAX_MEMORY_POOLS  1
/* 15 threads x 1024 bytes = 15 KB total stack area.
 * Matches the TaktOS port sizing for comparable numbers on 48 KB SRAM1. */
#define TM_THREADX_STACK_BYTES       1024u
#define TM_THREADX_TICK_HZ           1000u
/* 10 messages x 4 ULONGs (16 B) = 160 B of queue storage. Matches the
 * TaktOS and FreeRTOS queue depth so Thread-Metric message-processing
 * numbers are comparable.                                                */
#define TM_THREADX_QUEUE_SIZE        160u
#define TM_THREADX_QUEUE_MSG_WORDS   (4u * sizeof(unsigned long) / sizeof(ULONG))
#define TM_BLOCK_SIZE                128u
#define TM_POOL_SIZE                 2048u

// -----------------------------------------------------------------------------
// NVIC addresses. STM32L4 has 4 implementable priority bits.
// TIM7_IRQn = 55 (STM32L432xx). Pended only - TIM7 is never clocked.
// -----------------------------------------------------------------------------
#define NVIC_ISER1                   (*(volatile uint32_t*)0xE000E104u)  // IRQ32..63
#define NVIC_IPR13                   (*(volatile uint32_t*)0xE000E434u)  // IRQ52..55
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     55u

// -----------------------------------------------------------------------------
// Per-thread storage
// -----------------------------------------------------------------------------
static TX_THREAD      g_tm_thread[TM_THREADX_MAX_THREADS];
static ULONG          g_tm_stack[TM_THREADX_MAX_THREADS]
                                [(TM_THREADX_STACK_BYTES + sizeof(ULONG) - 1u) / sizeof(ULONG)]
                                __attribute__((aligned(8)));
static VOID         (*g_tm_entry[TM_THREADX_MAX_THREADS])(void);

static TX_QUEUE       g_tm_queue[TM_THREADX_MAX_QUEUES];
static ULONG          g_tm_queue_storage[TM_THREADX_MAX_QUEUES]
                                        [TM_THREADX_QUEUE_SIZE / sizeof(ULONG)]
                                        __attribute__((aligned(8)));
static TX_SEMAPHORE   g_tm_sem[TM_THREADX_MAX_SEMAPHORES];
static TX_MUTEX       g_tm_mutex[TM_THREADX_MAX_MUTEXES];
static bool           g_tm_mutex_created[TM_THREADX_MAX_MUTEXES];
static TX_BLOCK_POOL  g_tm_pool[TM_THREADX_MAX_MEMORY_POOLS];
static ULONG          g_tm_pool_area[TM_THREADX_MAX_MEMORY_POOLS]
                                    [TM_POOL_SIZE / sizeof(ULONG)]
                                    __attribute__((aligned(8)));

static VOID         (*g_tm_init_fn)(void) = TX_NULL;

// Weak no-op hooks
void tm_interrupt_handler(void) __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void) {}
void tm_interrupt_preemption_handler(void) {}

static UINT tm_map_prio(int tm_prio)
{
    if (tm_prio < 1)  tm_prio = 1;
    if (tm_prio > 31) tm_prio = 31;
    return (UINT)tm_prio;
}

static VOID tm_thread_entry(ULONG arg)
{
    UINT id = (UINT)arg;
    if (id < TM_THREADX_MAX_THREADS && g_tm_entry[id] != TX_NULL) {
        g_tm_entry[id]();
    }
    for (;;) {}
}

// -----------------------------------------------------------------------------
// ThreadX application entry point. Called from _tx_initialize_kernel_enter.
// -----------------------------------------------------------------------------
void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    // Configure TIM7_IRQn priority and enable it BEFORE kicking the test.
    // IPR13 bits [31:24] = IRQ55. 4 prio bits, set to 0x80 (middle) so it
    // preempts threads but sits below kernel-critical sections.
    NVIC_IPR13 &= ~(0xFFu << 24);
    NVIC_IPR13 |=  (0x80u << 24);
    NVIC_ISER1  = (1u << (SW_IRQ_N - 32u));

    if (g_tm_init_fn != TX_NULL) {
        g_tm_init_fn();
    }
}

void tm_initialize(void (*test_initialization_function)(void))
{
    UINT i;
    for (i = 0; i < TM_THREADX_MAX_THREADS; ++i) {
        g_tm_entry[i] = TX_NULL;
    }
    for (i = 0; i < TM_THREADX_MAX_MUTEXES; ++i) {
        g_tm_mutex_created[i] = false;
    }
    g_tm_init_fn = test_initialization_function;
    tx_kernel_enter();  // never returns
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    UINT status;
    if (thread_id < 0 || thread_id >= TM_THREADX_MAX_THREADS || entry_function == 0)
        return TM_ERROR;

    g_tm_entry[thread_id] = entry_function;
    status = tx_thread_create(&g_tm_thread[thread_id],
                              (CHAR *)"TM",
                              tm_thread_entry,
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
    if (thread_id < 0 || thread_id >= TM_THREADX_MAX_THREADS) return TM_ERROR;
    return (tx_thread_resume(&g_tm_thread[thread_id]) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_THREADX_MAX_THREADS) return TM_ERROR;
    return (tx_thread_suspend(&g_tm_thread[thread_id]) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

void tm_thread_relinquish(void)
{
    tx_thread_relinquish();
}

void tm_thread_sleep(int seconds)
{
    tx_thread_sleep((ULONG)(seconds * TM_THREADX_TICK_HZ));
}

void tm_thread_sleep_ticks(int ticks)
{
    tx_thread_sleep((ULONG)(ticks <= 0 ? 1 : ticks));
}

int tm_queue_create(int queue_id)
{
    UINT status;
    if (queue_id < 0 || queue_id >= TM_THREADX_MAX_QUEUES) return TM_ERROR;
    status = tx_queue_create(&g_tm_queue[queue_id],
                             (CHAR *)"Q",
                             (UINT)TM_THREADX_QUEUE_MSG_WORDS,
                             g_tm_queue_storage[queue_id],
                             sizeof(g_tm_queue_storage[queue_id]));
    return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_THREADX_MAX_QUEUES) return TM_ERROR;
    return (tx_queue_send(&g_tm_queue[queue_id], message_ptr, TX_NO_WAIT) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_THREADX_MAX_QUEUES) return TM_ERROR;
    return (tx_queue_receive(&g_tm_queue[queue_id], message_ptr, TX_NO_WAIT) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_THREADX_MAX_SEMAPHORES) return TM_ERROR;
    return (tx_semaphore_create(&g_tm_sem[semaphore_id], (CHAR *)"S", 1) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_THREADX_MAX_SEMAPHORES) return TM_ERROR;
    return (tx_semaphore_get(&g_tm_sem[semaphore_id], TX_NO_WAIT) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_THREADX_MAX_SEMAPHORES) return TM_ERROR;
    return (tx_semaphore_put(&g_tm_sem[semaphore_id]) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_THREADX_MAX_MUTEXES) return TM_ERROR;
    UINT status = tx_mutex_create(&g_tm_mutex[mutex_id], (CHAR *)"M", TX_INHERIT);
    g_tm_mutex_created[mutex_id] = (status == TX_SUCCESS);
    return g_tm_mutex_created[mutex_id] ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_THREADX_MAX_MUTEXES || !g_tm_mutex_created[mutex_id])
        return TM_ERROR;
    return (tx_mutex_get(&g_tm_mutex[mutex_id], TX_WAIT_FOREVER) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_THREADX_MAX_MUTEXES || !g_tm_mutex_created[mutex_id])
        return TM_ERROR;
    return (tx_mutex_put(&g_tm_mutex[mutex_id]) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    if (pool_id < 0 || pool_id >= TM_THREADX_MAX_MEMORY_POOLS) return TM_ERROR;
    return (tx_block_pool_create(&g_tm_pool[pool_id], (CHAR *)"P", TM_BLOCK_SIZE,
                                 g_tm_pool_area[pool_id],
                                 sizeof(g_tm_pool_area[pool_id])) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_THREADX_MAX_MEMORY_POOLS || memory_ptr == 0) return TM_ERROR;
    return (tx_block_allocate(&g_tm_pool[pool_id], (VOID **)memory_ptr, TX_NO_WAIT) == TX_SUCCESS)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    (void)pool_id;
    return (tx_block_release((VOID *)memory_ptr) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

void tm_cause_interrupt(void)
{
    NVIC_STIR = SW_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// TIM7_IRQHandler - borrowed for tm_cause_interrupt().
// Strong symbol overrides the weak alias in startup_stm32l432kc.S.
// -----------------------------------------------------------------------------
void TIM7_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}
