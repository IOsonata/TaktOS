#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "tm_api.h"

// -----------------------------------------------------------------------------
// FreeRTOS bare-metal STM32L432KC port for the official Thread-Metric harness.
//
// Target: NUCLEO-L432KC, STM32L432KCU6, Cortex-M4F @ 80 MHz.
// UART  : USART2 on PA2 (AF7), 115200 8N1, ST-LINK/V2-1 Virtual COM Port.
// Tick  : 1000 Hz.
// SW IRQ: TIM7_IRQn (IRQ 55) - pended only; TIM7 is never clocked.
//
// The startup_stm32l432kc.S, system_stm32l432kc.c, gcc_stm32l432kc.ld, and
// tm_console_stm32l432kc.cpp files are shared with the TaktOS and ThreadX
// builds. FreeRTOSConfig.h maps PendSV_Handler, SVC_Handler, SysTick_Handler
// back to their FreeRTOS implementations (xPortPendSVHandler etc.), so the
// startup's weak aliases resolve to the FreeRTOS port at link time.
// -----------------------------------------------------------------------------

#define TM_FREERTOS_MAX_THREADS      15
#define TM_FREERTOS_MAX_QUEUES       1
#define TM_FREERTOS_MAX_SEMAPHORES   1
#define TM_FREERTOS_MAX_MUTEXES      1
#define TM_FREERTOS_MAX_POOLS        1

/* Stack sizing for STM32L432KC (48 KB SRAM1).
 * 15 threads x 256 words x 4 bytes = 15 KB total stack area.
 * Matches TaktOS port (1024-byte stacks). TM tasks are tiny - a counter
 * increment and an API call - so 1 KB per stack is plenty.             */
#define TM_FREERTOS_STACK_WORDS      256u
#define TM_FREERTOS_QUEUE_DEPTH      10u
#define TM_FREERTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

#define TM_BLOCK_SIZE                128u
#define TM_POOL_SIZE                 2048u
#define TM_BLOCK_COUNT               (TM_POOL_SIZE / TM_BLOCK_SIZE)

// -----------------------------------------------------------------------------
// NVIC + SCB register addresses (ARMv7-M).
// STM32L4 has 4 implementable priority bits (top 4 of each 8-bit byte).
// -----------------------------------------------------------------------------
#define SCB_SHPR3                    (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER1                   (*(volatile uint32_t*)0xE000E104u)  // IRQ32..63
#define NVIC_IPR13                   (*(volatile uint32_t*)0xE000E434u)  // IRQ52..55
#define NVIC_STIR                    (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                     55u     // TIM7_IRQn on STM32L432xx
#define TM_SOFTIRQ_LIBRARY_PRIORITY  8u      // > kernel IRQ prio so ISR runs first

// -----------------------------------------------------------------------------
// Per-task storage. Static allocation throughout (configSUPPORT_STATIC_ALLOCATION=1).
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

// Weak no-op hooks - a test file can override to run work from the ISR.
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
    // Invert: TM 1 = highest -> FreeRTOS (MAX-1); TM 31 = lowest -> FreeRTOS 0.
    return (UBaseType_t)((configMAX_PRIORITIES - 1) - tm_priority);
}

static void tm_set_kernel_priorities(void)
{
    // PendSV and SysTick at configKERNEL_INTERRUPT_PRIORITY (lowest).
    // SHPR3: [23:16] = PendSV, [31:24] = SysTick.
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (uint32_t)(configKERNEL_INTERRUPT_PRIORITY << 16);
    v |= (uint32_t)(configKERNEL_INTERRUPT_PRIORITY << 24);
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    // TIM7_IRQn = 55: NVIC_IPR13 covers IRQ 52..55. Byte 3 (bits [31:24]) = IRQ55.
    // Priority must numerically beat kernel prio so the ISR preempts kernel
    // critical sections only after critical exits, but runs promptly.
    const uint32_t prio_field = (uint32_t)(TM_SOFTIRQ_LIBRARY_PRIORITY
                                          << (8u - configPRIO_BITS));
    NVIC_IPR13 &= ~(0xFFu << 24);
    NVIC_IPR13 |=  (prio_field << 24);

    // Enable in NVIC. IRQ55 is bit (55-32)=23 of ISER1.
    NVIC_ISER1 = (1u << (SW_IRQ_N - 32u));
}

void tm_initialize(void (*test_initialization_function)(void))
{
    int i;

    for (i = 0; i < TM_FREERTOS_MAX_THREADS; ++i) {
        g_tm_thread[i] = NULL;
        g_tm_entry[i] = NULL;
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
    if (h == NULL) {
        return TM_ERROR;
    }

    g_tm_thread[thread_id] = h;
    vTaskSuspend(h);   // create suspended, matching official semantics
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS ||
        g_tm_thread[thread_id] == NULL) {
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
    if (thread_id < 0 || thread_id >= TM_FREERTOS_MAX_THREADS ||
        g_tm_thread[thread_id] == NULL) {
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

// ----- Queue ----------------------------------------------------------------
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

// ----- Semaphore ------------------------------------------------------------
int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_FREERTOS_MAX_SEMAPHORES) return TM_ERROR;

    g_tm_sem[semaphore_id] = xSemaphoreCreateBinaryStatic(&g_tm_sem_struct[semaphore_id]);
    if (g_tm_sem[semaphore_id] == NULL) return TM_ERROR;

    xSemaphoreGive(g_tm_sem[semaphore_id]);  // start available
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
        if (xSemaphoreGiveFromISR(g_tm_sem[semaphore_id], &yield) != pdTRUE) {
            return TM_ERROR;
        }
        portYIELD_FROM_ISR(yield);
        return TM_SUCCESS;
    }

    return (xSemaphoreGive(g_tm_sem[semaphore_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

// ----- Mutex ----------------------------------------------------------------
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
    return (xSemaphoreTake(g_tm_mutex[mutex_id], portMAX_DELAY) == pdTRUE)
                ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_MUTEXES ||
        g_tm_mutex[mutex_id] == NULL) return TM_ERROR;
    return (xSemaphoreGive(g_tm_mutex[mutex_id]) == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

// ----- Memory pool (simple free-list) ---------------------------------------
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

// -----------------------------------------------------------------------------
// TIM7_IRQHandler - borrowed for tm_cause_interrupt(). Declared as a strong
// symbol here to override the weak alias from startup_stm32l432kc.S.
// TIM7 is never clocked by this port - only its NVIC pending bit is touched.
// -----------------------------------------------------------------------------
void TIM7_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

// -----------------------------------------------------------------------------
// FreeRTOS static allocation hooks (required by configSUPPORT_STATIC_ALLOCATION).
// -----------------------------------------------------------------------------
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
