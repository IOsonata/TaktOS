
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "tm_api.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

// -----------------------------------------------------------------------------
// nRF54L15 app-core console / soft-IRQ helpers — taken from ThreadX port.
// -----------------------------------------------------------------------------
#include "nrf.h"

#define TM_RTOS_MAX_THREADS          10
#define TM_RTOS_MAX_QUEUES           1
#define TM_RTOS_MAX_SEMAPHORES       1
#define TM_RTOS_MAX_POOLS            1

#define TM_RTOS_STACK_BYTES          1024u
#define TM_RTOS_TICK_HZ              1000u
#define TM_RTOS_CORE_CLOCK_HZ        128000000u

#define TM_RTOS_QUEUE_DEPTH          10u
#define TM_RTOS_QUEUE_MSG_SIZE       (4u * sizeof(unsigned long))
#define TM_RTOS_QUEUE_ULONGS_PER_MSG 4u

#define TM_BLOCK_SIZE                128u
#define TM_POOL_SIZE                 2048u
#define TM_BLOCK_COUNT               (TM_POOL_SIZE / TM_BLOCK_SIZE)

#define UART_TX_PIN                  0u
#define UART_BAUDRATE_115200         0x01D7E000u

#define SCB_SHPR3                    (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER0                   (*(volatile uint32_t*)0xE000E100u)
#define NVIC_IPR                     ((volatile uint8_t*)0xE000E400u)
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     28u

static uint8_t  g_pool_area[TM_RTOS_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static void    *g_pool_free[TM_RTOS_MAX_POOLS];
static bool     g_pool_created[TM_RTOS_MAX_POOLS];
static char     g_uart_buf[128];

void tm_interrupt_handler(void) __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void) {}
void tm_interrupt_preemption_handler(void) {}

// ── UART helpers (identical to ThreadX / TaktOS ports) ────────────────────────

static void tm_uart_send(const char *buf, uint32_t len)
{
    NRF_UARTE_Type *uart = NRF_UARTE30_S;
    while (len > 0u) {
        uint32_t chunk = (len > sizeof(g_uart_buf)) ? (uint32_t)sizeof(g_uart_buf) : len;
        for (uint32_t i = 0u; i < chunk; ++i) {
            g_uart_buf[i] = buf[i];
        }
        uart->EVENTS_DMA.TX.END  = 0u;
        uart->DMA.TX.PTR         = (uint32_t)(uintptr_t)g_uart_buf;
        uart->DMA.TX.MAXCNT      = chunk;
        uart->TASKS_DMA.TX.START = 1u;
        while (!uart->EVENTS_DMA.TX.END) {}
        uart->TASKS_DMA.TX.STOP  = 1u;
        buf += chunk;
        len -= chunk;
    }
}

void tm_hw_console_init(void)
{
    NRF_UARTE_Type *uart = NRF_UARTE30_S;
    uart->PSEL.TXD  = UART_TX_PIN;
    uart->BAUDRATE  = UART_BAUDRATE_115200;
    uart->CONFIG    = 0u;
    uart->ENABLE    = 8u;
}

// ── System handler and IRQ priorities ────────────────────────────────────────

static void tm_set_system_handler_priorities(void)
{
    // PendSV = SHPR3[23:16], SysTick = SHPR3[31:24] — both lowest priority (0xFF).
    // FreeRTOS xPortStartScheduler() will also OR these in; setting them here
    // ensures they are correct before vTaskStartScheduler() is called.
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xFFu << 16);
    v |= (0xFFu << 24);
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    // IRQ28 at priority 0xC0 — above PendSV/SysTick so the ISR runs first.
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

// ── Thread descriptors ───────────────────────────────────────────────────────

typedef struct TmThreadDesc_s {
    bool         allocated;
    bool         start_resumed;
    uint8_t      tm_priority;
    void       (*entry)(void);
    TaskHandle_t handle;
    StaticTask_t tcb;
    StackType_t  stack[TM_RTOS_STACK_BYTES / sizeof(StackType_t)];
} TmThreadDesc_t;

static TmThreadDesc_t     g_threads[TM_RTOS_MAX_THREADS];
static StaticQueue_t     g_queue_ctrl[TM_RTOS_MAX_QUEUES];
static uint8_t           g_queue_storage[TM_RTOS_MAX_QUEUES]
                                        [TM_RTOS_QUEUE_DEPTH * TM_RTOS_QUEUE_MSG_SIZE]
                                        __attribute__((aligned(4)));
static QueueHandle_t     g_queues[TM_RTOS_MAX_QUEUES];
static bool              g_queue_created[TM_RTOS_MAX_QUEUES];
static StaticSemaphore_t g_sem_ctrl[TM_RTOS_MAX_SEMAPHORES];
static SemaphoreHandle_t g_semaphores[TM_RTOS_MAX_SEMAPHORES];
static bool              g_semaphore_created[TM_RTOS_MAX_SEMAPHORES];
static bool              g_scheduler_started = false;

static inline UBaseType_t tm_to_freertos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (UBaseType_t)(((31 - tm_priority) * (configMAX_PRIORITIES - 1u)) / 30u);
}

static void tm_thread_trampoline(void *arg)
{
    int id = (int)(uintptr_t)arg;
    if (id >= 0 && id < TM_RTOS_MAX_THREADS && g_threads[id].entry) {
        g_threads[id].entry();
    }
    for (;;) {}
}

static void tm_apply_initial_thread_states(void)
{
    for (int i = 0; i < TM_RTOS_MAX_THREADS; ++i) {
        if (g_threads[i].allocated && g_threads[i].handle != NULL && !g_threads[i].start_resumed) {
            vTaskSuspend(g_threads[i].handle);
        }
    }
}

