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

/* NUCLEO-G474RE: LPUART1 on PA2/PA3 AF12, routed to ST-LINK/V3E VCP.
 * Kernel clock = PCLK1 (default). BRR = 256 * fck / baud. */
#define RCC_BASE                     0x40021000u
#define RCC_AHB2ENR                  (*(volatile uint32_t*)(RCC_BASE + 0x4Cu))
#define RCC_APB1ENR2                 (*(volatile uint32_t*)(RCC_BASE + 0x5Cu))
#define RCC_AHB2ENR_GPIOAEN          (1u << 0)
#define RCC_APB1ENR2_LPUART1EN       (1u << 0)

#define GPIOA_BASE                   0x48000000u
#define GPIOA_MODER                  (*(volatile uint32_t*)(GPIOA_BASE + 0x00u))
#define GPIOA_OSPEEDR                (*(volatile uint32_t*)(GPIOA_BASE + 0x08u))
#define GPIOA_AFRL                   (*(volatile uint32_t*)(GPIOA_BASE + 0x20u))

#define LPUART1_BASE                 0x40008000u
#define LPUART1_CR1                  (*(volatile uint32_t*)(LPUART1_BASE + 0x00u))
#define LPUART1_BRR                  (*(volatile uint32_t*)(LPUART1_BASE + 0x0Cu))
#define LPUART1_ISR                  (*(volatile uint32_t*)(LPUART1_BASE + 0x1Cu))
#define LPUART1_TDR                  (*(volatile uint32_t*)(LPUART1_BASE + 0x28u))
#define USART_CR1_UE                 (1u << 0)
#define USART_CR1_RE                 (1u << 2)
#define USART_CR1_TE                 (1u << 3)
#define USART_ISR_TXE                (1u << 7)

extern uint32_t SystemCoreClock;

/* NVIC soft-IRQ on TIM7_DAC_IRQn = 55 (STM32G474). */
#define NVIC_IPR_BYTE                ((volatile uint8_t*) 0xE000E400u)
#define NVIC_ISER                    ((volatile uint32_t*)0xE000E100u)
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     55u

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
    uint32_t timeout;
    while (len > 0u) {
        timeout = 20000000u;
        while ((LPUART1_ISR & USART_ISR_TXE) == 0u) {
            if (--timeout == 0u) return;
        }
        LPUART1_TDR = (uint32_t)(*(const uint8_t*)buf);
        ++buf;
        --len;
    }
}

void tm_hw_console_init(void)
{
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    (void)RCC_APB1ENR2;

    GPIOA_MODER   &= ~((3u << (2*2)) | (3u << (3*2)));
    GPIOA_MODER   |=  ((2u << (2*2)) | (2u << (3*2)));
    GPIOA_OSPEEDR |=  ((3u << (2*2)) | (3u << (3*2)));
    GPIOA_AFRL    &= ~((0xFu << (2*4)) | (0xFu << (3*4)));
    GPIOA_AFRL    |=  ((0xCu << (2*4)) | (0xCu << (3*4)));

    LPUART1_CR1 = 0u;
    LPUART1_BRR = (uint32_t)((256ULL * (uint64_t)SystemCoreClock
                              + 115200ULL / 2ULL) / 115200ULL);
    LPUART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
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
    uint8_t prio_field = (uint8_t)(6u << 4);  /* priority 6, 4 bits on G4 */
    NVIC_IPR_BYTE[SW_IRQ_N] = prio_field;
    NVIC_ISER[SW_IRQ_N >> 5] = (1u << (SW_IRQ_N & 31u));
    NVIC_STIR = SW_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// Polled tm_putchar on LPUART1 (NUCLEO-G474RE, ST-LINK/V3E VCP).
// -----------------------------------------------------------------------------
void tm_putchar(int c)
{
    uint32_t timeout = 20000000u;
    while ((LPUART1_ISR & USART_ISR_TXE) == 0u) {
        if (--timeout == 0u) return;
    }
    LPUART1_TDR = (uint32_t)(c & 0xFFu);
}
