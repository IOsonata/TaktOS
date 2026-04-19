#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "tm_api.h"

// -----------------------------------------------------------------------------
// FreeRTOS bare-metal nRF52832 port for the official Thread-Metric harness.
//
// This keeps the same high-level API mapping as the official sysprog21
// FreeRTOS port, but targets real Cortex-M4 hardware instead of POSIX/QEMU.
// -----------------------------------------------------------------------------

#define TM_FREERTOS_MAX_THREADS      12
#define TM_FREERTOS_MAX_QUEUES       1
#define TM_FREERTOS_MAX_SEMAPHORES   1
#define TM_FREERTOS_MAX_MUTEXES      1
#define TM_FREERTOS_MAX_POOLS        1

#define TM_FREERTOS_STACK_WORDS      1024u
#define TM_FREERTOS_QUEUE_DEPTH      10u
#define TM_FREERTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))
#define TM_RTOS_QUEUE_ULONGS_PER_MSG 4u

#define TM_BLOCK_SIZE                128u
#define TM_POOL_SIZE                 2048u
#define TM_BLOCK_COUNT               (TM_POOL_SIZE / TM_BLOCK_SIZE)

// nRF52 UARTE0 (adjust TX pin for your board if needed)
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

#define SCB_SHPR3                    (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER0                   (*(volatile uint32_t*)0xE000E100u)
#define NVIC_IPR5                    (*(volatile uint32_t*)0xE000E414u)
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     21u
#define TM_SOFTIRQ_LIBRARY_PRIORITY  6u

static TaskHandle_t      g_tm_thread[TM_FREERTOS_MAX_THREADS];
static StaticTask_t      g_tm_tcb[TM_FREERTOS_MAX_THREADS];
static StackType_t       g_tm_stack[TM_FREERTOS_MAX_THREADS][TM_FREERTOS_STACK_WORDS];
static void            (*g_tm_entry[TM_FREERTOS_MAX_THREADS])(void);

static QueueHandle_t     g_tm_queue[TM_FREERTOS_MAX_QUEUES];
static StaticQueue_t     g_tm_queue_struct[TM_FREERTOS_MAX_QUEUES];
static uint8_t           g_tm_queue_storage[TM_FREERTOS_MAX_QUEUES]
                                           [TM_FREERTOS_QUEUE_DEPTH * TM_FREERTOS_QUEUE_MSG_SIZE]
                                           __attribute__((aligned(4)));

static SemaphoreHandle_t g_tm_sem[TM_FREERTOS_MAX_SEMAPHORES];
static StaticSemaphore_t g_tm_sem_struct[TM_FREERTOS_MAX_SEMAPHORES];
static SemaphoreHandle_t g_tm_mutex[TM_FREERTOS_MAX_MUTEXES];
static StaticSemaphore_t g_tm_mutex_struct[TM_FREERTOS_MAX_MUTEXES];

static uint8_t           g_tm_pool_area[TM_FREERTOS_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static void             *g_tm_pool_free[TM_FREERTOS_MAX_POOLS];

static char              g_uart_buf[128];

void tm_interrupt_handler(void) __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void) {}
void tm_interrupt_preemption_handler(void) {}

static BaseType_t tm_in_isr(void)
{
    uint32_t ipsr;
    __asm volatile ("MRS %0, IPSR" : "=r"(ipsr));
    return (ipsr != 0u) ? pdTRUE : pdFALSE;
}

static void tm_task_trampoline(void *param)
{
    int id = (int)(uintptr_t)param;
    if (id >= 0 && id < TM_FREERTOS_MAX_THREADS && g_tm_entry[id] != NULL) {
        g_tm_entry[id]();
    }
    vTaskDelete(NULL);
}

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

static UBaseType_t tm_map_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    // Match the official FreeRTOS port mapping.
    return (UBaseType_t)((configMAX_PRIORITIES - 1) - tm_priority);
}

