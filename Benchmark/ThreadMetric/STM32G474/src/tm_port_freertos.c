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

// LPUART1 on NUCLEO-G474RE (PA2/PA3 AF12, routed to ST-LINK/V3E VCP).
// See tm_console_nucleo_g474.cpp for a clean version of the init; this file
// duplicates the minimal bits because the official Thread-Metric FreeRTOS
// port is a single .c with its own console. Kernel clock defaults to PCLK1.
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
#define USART_CR1_TE                 (1u << 3)
#define USART_CR1_RE                 (1u << 2)
#define USART_ISR_TXE                (1u << 7)

extern uint32_t SystemCoreClock;

#define SCB_SHPR3                    (*(volatile uint32_t*)0xE000ED20u)
// On Cortex-M with byte-addressed NVIC IPR, we use the byte array pattern.
#define NVIC_IPR_BYTE                ((volatile uint8_t*) 0xE000E400u)
#define NVIC_ISER                    ((volatile uint32_t*)0xE000E100u)
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     55u   /* TIM7_DAC_IRQn on STM32G474 */
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
    uint8_t prio_field = (uint8_t)(TM_SOFTIRQ_LIBRARY_PRIORITY << (8u - configPRIO_BITS));
    NVIC_IPR_BYTE[SW_IRQ_N] = prio_field;
    NVIC_ISER[SW_IRQ_N >> 5] = (1u << (SW_IRQ_N & 31u));
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

void TIM7_DAC_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

// -----------------------------------------------------------------------------
// Required FreeRTOS hooks for static allocation (no heap).
// -----------------------------------------------------------------------------
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t idleTCB;
    static StackType_t  idleStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &idleTCB;
    *ppxIdleTaskStackBuffer = idleStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
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
