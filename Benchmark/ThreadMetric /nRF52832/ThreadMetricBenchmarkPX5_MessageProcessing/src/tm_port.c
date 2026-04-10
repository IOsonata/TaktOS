#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include "tm_api.h"

#define TM_PX5_MAX_THREADS        10
#define TM_PX5_MAX_QUEUES         1
#define TM_PX5_MAX_SEMAPHORES     1
#define TM_PX5_MAX_POOLS          1

#define TM_PX5_STACK_BYTES        1024u
#define TM_PX5_QUEUE_DEPTH        10u
#define TM_PX5_QUEUE_MSG_SIZE     (4u * sizeof(unsigned long))
#define TM_PX5_TICK_HZ            1000u
#define TM_PX5_MEMORY_WORDS       4096u
#define TM_PX5_RUN_TIME_ID        0xBD93A508UL
#define TM_PX5_CORE_CLOCK_HZ      64000000u

#define TM_BLOCK_SIZE             128u
#define TM_POOL_SIZE              2048u

#define UARTE0_BASE               0x40002000u
#define UARTE0_STARTTX            (*(volatile uint32_t*)(UARTE0_BASE + 0x008u))
#define UARTE0_STOPTX             (*(volatile uint32_t*)(UARTE0_BASE + 0x00Cu))
#define UARTE0_ENDTX              (*(volatile uint32_t*)(UARTE0_BASE + 0x120u))
#define UARTE0_TXD_PTR            (*(volatile uint32_t*)(UARTE0_BASE + 0x544u))
#define UARTE0_TXD_MAXCNT         (*(volatile uint32_t*)(UARTE0_BASE + 0x548u))
#define UARTE0_ENABLE             (*(volatile uint32_t*)(UARTE0_BASE + 0x500u))
#define UARTE0_PSEL_TXD           (*(volatile uint32_t*)(UARTE0_BASE + 0x50Cu))
#define UARTE0_BAUDRATE           (*(volatile uint32_t*)(UARTE0_BASE + 0x524u))
#define UARTE0_CONFIG             (*(volatile uint32_t*)(UARTE0_BASE + 0x56Cu))
#define NRF52_GPIO_BASE           0x50000000u
#define GPIO_DIRSET               (*(volatile uint32_t*)(NRF52_GPIO_BASE + 0x518u))
#define GPIO_OUTSET               (*(volatile uint32_t*)(NRF52_GPIO_BASE + 0x508u))
#define UART_TX_PIN               7u
#define UART_BAUDRATE_115200      0x01D7E000u

#define SCB_SHPR3                 (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER0                (*(volatile uint32_t*)0xE000E100u)
#define NVIC_IPR5                 (*(volatile uint32_t*)0xE000E414u)
#define NVIC_STIR                 (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                  21u
#define TM_SOFTIRQ_PRIORITY_BYTE  0xC0u
#define SYST_CSR                 (*(volatile uint32_t*)0xE000E010u)
#define SYST_RVR                 (*(volatile uint32_t*)0xE000E014u)
#define SYST_CVR                 (*(volatile uint32_t*)0xE000E018u)

extern unsigned char _Proc_Stack_Base[];
extern unsigned char _Proc_Stack_Limit[];
void px5_timer_interrupt_process(void);

typedef struct {
    pthread_t handle;
    bool allocated;
    bool priority_applied;
    int mapped_priority;
    uint8_t stack[TM_PX5_STACK_BYTES] __attribute__((aligned(8)));
} tm_px5_thread_t;

static tm_px5_thread_t         g_tm_thread[TM_PX5_MAX_THREADS];
static void                  (*g_tm_entry[TM_PX5_MAX_THREADS])(void);
static pthread_fastqueue_t     g_tm_queue[TM_PX5_MAX_QUEUES];
static bool                    g_tm_queue_created[TM_PX5_MAX_QUEUES];
static sem_t                   g_tm_sem[TM_PX5_MAX_SEMAPHORES];
static bool                    g_tm_sem_created[TM_PX5_MAX_SEMAPHORES];
static pthread_partitionpool_t g_tm_pool[TM_PX5_MAX_POOLS];
static bool                    g_tm_pool_created[TM_PX5_MAX_POOLS];
static unsigned char           g_tm_pool_area[TM_PX5_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static u_long                  g_px5_memory_area[TM_PX5_MEMORY_WORDS];
static char                    g_uart_buf[128];

void tm_interrupt_handler(void) __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void) {}
void tm_interrupt_preemption_handler(void) {}

