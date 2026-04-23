#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "tm_api.h"

// -----------------------------------------------------------------------------
// FreeRTOS bare-metal STM32H753ZI port for the official Thread-Metric harness.
//
// Target: NUCLEO-H753ZI, STM32H753ZIT6, Cortex-M7 + DP FPU @ 64 MHz (HSI).
// UART  : USART3 on PD8 (AF7), 115200 8N1, ST-LINK/V3 Virtual COM Port.
// Tick  : 1000 Hz.
// SW IRQ: TIM7_IRQn (IRQ 55) - pended only; TIM7 is never clocked.
//
// startup_stm32h753.S, system_stm32h753.c, gcc_stm32h753.ld, and
// tm_console_nucleo_h753.cpp are shared with the TaktOS and ThreadX builds.
// FreeRTOSConfig.h maps PendSV_Handler, SVC_Handler, SysTick_Handler back to
// the FreeRTOS ARM_CM7/r0p1 port implementations, which override the weak
// aliases in startup_stm32h753.S at link time.
// -----------------------------------------------------------------------------

#define TM_FREERTOS_MAX_THREADS      9     /* matches TaktOS MAX_SLOTS on this board */
#define TM_FREERTOS_MAX_QUEUES       1
#define TM_FREERTOS_MAX_SEMAPHORES   1
#define TM_FREERTOS_MAX_MUTEXES      1
#define TM_FREERTOS_MAX_POOLS        1

/* 128 KB DTCM-RAM available. Match the TaktOS port's 2048-byte stacks for
 * apples-to-apples comparison on the same board.
 *   9 threads x 512 words x 4 bytes = 18 KB total stack area.               */
#define TM_FREERTOS_STACK_WORDS      512u
#define TM_FREERTOS_QUEUE_DEPTH      10u
#define TM_FREERTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

#define TM_BLOCK_SIZE                128u
#define TM_POOL_SIZE                 2048u
#define TM_BLOCK_COUNT               (TM_POOL_SIZE / TM_BLOCK_SIZE)

// -----------------------------------------------------------------------------
// NVIC + SCB register addresses (ARMv7-M).
// STM32H7 has 4 implementable priority bits (top 4 of each 8-bit byte).
// -----------------------------------------------------------------------------
#define SCB_SHPR3                    (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER1                   (*(volatile uint32_t*)0xE000E104u)  // IRQ32..63
#define NVIC_IPR_BYTE                ((volatile uint8_t*)0xE000E400UL)   // byte-addressable
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     55u     // TIM7_IRQn on STM32H753xx
/* Priority 0xC0: higher than PendSV/SysTick (0xF0) so the ISR runs promptly,
 * matching the TaktOS H753 port's choice for comparable measurements.     */
#define TM_SOFTIRQ_IPR_BYTE          0xC0u

// -----------------------------------------------------------------------------
// Per-task storage. Static allocation throughout.
// -----------------------------------------------------------------------------
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

static uint8_t           g_tm_pool_area[TM_FREERTOS_MAX_POOLS][TM_POOL_SIZE]
                                        __attribute__((aligned(sizeof(void*))));
static void             *g_tm_pool_free[TM_FREERTOS_MAX_POOLS];

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

static UBaseType_t tm_map_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (UBaseType_t)((configMAX_PRIORITIES - 1) - tm_priority);
}

static void tm_set_kernel_priorities(void)
{
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (uint32_t)(configKERNEL_INTERRUPT_PRIORITY << 16);  // PendSV
    v |= (uint32_t)(configKERNEL_INTERRUPT_PRIORITY << 24);  // SysTick
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    // Byte-addressable NVIC IPR - IRQ55 gets its own byte.
    NVIC_IPR_BYTE[SW_IRQ_N] = TM_SOFTIRQ_IPR_BYTE;
    NVIC_ISER1 = (1u << (SW_IRQ_N - 32u));
}