static void tm_set_kernel_priorities(void)
{
    volatile uint32_t *shpr3 = (volatile uint32_t*)0xE000ED20u;
    *shpr3 |= (configKERNEL_INTERRUPT_PRIORITY << 16) |
              (configKERNEL_INTERRUPT_PRIORITY << 24);
}

static void tm_enable_software_interrupt(void)
{
    uint32_t prio_field = (uint32_t)(TM_SOFTIRQ_LIBRARY_PRIORITY << (8u - configPRIO_BITS));
    NVIC_IPR5 &= ~(0xFFu << 8);
    NVIC_IPR5 |=  (prio_field << 8);
    NVIC_ISER0 = (1u << SW_IRQ_N);
}

void tm_initialize(void (*test_initialization_function)(void))
{
    int i;

    for (i = 0; i < TM_FREERTOS_MAX_THREADS; ++i) {
        g_tm_thread[i] = NULL;
        g_tm_entry[i] = NULL;
    }
    for (i = 0; i < TM_FREERTOS_MAX_QUEUES; ++i) {
        g_tm_queue[i] = NULL;
    }
    for (i = 0; i < TM_FREERTOS_MAX_SEMAPHORES; ++i) {
        g_tm_sem[i] = NULL;
    }
    for (i = 0; i < TM_FREERTOS_MAX_MUTEXES; ++i) {
        g_tm_mutex[i] = NULL;
    }
    for (i = 0; i < TM_FREERTOS_MAX_POOLS; ++i) {
        g_tm_pool_free[i] = NULL;
    }

    tm_set_kernel_priorities();
    tm_enable_software_interrupt();

    test_initialization_function();

    vTaskStartScheduler();
    tm_check_fail("FATAL: scheduler failed to start\n");
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    TaskHandle_t h;

    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS ||
        priority < 1 || priority > 31 || entry_function == NULL) {
        return TM_ERROR;
    }

    g_tm_entry[thread_id] = entry_function;

    h = xTaskCreateStatic(tm_task_trampoline,
                          "TM",
                          TM_FREERTOS_STACK_WORDS,
                          (void*)(uintptr_t)thread_id,
                          tm_map_priority(priority),
                          g_tm_stack[thread_id],
                          &g_tm_tcb[thread_id]);
    if (h == NULL) {
        return TM_ERROR;
    }

    g_tm_thread[thread_id] = h;
    vTaskSuspend(h);   // create suspended, matching official semantics
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS || g_tm_thread[thread_id] == NULL) {
        return TM_ERROR;
    }

    if (tm_in_isr()) {
        BaseType_t yield = xTaskResumeFromISR(g_tm_thread[thread_id]);
        portYIELD_FROM_ISR(yield);
        return TM_SUCCESS;
    }

    vTaskResume(g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS || g_tm_thread[thread_id] == NULL) {
        return TM_ERROR;
    }

    vTaskSuspend(g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

void tm_thread_relinquish(void)
{
    taskYIELD();
}

void tm_thread_sleep(int seconds)
{
    vTaskDelay(pdMS_TO_TICKS((uint32_t)seconds * 1000u));
}
void tm_thread_sleep_ticks(int ticks)
{
    vTaskDelay((TickType_t)(ticks <= 0 ? 1 : ticks));
}


int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_FREERTOS_MAX_QUEUES) {
        return TM_ERROR;
    }

    g_tm_queue[queue_id] = xQueueCreateStatic(TM_FREERTOS_QUEUE_DEPTH,
                                              TM_FREERTOS_QUEUE_MSG_SIZE,
                                              g_tm_queue_storage[queue_id],
                                              &g_tm_queue_struct[queue_id]);
    return (g_tm_queue[queue_id] != NULL) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_FREERTOS_MAX_QUEUES || g_tm_queue[queue_id] == NULL) {
        return TM_ERROR;
    }

    return (xQueueSendToBack(g_tm_queue[queue_id], (const void*)message_ptr, 0) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_FREERTOS_MAX_QUEUES || g_tm_queue[queue_id] == NULL) {
        return TM_ERROR;
    }

    return (xQueueReceive(g_tm_queue[queue_id], (void*)message_ptr, 0) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES) {
        return TM_ERROR;
    }

    g_tm_sem[semaphore_id] = xSemaphoreCreateBinaryStatic(&g_tm_sem_struct[semaphore_id]);
    if (g_tm_sem[semaphore_id] == NULL) {
        return TM_ERROR;
    }

    xSemaphoreGive(g_tm_sem[semaphore_id]);  // start available
    return TM_SUCCESS;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES || g_tm_sem[semaphore_id] == NULL) {
        return TM_ERROR;
    }

    return (xSemaphoreTake(g_tm_sem[semaphore_id], 0) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES || g_tm_sem[semaphore_id] == NULL) {
        return TM_ERROR;
    }

    if (tm_in_isr()) {
        BaseType_t yield = pdFALSE;
        if (xSemaphoreGiveFromISR(g_tm_sem[semaphore_id], &yield) != pdTRUE) {
            return TM_ERROR;
        }
        portYIELD_FROM_ISR(yield);
        return TM_SUCCESS;
    }

    return (xSemaphoreGive(g_tm_sem[semaphore_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES) {
        return TM_ERROR;
    }

    g_tm_mutex[mutex_id] = xSemaphoreCreateMutexStatic(&g_tm_mutex_struct[mutex_id]);
    return (g_tm_mutex[mutex_id] != NULL) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES || g_tm_mutex[mutex_id] == NULL) {
        return TM_ERROR;
    }

    return (xSemaphoreTake(g_tm_mutex[mutex_id], portMAX_DELAY) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES || g_tm_mutex[mutex_id] == NULL) {
        return TM_ERROR;
    }

    return (xSemaphoreGive(g_tm_mutex[mutex_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    int i;
    uint8_t *base;

    if (pool_id < 0 || pool_id >= TM_FREERTOS_MAX_POOLS) {
        return TM_ERROR;
    }

    base = g_tm_pool_area[pool_id];
    g_tm_pool_free[pool_id] = (void*)base;

    for (i = 0; i < (int)TM_BLOCK_COUNT - 1; ++i) {
        void **block = (void**)(base + ((uint32_t)i * TM_BLOCK_SIZE));
        *block = (void*)(base + ((uint32_t)(i + 1) * TM_BLOCK_SIZE));
    }
    {
        void **last = (void**)(base + ((uint32_t)(TM_BLOCK_COUNT - 1) * TM_BLOCK_SIZE));
        *last = NULL;
    }

    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    void *block;

    if (pool_id < 0 || pool_id >= TM_FREERTOS_MAX_POOLS || memory_ptr == NULL) {
        return TM_ERROR;
    }

    block = g_tm_pool_free[pool_id];
    if (block == NULL) {
        return TM_ERROR;
    }

    g_tm_pool_free[pool_id] = *((void**)block);
    *memory_ptr = (unsigned char*)block;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_FREERTOS_MAX_POOLS || memory_ptr == NULL) {
        return TM_ERROR;
    }

    *((void**)memory_ptr) = g_tm_pool_free[pool_id];
    g_tm_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

void tm_cause_interrupt(void)
{
    NVIC_STIR = SW_IRQ_N;
}

// -----------------------------------------------------------------------------
// Non-Blocking Asynchronous UART via nRF52 EasyDMA
// -----------------------------------------------------------------------------
// Output buffer.  Sized to hold one full report line with margin.
// tm_uart_flush() is synchronous  it waits for DMA completion and issues
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

void SWI1_EGU1_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t idleTCB;
    static StackType_t  idleStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer = &idleTCB;
    *ppxIdleTaskStackBuffer = idleStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}