static int tm_map_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (int)(PX5_MAXIMUM_PRIORITIES - tm_priority);
}

static void *tm_thread_trampoline(void *arg)
{
    int id = (int)(uintptr_t)arg;
    if (id >= 0 && id < TM_PX5_MAX_THREADS && g_tm_entry[id])
    {
        g_tm_entry[id]();
    }
    for (;;) { sched_yield(); }
}

static void tm_uart_send(const char *buf, uint32_t len)
{
    while (len > 0u)
    {
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

static void tm_set_system_handler_priorities(void)
{
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xFFu << 16);
    v |= (0xFFu << 24);
    SCB_SHPR3 = v;
}

static void tm_start_systick(void)
{
    SYST_RVR = (TM_PX5_CORE_CLOCK_HZ / TM_PX5_TICK_HZ) - 1u;
    SYST_CVR = 0u;
    SYST_CSR = 0x07u;
}

static void tm_enable_software_interrupt(void)
{
    NVIC_IPR5 &= ~(0xFFu << 8);
    NVIC_IPR5 |=  (TM_SOFTIRQ_PRIORITY_BYTE << 8);
    NVIC_ISER0 = (1u << SW_IRQ_N);
}

static void tm_px5_prepare_process_stack(void)
{
    __asm volatile (
        "cpsid i        \n"
        "msr psp, %0    \n"
        "movs r1, #2    \n"
        "msr control, r1\n"
        "isb            \n"
        "cpsie i        \n"
        : : "r"(_Proc_Stack_Limit) : "r1", "memory");
}

void tm_initialize(void (*test_initialization_function)(void))
{
    int i;
    int old_prio;

    for (i = 0; i < TM_PX5_MAX_THREADS; ++i)
    {
        memset(&g_tm_thread[i], 0, sizeof(g_tm_thread[i]));
        g_tm_thread[i].priority_applied = false;
        g_tm_thread[i].mapped_priority = 0;
        g_tm_entry[i] = NULL;
    }
    for (i = 0; i < TM_PX5_MAX_QUEUES; ++i) g_tm_queue_created[i] = false;
    for (i = 0; i < TM_PX5_MAX_SEMAPHORES; ++i) g_tm_sem_created[i] = false;
    for (i = 0; i < TM_PX5_MAX_POOLS; ++i) g_tm_pool_created[i] = false;

    tm_set_system_handler_priorities();
    tm_start_systick();
    tm_enable_software_interrupt();
    tm_px5_prepare_process_stack();

    if (px5_pthread_start(TM_PX5_RUN_TIME_ID, g_px5_memory_area, sizeof(g_px5_memory_area)) != PX5_SUCCESS)
    {
        tm_check_fail("FATAL: px5_pthread_start failed\n");
    }

    if (px5_pthread_priority_change(pthread_self(), PX5_MAXIMUM_PRIORITIES - 1, &old_prio) != PX5_SUCCESS)
    {
        tm_check_fail("FATAL: bootstrap priority raise failed\n");
    }

    test_initialization_function();

    if (px5_pthread_priority_change(pthread_self(), 0, &old_prio) != PX5_SUCCESS)
    {
        tm_check_fail("FATAL: bootstrap priority lower failed\n");
    }

    sched_yield();
    for (;;) { px5_pthread_tick_sleep((tick_t)(60u * TM_PX5_TICK_HZ)); }
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    pthread_attr_t attr;
    int rc;

    if (thread_id < 0 || thread_id >= TM_PX5_MAX_THREADS ||
        priority < 1 || priority > 31 || entry_function == NULL)
    {
        return TM_ERROR;
    }

    g_tm_entry[thread_id] = entry_function;

    rc = pthread_attr_init(&attr);
    if (rc != PX5_SUCCESS)
        return TM_ERROR;

    (void)pthread_attr_setstackaddr(&attr, g_tm_thread[thread_id].stack);
    (void)pthread_attr_setstacksize(&attr, sizeof(g_tm_thread[thread_id].stack));
    g_tm_thread[thread_id].mapped_priority = tm_map_priority(priority);
    (void)px5_pthread_attr_setpriority(&attr, 0);

    rc = pthread_create(&g_tm_thread[thread_id].handle,
                        &attr,
                        tm_thread_trampoline,
                        (void *)(uintptr_t)thread_id);
    (void)pthread_attr_destroy(&attr);
    if (rc != PX5_SUCCESS)
        return TM_ERROR;

    g_tm_thread[thread_id].allocated = true;
    g_tm_thread[thread_id].priority_applied = false;
    return (px5_pthread_suspend(g_tm_thread[thread_id].handle) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_resume(int thread_id)
{
    int old_prio;

    if (thread_id < 0 || thread_id >= TM_PX5_MAX_THREADS || !g_tm_thread[thread_id].allocated)
        return TM_ERROR;

    if (!g_tm_thread[thread_id].priority_applied)
    {
        if (px5_pthread_priority_change(g_tm_thread[thread_id].handle,
                                        g_tm_thread[thread_id].mapped_priority,
                                        &old_prio) != PX5_SUCCESS)
            return TM_ERROR;
        g_tm_thread[thread_id].priority_applied = true;
    }

    return (px5_pthread_resume(g_tm_thread[thread_id].handle) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_PX5_MAX_THREADS || !g_tm_thread[thread_id].allocated)
        return TM_ERROR;
    return (px5_pthread_suspend(g_tm_thread[thread_id].handle) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

void tm_thread_relinquish(void)
{
    sched_yield();
}

void tm_thread_sleep(int seconds)
{
    tick_t ticks = (seconds <= 0) ? (tick_t)0 : (tick_t)((unsigned int)seconds * TM_PX5_TICK_HZ);
    (void)px5_pthread_tick_sleep(ticks);
}

int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_PX5_MAX_QUEUES)
        return TM_ERROR;
    if (px5_pthread_fastqueue_create(&g_tm_queue[queue_id], NULL, TM_PX5_QUEUE_MSG_SIZE, TM_PX5_QUEUE_DEPTH) != PX5_SUCCESS)
        return TM_ERROR;
    g_tm_queue_created[queue_id] = true;
    return TM_SUCCESS;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_PX5_MAX_QUEUES || !g_tm_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;
    return (px5_pthread_fastqueue_trysend(&g_tm_queue[queue_id], (u_long*)message_ptr, TM_PX5_QUEUE_MSG_SIZE) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_PX5_MAX_QUEUES || !g_tm_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;
    return (px5_pthread_fastqueue_tryreceive(&g_tm_queue[queue_id], (u_long*)message_ptr, TM_PX5_QUEUE_MSG_SIZE) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_PX5_MAX_SEMAPHORES)
        return TM_ERROR;
    if (sem_init(&g_tm_sem[semaphore_id], 0, 1u) != PX5_SUCCESS)
        return TM_ERROR;
    g_tm_sem_created[semaphore_id] = true;
    return TM_SUCCESS;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_PX5_MAX_SEMAPHORES || !g_tm_sem_created[semaphore_id])
        return TM_ERROR;
    return (sem_trywait(&g_tm_sem[semaphore_id]) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_PX5_MAX_SEMAPHORES || !g_tm_sem_created[semaphore_id])
        return TM_ERROR;
    return (sem_post(&g_tm_sem[semaphore_id]) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    if (pool_id < 0 || pool_id >= TM_PX5_MAX_POOLS)
        return TM_ERROR;
    if (px5_pthread_partitionpool_create(&g_tm_pool[pool_id], NULL, g_tm_pool_area[pool_id], TM_POOL_SIZE, TM_BLOCK_SIZE) != PX5_SUCCESS)
        return TM_ERROR;
    g_tm_pool_created[pool_id] = true;
    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    void *p = NULL;
    if (pool_id < 0 || pool_id >= TM_PX5_MAX_POOLS || !g_tm_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;
    if (px5_pthread_partitionpool_tryallocate(&g_tm_pool[pool_id], &p, TM_BLOCK_SIZE) != PX5_SUCCESS || p == NULL)
        return TM_ERROR;
    *memory_ptr = (unsigned char*)p;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    (void)pool_id;
    if (memory_ptr == NULL)
        return TM_ERROR;
    return (px5_pthread_partitionpool_free((void*)memory_ptr) == PX5_SUCCESS) ? TM_SUCCESS : TM_ERROR;
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

void SysTick_Handler(void)
{
    px5_timer_interrupt_process();
}

void SWI1_EGU1_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}
