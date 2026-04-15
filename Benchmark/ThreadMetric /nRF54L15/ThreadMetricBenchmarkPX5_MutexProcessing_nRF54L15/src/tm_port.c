
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include "../../px5/source/pthread.h"
#include "../../px5/source/semaphore.h"
#include "../../px5/source/mqueue.h"
#include "../../px5/source/sched.h"
#include "../../px5/source/unistd.h"
#include "../../px5/source/fcntl.h"
#include <string.h>
#include <cstdio>

#include "tm_api.h"

// -----------------------------------------------------------------------------
// nRF54L15 app-core console / soft-IRQ helpers reused across ports.
// -----------------------------------------------------------------------------
#include "nrf.h"

#define TM_RTOS_MAX_THREADS        10
#define TM_RTOS_MAX_QUEUES         1
#define TM_RTOS_MAX_SEMAPHORES     1
#define TM_RTOS_MAX_MUTEXES        1
#define TM_RTOS_MAX_POOLS          1

#define TM_RTOS_STACK_BYTES        1024u
#define TM_RTOS_TICK_HZ            1000u
#define TM_RTOS_CORE_CLOCK_HZ      128000000u

#define TM_RTOS_QUEUE_DEPTH        10u
#define TM_RTOS_QUEUE_MSG_SIZE     (4u * sizeof(unsigned long))
#define TM_RTOS_QUEUE_ULONGS_PER_MSG 4u

#define TM_BLOCK_SIZE              128u
#define TM_POOL_SIZE               2048u
#define TM_BLOCK_COUNT             (TM_POOL_SIZE / TM_BLOCK_SIZE)

#define UART_TX_PIN                0u
#define UART_BAUDRATE_115200       0x01D7E000u

#define SCB_SHPR3                  (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER0                 (*(volatile uint32_t*)0xE000E100u)
#define NVIC_IPR                   ((volatile uint8_t*)0xE000E400u)
#define NVIC_STIR                  (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                   28u

static uint8_t       g_pool_area[TM_RTOS_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static void         *g_pool_free[TM_RTOS_MAX_POOLS];
static bool          g_pool_created[TM_RTOS_MAX_POOLS];
static char          g_uart_buf[128];

void tm_interrupt_handler(void) __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void) {}
void tm_interrupt_preemption_handler(void) {}

static void tm_uart_send(const char *buf, uint32_t len)
{
    NRF_UARTE_Type *uart = NRF_UARTE30_S;
    while (len > 0u) {
        uint32_t chunk = (len > sizeof(g_uart_buf)) ? (uint32_t)sizeof(g_uart_buf) : len;
        for (uint32_t i = 0u; i < chunk; ++i) {
            g_uart_buf[i] = buf[i];
        }
        uart->EVENTS_DMA.TX.END   = 0u;
        uart->DMA.TX.PTR          = (uint32_t)(uintptr_t)g_uart_buf;
        uart->DMA.TX.MAXCNT       = chunk;
        uart->TASKS_DMA.TX.START  = 1u;
        while (!uart->EVENTS_DMA.TX.END) {}
        uart->TASKS_DMA.TX.STOP   = 1u;
        buf += chunk;
        len -= chunk;
    }
}

void tm_hw_console_init(void)
{
    NRF_UARTE_Type *uart = NRF_UARTE30_S;
    uart->PSEL.TXD   = UART_TX_PIN;
    uart->BAUDRATE   = UART_BAUDRATE_115200;
    uart->CONFIG     = 0u;
    uart->ENABLE     = 8u;
}

static void tm_set_system_handler_priorities(void)
{
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xFFu << 16);
    v |= (0xFFu << 24);
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    NVIC_IPR[SW_IRQ_N] = 0xC0u;
    NVIC_ISER0 = (1u << SW_IRQ_N);
}

void tm_cause_interrupt(void)
{
    NVIC_STIR = SW_IRQ_N;
}

void tm_putchar(int c)
{
    char ch = (char)c;
    tm_uart_send(&ch, 1u);
}

void SWI00_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}


struct TmThreadDesc {
    bool            allocated;
    bool            started;
    uint8_t         tm_priority;
    void          (*entry)(void);
    pthread_t       thread;
    pthread_attr_t  attr;
    sem_t           start_gate;
    uint8_t         stack[TM_RTOS_STACK_BYTES] __attribute__((aligned(8)));
};

static TmThreadDesc  g_threads[TM_RTOS_MAX_THREADS];
static sem_t           g_semaphores[TM_RTOS_MAX_SEMAPHORES];
static bool            g_semaphore_created[TM_RTOS_MAX_SEMAPHORES];
static pthread_mutex_t g_mutexes[TM_RTOS_MAX_MUTEXES];
static bool            g_mutex_created[TM_RTOS_MAX_MUTEXES];
static mqd_t           g_queues[TM_RTOS_MAX_QUEUES];
static bool          g_queue_created[TM_RTOS_MAX_QUEUES];

static void *tm_thread_trampoline(void *arg)
{
    int id = (int)(uintptr_t)arg;
    if (id < 0 || id >= TM_RTOS_MAX_THREADS) return NULL;
    sem_wait(&g_threads[id].start_gate);
    g_threads[id].started = true;
    if (g_threads[id].entry) {
        g_threads[id].entry();
    }
    return NULL;
}

static inline int tm_to_px5_priority(int tm_priority)
{
    if (tm_priority < 1) tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return tm_priority;
}