// ── tm_initialize ─────────────────────────────────────────────────────────────
// Mirrors the ThreadX port structure:
//   set priorities → init console → run test init → start kernel.

void tm_initialize(void (*test_initialization_function)(void))
{
    for (int i = 0; i < TM_RTOS_MAX_THREADS; ++i) {
        g_threads[i].allocated  = false;
        g_threads[i].start_resumed = false;
        g_threads[i].tm_priority = 31u;
        g_threads[i].entry      = NULL;
        g_threads[i].handle     = NULL;
    }
    g_scheduler_started = false;

    for (int i = 0; i < TM_RTOS_MAX_QUEUES; ++i) {
        g_queues[i]       = NULL;
        g_queue_created[i] = false;
    }
    for (int i = 0; i < TM_RTOS_MAX_SEMAPHORES; ++i) {
        g_semaphores[i]       = NULL;
        g_semaphore_created[i] = false;
    }
    for (int i = 0; i < TM_RTOS_MAX_POOLS; ++i) {
        g_pool_created[i] = false;
        g_pool_free[i]    = NULL;
    }
    tm_set_system_handler_priorities();
    tm_hw_console_init();
    test_initialization_function();
    tm_apply_initial_thread_states();
    tm_enable_software_interrupt();
    g_scheduler_started = true;
    vTaskStartScheduler();
    for (;;) {}
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    if (thread_id < 0 || thread_id >= TM_RTOS_MAX_THREADS ||
        priority < 1 || priority > 31 || entry_function == NULL)
        return TM_ERROR;

    TmThreadDesc_t *t  = &g_threads[thread_id];
    t->allocated     = true;
    t->start_resumed = false;
    t->tm_priority   = (uint8_t)priority;
    t->entry         = entry_function;
    t->handle = xTaskCreateStatic(tm_thread_trampoline,
                                  "tm",
                                  (uint32_t)(sizeof(t->stack) / sizeof(StackType_t)),
                                  (void*)(uintptr_t)thread_id,
                                  tm_to_freertos_priority(priority),
                                  t->stack,
                                  &t->tcb);
    if (t->handle == NULL)
        return TM_ERROR;
    if (g_scheduler_started) {
        vTaskSuspend(t->handle);
    }
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_RTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;
    if (!g_scheduler_started) {
        g_threads[thread_id].start_resumed = true;
        return TM_SUCCESS;
    }
    vTaskResume(g_threads[thread_id].handle);
    return TM_SUCCESS;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_RTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;
    if (!g_scheduler_started) {
        g_threads[thread_id].start_resumed = false;
        return TM_SUCCESS;
    }
    vTaskSuspend(g_threads[thread_id].handle);
    return TM_SUCCESS;
}

void tm_thread_relinquish(void) { taskYIELD(); }

void tm_thread_sleep(int seconds)
{
    TickType_t ticks = (seconds <= 0) ? 0u : pdMS_TO_TICKS((uint32_t)seconds * 1000u);
    vTaskDelay(ticks);
}

int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_RTOS_MAX_QUEUES)
        return TM_ERROR;
    g_queues[queue_id] = xQueueCreateStatic(TM_RTOS_QUEUE_DEPTH,
                                            TM_RTOS_QUEUE_MSG_SIZE,
                                            g_queue_storage[queue_id],
                                            &g_queue_ctrl[queue_id]);
    g_queue_created[queue_id] = (g_queues[queue_id] != NULL);
    return g_queue_created[queue_id] ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_RTOS_MAX_QUEUES ||
        !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;
    return (xQueueSend(g_queues[queue_id], message_ptr, 0) == pdPASS) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_RTOS_MAX_QUEUES ||
        !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;
    return (xQueueReceive(g_queues[queue_id], message_ptr, 0) == pdPASS) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_RTOS_MAX_SEMAPHORES)
        return TM_ERROR;
    g_semaphores[semaphore_id] = xSemaphoreCreateCountingStatic(
                                     1u, 1u, &g_sem_ctrl[semaphore_id]);
    g_semaphore_created[semaphore_id] = (g_semaphores[semaphore_id] != NULL);
    return g_semaphore_created[semaphore_id] ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_RTOS_MAX_SEMAPHORES ||
        !g_semaphore_created[semaphore_id])
        return TM_ERROR;
    return (xSemaphoreTake(g_semaphores[semaphore_id], 0) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_RTOS_MAX_SEMAPHORES ||
        !g_semaphore_created[semaphore_id])
        return TM_ERROR;
    return (xSemaphoreGive(g_semaphores[semaphore_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    if (pool_id < 0 || pool_id >= TM_RTOS_MAX_POOLS)
        return TM_ERROR;
    uint8_t *base    = g_pool_area[pool_id];
    g_pool_free[pool_id] = (void*)base;
    for (int i = 0; i < (int)TM_BLOCK_COUNT - 1; ++i) {
        void **block = (void**)(base + (uint32_t)i * TM_BLOCK_SIZE);
        *block       = (void*)(base + (uint32_t)(i + 1) * TM_BLOCK_SIZE);
    }
    *((void**)(base + (uint32_t)(TM_BLOCK_COUNT - 1) * TM_BLOCK_SIZE)) = NULL;
    g_pool_created[pool_id] = true;
    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_RTOS_MAX_POOLS ||
        !g_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;
    void *block = g_pool_free[pool_id];
    if (block == NULL)
        return TM_ERROR;
    g_pool_free[pool_id] = *((void**)block);
    *memory_ptr          = (unsigned char*)block;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_RTOS_MAX_POOLS ||
        !g_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;
    *((void**)memory_ptr) = g_pool_free[pool_id];
    g_pool_free[pool_id]  = (void*)memory_ptr;
    return TM_SUCCESS;
}