void tm_initialize(void (*test_initialization_function)(void))
{
    int i;

    for (i = 0; i < TM_FREERTOS_MAX_THREADS; ++i) {
        g_tm_thread[i] = NULL;
        g_tm_entry[i]  = NULL;
    }
    for (i = 0; i < TM_FREERTOS_MAX_QUEUES; ++i)     g_tm_queue[i] = NULL;
    for (i = 0; i < TM_FREERTOS_MAX_SEMAPHORES; ++i) g_tm_sem[i] = NULL;
    for (i = 0; i < TM_FREERTOS_MAX_MUTEXES; ++i)    g_tm_mutex[i] = NULL;
    for (i = 0; i < TM_FREERTOS_MAX_POOLS; ++i)      g_tm_pool_free[i] = NULL;

    tm_set_kernel_priorities();
    tm_enable_software_interrupt();

    test_initialization_function();

    vTaskStartScheduler();
    tm_check_fail("FATAL: FreeRTOS scheduler failed to start\n");
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
    if (h == NULL) return TM_ERROR;

    g_tm_thread[thread_id] = h;
    vTaskSuspend(h);
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS ||
        g_tm_thread[thread_id] == NULL) return TM_ERROR;

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
    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS ||
        g_tm_thread[thread_id] == NULL) return TM_ERROR;
    vTaskSuspend(g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

void tm_thread_relinquish(void)                      { taskYIELD(); }
void tm_thread_sleep(int seconds)                    { vTaskDelay(pdMS_TO_TICKS((uint32_t)seconds * 1000u)); }
void tm_thread_sleep_ticks(int ticks)                { vTaskDelay((TickType_t)(ticks <= 0 ? 1 : ticks)); }

int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_FREERTOS_MAX_QUEUES) return TM_ERROR;
    g_tm_queue[queue_id] = xQueueCreateStatic(TM_FREERTOS_QUEUE_DEPTH,
                                              TM_FREERTOS_QUEUE_MSG_SIZE,
                                              g_tm_queue_storage[queue_id],
                                              &g_tm_queue_struct[queue_id]);
    return (g_tm_queue[queue_id] != NULL) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_FREERTOS_MAX_QUEUES ||
        g_tm_queue[queue_id] == NULL) return TM_ERROR;
    return (xQueueSendToBack(g_tm_queue[queue_id], (const void*)message_ptr, 0) == pdTRUE)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_FREERTOS_MAX_QUEUES ||
        g_tm_queue[queue_id] == NULL) return TM_ERROR;
    return (xQueueReceive(g_tm_queue[queue_id], (void*)message_ptr, 0) == pdTRUE)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES) return TM_ERROR;
    g_tm_sem[semaphore_id] = xSemaphoreCreateBinaryStatic(&g_tm_sem_struct[semaphore_id]);
    if (g_tm_sem[semaphore_id] == NULL) return TM_ERROR;
    xSemaphoreGive(g_tm_sem[semaphore_id]);
    return TM_SUCCESS;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES ||
        g_tm_sem[semaphore_id] == NULL) return TM_ERROR;
    return (xSemaphoreTake(g_tm_sem[semaphore_id], 0) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES ||
        g_tm_sem[semaphore_id] == NULL) return TM_ERROR;

    if (tm_in_isr()) {
        BaseType_t yield = pdFALSE;
        if (xSemaphoreGiveFromISR(g_tm_sem[semaphore_id], &yield) != pdTRUE) return TM_ERROR;
        portYIELD_FROM_ISR(yield);
        return TM_SUCCESS;
    }
    return (xSemaphoreGive(g_tm_sem[semaphore_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES) return TM_ERROR;
    g_tm_mutex[mutex_id] = xSemaphoreCreateMutexStatic(&g_tm_mutex_struct[mutex_id]);
    return (g_tm_mutex[mutex_id] != NULL) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES ||
        g_tm_mutex[mutex_id] == NULL) return TM_ERROR;
    return (xSemaphoreTake(g_tm_mutex[mutex_id], portMAX_DELAY) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES ||
        g_tm_mutex[mutex_id] == NULL) return TM_ERROR;
    return (xSemaphoreGive(g_tm_mutex[mutex_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
    int i;
    uint8_t *base;

    if (pool_id < 0 || pool_id >= TM_FREERTOS_MAX_POOLS) return TM_ERROR;

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
    if (pool_id < 0 || pool_id >= TM_FREERTOS_MAX_POOLS || memory_ptr == NULL) return TM_ERROR;
    block = g_tm_pool_free[pool_id];
    if (block == NULL) return TM_ERROR;
    g_tm_pool_free[pool_id] = *((void**)block);
    *memory_ptr = (unsigned char*)block;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_FREERTOS_MAX_POOLS || memory_ptr == NULL) return TM_ERROR;
    *((void**)memory_ptr) = g_tm_pool_free[pool_id];
    g_tm_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

void tm_cause_interrupt(void)
{
    NVIC_STIR = SW_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

// Strong override of the weak alias from startup_stm32h753.S.
void TIM7_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

// FreeRTOS static allocation hooks.
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