void tm_initialize(void (*test_initialization_function)(void))
{
    for (int i = 0; i < TM_RTOS_MAX_THREADS; ++i) {
        g_threads[i].allocated = false;
        g_threads[i].started = false;
        g_threads[i].tm_priority = 31u;
        g_threads[i].entry = NULL;
    }
    for (int i = 0; i < TM_RTOS_MAX_QUEUES; ++i) {
        g_queue_created[i] = false;
    }
    for (int i = 0; i < TM_RTOS_MAX_SEMAPHORES; ++i) {
        g_semaphore_created[i] = false;
    }
    for (int i = 0; i < TM_RTOS_MAX_MUTEXES; ++i) {
        g_mutex_created[i] = false;
    }
    for (int i = 0; i < TM_RTOS_MAX_POOLS; ++i) {
        g_pool_created[i] = false;
        g_pool_free[i] = NULL;
    }

    tm_set_system_handler_priorities();
    tm_hw_console_init();
    test_initialization_function();
    tm_enable_software_interrupt();
    for (;;) { sleep(1); }
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    if (thread_id < 0 || thread_id >= TM_RTOS_MAX_THREADS || priority < 1 || priority > 31 || entry_function == NULL)
        return TM_ERROR;
    TmThreadDesc *t = &g_threads[thread_id];
    t->allocated = true;
    t->started = false;
    t->tm_priority = (uint8_t)priority;
    t->entry = entry_function;
    sem_init(&t->start_gate, 0, 0);
    pthread_attr_init(&t->attr);
    pthread_attr_setstackaddr(&t->attr, t->stack);
    pthread_attr_setstacksize(&t->attr, sizeof(t->stack));
    px5_pthread_attr_setpriority(&t->attr, tm_to_px5_priority(priority));
    if (pthread_create(&t->thread, &t->attr, tm_thread_trampoline, (void*)(uintptr_t)thread_id) != 0)
        return TM_ERROR;
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_RTOS_MAX_THREADS || !g_threads[thread_id].allocated) return TM_ERROR;
    if (!g_threads[thread_id].started) {
        return (sem_post(&g_threads[thread_id].start_gate) == 0) ? TM_SUCCESS : TM_ERROR;
    }
    return (px5_pthread_resume(g_threads[thread_id].thread) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_RTOS_MAX_THREADS || !g_threads[thread_id].allocated) return TM_ERROR;
    if (!g_threads[thread_id].started) return TM_SUCCESS;
    return (px5_pthread_suspend(g_threads[thread_id].thread) == 0) ? TM_SUCCESS : TM_ERROR;
}

void tm_thread_relinquish(void) { sched_yield(); }

void tm_thread_sleep(int seconds) { sleep((unsigned int)((seconds <= 0) ? 0 : seconds)); }

int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_RTOS_MAX_QUEUES) return TM_ERROR;
    char qname[8];
    snprintf(qname, sizeof(qname), "/tmq%d", queue_id);
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = TM_RTOS_QUEUE_DEPTH;
    attr.mq_msgsize = TM_RTOS_QUEUE_MSG_SIZE;
    g_queues[queue_id] = mq_open(qname, O_CREAT | O_RDWR, 0, &attr);
    g_queue_created[queue_id] = (g_queues[queue_id] != (mqd_t)-1);
    return g_queue_created[queue_id] ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_RTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL) return TM_ERROR;
    return (mq_send(g_queues[queue_id], (char*)message_ptr, TM_RTOS_QUEUE_MSG_SIZE, 0) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_RTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL) return TM_ERROR;
    return (mq_receive(g_queues[queue_id], (char*)message_ptr, TM_RTOS_QUEUE_MSG_SIZE, NULL) >= 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_RTOS_MAX_SEMAPHORES) return TM_ERROR;
    g_semaphore_created[semaphore_id] = (sem_init(&g_semaphores[semaphore_id], 0, 1) == 0);
    return g_semaphore_created[semaphore_id] ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_RTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id]) return TM_ERROR;
    return (sem_trywait(&g_semaphores[semaphore_id]) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_RTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id]) return TM_ERROR;
    return (sem_post(&g_semaphores[semaphore_id]) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_RTOS_MAX_MUTEXES) return TM_ERROR;
    g_mutex_created[mutex_id] = (pthread_mutex_init(&g_mutexes[mutex_id], NULL) == 0);
    return g_mutex_created[mutex_id] ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_RTOS_MAX_MUTEXES || !g_mutex_created[mutex_id]) return TM_ERROR;
    return (pthread_mutex_lock(&g_mutexes[mutex_id]) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_RTOS_MAX_MUTEXES || !g_mutex_created[mutex_id]) return TM_ERROR;
    return (pthread_mutex_unlock(&g_mutexes[mutex_id]) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    if (pool_id < 0 || pool_id >= TM_RTOS_MAX_POOLS) return TM_ERROR;
    uint8_t *base = g_pool_area[pool_id];
    g_pool_free[pool_id] = (void*)base;
    for (int i = 0; i < (int)TM_BLOCK_COUNT - 1; ++i) {
        void **block = (void**)(base + (uint32_t)i * TM_BLOCK_SIZE);
        *block = (void*)(base + (uint32_t)(i + 1) * TM_BLOCK_SIZE);
    }
    *((void**)(base + (uint32_t)(TM_BLOCK_COUNT - 1) * TM_BLOCK_SIZE)) = NULL;
    g_pool_created[pool_id] = true;
    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_RTOS_MAX_POOLS || !g_pool_created[pool_id] || memory_ptr == NULL) return TM_ERROR;
    void *block = g_pool_free[pool_id];
    if (block == NULL) return TM_ERROR;
    g_pool_free[pool_id] = *((void**)block);
    *memory_ptr = (unsigned char*)block;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_RTOS_MAX_POOLS || !g_pool_created[pool_id] || memory_ptr == NULL) return TM_ERROR;
    *((void**)memory_ptr) = g_pool_free[pool_id];
    g_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}
