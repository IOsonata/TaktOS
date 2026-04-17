#include <stdint.h>
#include <string.h>
#include <stdbool.h>

//#define TX_INCLUDE_USER_DEFINE_FILE
#ifndef TX_DISABLE_ERROR_CHECKING
#define TX_DISABLE_ERROR_CHECKING
#endif
#include "tx_api.h"
#include "tm_api.h"

#define TM_THREADX_MAX_THREADS       15
#define TM_THREADX_MAX_QUEUES        1
#define TM_THREADX_MAX_SEMAPHORES    1
#define TM_THREADX_MAX_MUTEXES     1
#define TM_THREADX_MAX_MEMORY_POOLS  1
#define TM_THREADX_STACK_BYTES       2096u
#define TM_THREADX_TICK_HZ           1000u
#define TM_THREADX_QUEUE_SIZE        400u
#define TM_THREADX_QUEUE_MSG_WORDS   (4u * sizeof(unsigned long) / sizeof(ULONG))
#define TM_BLOCK_SIZE                128u
#define TM_POOL_SIZE                 2048u

/* nRF52 UARTE0 TX on P0.07 */
#define UARTE0_BASE                  0x40002000u
#define UARTE0_STARTTX               (*(volatile uint32_t*)(UARTE0_BASE + 0x008u))
#define UARTE0_STOPTX                (*(volatile uint32_t*)(UARTE0_BASE + 0x00Cu))
#define UARTE0_ENDTX                 (*(volatile uint32_t*)(UARTE0_BASE + 0x120u))
#define UARTE0_TXD_PTR               (*(volatile uint32_t*)(UARTE0_BASE + 0x544u))
#define UARTE0_TXD_MAXCNT            (*(volatile uint32_t*)(UARTE0_BASE + 0x548u))
#define UARTE0_ENABLE                (*(volatile uint32_t*)(UARTE0_BASE + 0x500u))
#define UARTE0_PSEL_TXD              (*(volatile uint32_t*)(UARTE0_BASE + 0x50Cu))
#define UARTE0_BAUDRATE              (*(volatile uint32_t*)(UARTE0_BASE + 0x524u))
#define UARTE0_CONFIG                (*(volatile uint32_t*)(UARTE0_BASE + 0x56Cu))
#define NRF52_GPIO_BASE              0x50000000u
#define GPIO_DIRSET                  (*(volatile uint32_t*)(NRF52_GPIO_BASE + 0x518u))
#define GPIO_OUTSET                  (*(volatile uint32_t*)(NRF52_GPIO_BASE + 0x508u))
#define UART_TX_PIN                  7u
#define UART_BAUDRATE_115200         0x01D7E000u

static TX_THREAD g_tm_thread[TM_THREADX_MAX_THREADS];
static ULONG g_tm_stack[TM_THREADX_MAX_THREADS][(TM_THREADX_STACK_BYTES + sizeof(ULONG) - 1u) / sizeof(ULONG)] __attribute__((aligned(8)));
static VOID (*g_tm_entry[TM_THREADX_MAX_THREADS])(void);
static TX_QUEUE g_tm_queue[TM_THREADX_MAX_QUEUES];
static ULONG g_tm_queue_storage[TM_THREADX_MAX_QUEUES][TM_THREADX_QUEUE_SIZE / sizeof(ULONG)] __attribute__((aligned(8)));
static TX_SEMAPHORE g_tm_sem[TM_THREADX_MAX_SEMAPHORES];
static TX_MUTEX g_tm_mutex[TM_THREADX_MAX_MUTEXES];
static bool g_tm_mutex_created[TM_THREADX_MAX_MUTEXES];
static TX_BLOCK_POOL g_tm_pool[TM_THREADX_MAX_MEMORY_POOLS];
static ULONG g_tm_pool_area[TM_THREADX_MAX_MEMORY_POOLS][TM_POOL_SIZE / sizeof(ULONG)] __attribute__((aligned(8)));
static CHAR g_uart_buf[128];
static VOID (*g_tm_init_fn)(void) = TX_NULL;

static void tm_uart_send(const char *buf, uint32_t len)
{
    while (len > 0u) {
        uint32_t chunk = (len > sizeof(g_uart_buf)) ? (uint32_t)sizeof(g_uart_buf) : len;
        memcpy(g_uart_buf, buf, chunk);
        UARTE0_ENDTX      = 0u;
        UARTE0_TXD_PTR    = (uint32_t)(uintptr_t)g_uart_buf;
        UARTE0_TXD_MAXCNT = chunk;
        UARTE0_STARTTX    = 1u;
        while (!UARTE0_ENDTX) {}
        UARTE0_STOPTX     = 1u;
        buf += chunk;
        len -= chunk;
    }
}

void tm_hw_console_init(void)
{
    GPIO_DIRSET     = (1u << UART_TX_PIN);
    GPIO_OUTSET     = (1u << UART_TX_PIN);
    UARTE0_PSEL_TXD = UART_TX_PIN;
    UARTE0_BAUDRATE = UART_BAUDRATE_115200;
    UARTE0_CONFIG   = 0u;
    UARTE0_ENABLE   = 8u;
}

static UINT tm_map_prio(int tm_prio)
{
    if (tm_prio < 1) tm_prio = 1;
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

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;
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
    tx_kernel_enter();
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
    return (tx_queue_send(&g_tm_queue[queue_id], message_ptr, TX_NO_WAIT) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_THREADX_MAX_QUEUES) return TM_ERROR;
    return (tx_queue_receive(&g_tm_queue[queue_id], message_ptr, TX_NO_WAIT) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_THREADX_MAX_SEMAPHORES) return TM_ERROR;
    return (tx_semaphore_create(&g_tm_sem[semaphore_id], (CHAR *)"S", 1) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_THREADX_MAX_SEMAPHORES) return TM_ERROR;
    return (tx_semaphore_get(&g_tm_sem[semaphore_id], TX_NO_WAIT) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_THREADX_MAX_SEMAPHORES) return TM_ERROR;
    return (tx_semaphore_put(&g_tm_sem[semaphore_id]) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
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
    if (mutex_id < 0 || mutex_id >= TM_THREADX_MAX_MUTEXES || !g_tm_mutex_created[mutex_id]) return TM_ERROR;
    return (tx_mutex_get(&g_tm_mutex[mutex_id], TX_WAIT_FOREVER) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_THREADX_MAX_MUTEXES || !g_tm_mutex_created[mutex_id]) return TM_ERROR;
    return (tx_mutex_put(&g_tm_mutex[mutex_id]) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    if (pool_id < 0 || pool_id >= TM_THREADX_MAX_MEMORY_POOLS) return TM_ERROR;
    return (tx_block_pool_create(&g_tm_pool[pool_id], (CHAR *)"P", TM_BLOCK_SIZE,
                                 g_tm_pool_area[pool_id], sizeof(g_tm_pool_area[pool_id])) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_THREADX_MAX_MEMORY_POOLS || memory_ptr == 0) return TM_ERROR;
    return (tx_block_allocate(&g_tm_pool[pool_id], (VOID **)memory_ptr, TX_NO_WAIT) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    (void)pool_id;
    return (tx_block_release((VOID *)memory_ptr) == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

void tm_cause_interrupt(void)
{
    /* Not implemented in this bundle. */
}

// -----------------------------------------------------------------------------
// Non-Blocking Asynchronous UART via nRF52 EasyDMA
// -----------------------------------------------------------------------------
// Output buffer.  Sized to hold one full report line with margin.
// tm_uart_flush() is synchronous — it waits for DMA completion and issues
// STOPTX before returning, matching the nRF52 UARTE EasyDMA requirements.
// The reporter sleeps 30 seconds between outputs so transmission time is
// negligible and there is no benefit to an async design.
#define TM_UART_BUF_SIZE 256
static char     g_uart_tx_buf[TM_UART_BUF_SIZE];
static uint32_t g_uart_tx_idx = 0;

static void tm_uart_flush(void)
{
    if (g_uart_tx_idx == 0)
    {
        return;
    }

    UARTE0_ENDTX      = 0u;
    UARTE0_TXD_PTR    = (uint32_t)(uintptr_t)g_uart_tx_buf;
    UARTE0_TXD_MAXCNT = g_uart_tx_idx;
    UARTE0_STARTTX    = 1u;
    while (!UARTE0_ENDTX) {}
    UARTE0_STOPTX     = 1u;          // required between transfers on nRF52 UARTE
    g_uart_tx_idx     = 0;
}

void tm_putchar(int c)
{
    if (g_uart_tx_idx < TM_UART_BUF_SIZE)
    {
        g_uart_tx_buf[g_uart_tx_idx++] = (char)c;
    }

    // Flush on every newline or if the buffer is full.
    // Per-line flushing ensures each line is fully transmitted before the
    // task can be preempted, preventing partial lines from being stranded
    // in the buffer when the next 30-second window starts.
    if (c == '\n' || g_uart_tx_idx >= TM_UART_BUF_SIZE)
    {
        tm_uart_flush();
    }
}

