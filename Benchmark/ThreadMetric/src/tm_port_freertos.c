/**-------------------------------------------------------------------------
 * @file    tm_port_freertos.c
 *
 * @brief   Thread-Metric port for FreeRTOS. Shared across every MCU.
 *
 * Per-MCU board.h supplies TM_CORE_CLOCK_HZ, TM_SW_IRQn, and the
 * arch-specific implementations of TmCauseInterrupt / TmSetKernelPriorities
 * / TmEnableSoftwareInterrupt.
 *
 * Shape: direct forward, no defensive checks  matches Microsoft's Zephyr
 * reference port byte-for-byte in shape, mirroring tm_port_taktos and
 * tm_port_threadx for apples-to-apples comparison.
 *
 * No architecture-specific code in this file  builds for ARM Cortex-M
 * and RISC-V RV32 alike (FreeRTOS port assembly comes from upstream
 * portable/GCC/<ARCH>/port.c via Eclipse project configuration).
 * -------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tm_api.h"
#include "tm_port_common.h"
#include "board.h"           /* TM_CORE_CLOCK_HZ, TM_SW_IRQn, board helpers */

/* Stack identical across all ports: TM_PORT_STACK_BYTES (1024 bytes / 256 words). */
#define TM_FREERTOS_STACK_WORDS   (TM_PORT_STACK_BYTES / sizeof(StackType_t))

#define TM_QUEUE_MSG_BYTES        TM_PORT_QUEUE_MSG_BYTES

/* ----- Storage --------------------------------------------------------- */
static TaskHandle_t      g_tm_thread[TM_PORT_MAX_THREADS];
static StaticTask_t      g_tm_tcb   [TM_PORT_MAX_THREADS];
static StackType_t       g_tm_stack [TM_PORT_MAX_THREADS][TM_FREERTOS_STACK_WORDS]
                                    __attribute__((aligned(8)));
static void            (*g_tm_entry [TM_PORT_MAX_THREADS])(void);

static QueueHandle_t     g_tm_queue        [TM_PORT_MAX_QUEUES];
static StaticQueue_t     g_tm_queue_struct [TM_PORT_MAX_QUEUES];
static uint8_t           g_tm_queue_storage[TM_PORT_MAX_QUEUES]
                                           [TM_PORT_QUEUE_DEPTH * TM_QUEUE_MSG_BYTES]
                                           __attribute__((aligned(4)));

static SemaphoreHandle_t g_tm_sem        [TM_PORT_MAX_SEMAPHORES];
static StaticSemaphore_t g_tm_sem_struct [TM_PORT_MAX_SEMAPHORES];
static SemaphoreHandle_t g_tm_mutex      [TM_PORT_MAX_MUTEXES];
static StaticSemaphore_t g_tm_mutex_struct[TM_PORT_MAX_MUTEXES];

static uint8_t           g_tm_pool_area[TM_PORT_MAX_POOLS][TM_PORT_POOL_BYTES]
                                       __attribute__((aligned(sizeof(void*))));
static void             *g_tm_pool_free[TM_PORT_MAX_POOLS];

/* Pre-scheduler resume tracking. The Thread-Metric harness creates threads
 * and calls tm_thread_resume()/tm_thread_suspend() BEFORE the scheduler is
 * running. Calling vTaskResume() before vTaskStartScheduler() is undefined
 * behaviour: it pends a context switch through paths that assume task stacks
 * are already set up, which they are not yet.
 *
 * Solution: defer resume/suspend calls made before the scheduler has started.
 * Track which threads were marked resumed during init, then apply the
 * matching suspends to the rest just before vTaskStartScheduler. This keeps
 * tm_port_freertos.c portable across every MCU  no per-arch code. */
static bool              g_tm_resumed[TM_PORT_MAX_THREADS];
static bool              g_scheduler_started = false;

/* ----- ISR weak hooks (overridden by InterruptProcessing tests) -------- */
void tm_interrupt_handler(void)            __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void)            {}
void tm_interrupt_preemption_handler(void) {}

/* ----- Helpers --------------------------------------------------------- */
static void tm_task_trampoline(void *param)
{
    int thread_id = (int)(uintptr_t)param;
    g_tm_entry[thread_id]();
    /* Thread-Metric workers loop forever  fall back to suspend-self in the
     * unreachable case rather than vTaskDelete (lets the build skip
     * INCLUDE_vTaskDelete=1, which pulls in the deletion machinery). */
    vTaskSuspend(NULL);
    for (;;) {}
}

static UBaseType_t tm_map_priority(int tm_priority)
{
    /* Match the official FreeRTOS port mapping. */
    return (UBaseType_t)((configMAX_PRIORITIES - 1) - tm_priority);
}

/* ----- Initialization -------------------------------------------------- */
void tm_initialize(void (*test_initialization_function)(void))
{
    /* Reset deferred-resume tracking before test_initialization_function
     * creates threads and calls resume/suspend on them. */
    for (int i = 0; i < TM_PORT_MAX_THREADS; ++i) {
        g_tm_resumed[i] = false;
        g_tm_thread[i] = NULL;
    }
    g_scheduler_started = false;

    TmSetKernelPriorities();
    TmEnableSoftwareInterrupt();

    test_initialization_function();

    /* Apply the deferred suspends: any created thread NOT marked resumed
     * during init must be suspended before the scheduler runs. Threads ARE
     * created in the ready state by xTaskCreateStatic; we suspend the ones
     * that were never resumed. Calling vTaskSuspend pre-scheduler is safe
     * (FreeRTOS just updates the list  no context switch). */
    for (int i = 0; i < TM_PORT_MAX_THREADS; ++i) {
        if (g_tm_thread[i] != NULL && !g_tm_resumed[i]) {
            vTaskSuspend(g_tm_thread[i]);
        }
    }

    g_scheduler_started = true;
    vTaskStartScheduler();
    tm_check_fail("FATAL: scheduler failed to start\n");
}

/* ----- Thread ---------------------------------------------------------- */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    g_tm_entry[thread_id] = entry_function;

    g_tm_thread[thread_id] = xTaskCreateStatic(tm_task_trampoline,
                                               "TM",
                                               TM_FREERTOS_STACK_WORDS,
                                               (void*)(uintptr_t)thread_id,
                                               tm_map_priority(priority),
                                               g_tm_stack[thread_id],
                                               &g_tm_tcb[thread_id]);
    /* Created threads are runnable by default. The deferred-resume logic
     * in tm_initialize() handles pre-scheduler suspends. Post-scheduler
     * tm_thread_create (rare) would need a vTaskSuspend here, but the TM
     * harness creates all threads during init only. */
    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (!g_scheduler_started) {
        /* Defer: just mark this thread as wanting-to-run. tm_initialize
         * applies the deferred state right before vTaskStartScheduler. */
        g_tm_resumed[thread_id] = true;
        return TM_SUCCESS;
    }
    vTaskResume(g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

int tm_thread_suspend(int thread_id)
{
    if (!g_scheduler_started) {
        /* Defer: clear the resumed flag so tm_initialize will suspend it. */
        g_tm_resumed[thread_id] = false;
        return TM_SUCCESS;
    }
    vTaskSuspend(g_tm_thread[thread_id]);
    return TM_SUCCESS;
}

void tm_thread_relinquish(void) { taskYIELD(); }

void tm_thread_sleep(int seconds)
{
    vTaskDelay(pdMS_TO_TICKS((uint32_t)seconds * 1000u));
}

void tm_thread_sleep_ticks(int ticks)
{
    vTaskDelay((TickType_t)ticks);
}

/* ----- Queue ----------------------------------------------------------- */
int tm_queue_create(int queue_id)
{
    g_tm_queue[queue_id] = xQueueCreateStatic(TM_PORT_QUEUE_DEPTH,
                                              TM_QUEUE_MSG_BYTES,
                                              g_tm_queue_storage[queue_id],
                                              &g_tm_queue_struct[queue_id]);
    return TM_SUCCESS;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    return (xQueueSendToBack(g_tm_queue[queue_id], (const void*)message_ptr, 0) == pdTRUE)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    return (xQueueReceive(g_tm_queue[queue_id], (void*)message_ptr, 0) == pdTRUE)
           ? TM_SUCCESS : TM_ERROR;
}

/* ----- Semaphore ------------------------------------------------------- */
int tm_semaphore_create(int semaphore_id)
{
    g_tm_sem[semaphore_id] = xSemaphoreCreateBinaryStatic(&g_tm_sem_struct[semaphore_id]);
    xSemaphoreGive(g_tm_sem[semaphore_id]);   /* start available */
    return TM_SUCCESS;
}

int tm_semaphore_get(int semaphore_id)
{
    return (xSemaphoreTake(g_tm_sem[semaphore_id], 0) == pdTRUE)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    return (xSemaphoreGive(g_tm_sem[semaphore_id]) == pdTRUE)
           ? TM_SUCCESS : TM_ERROR;
}

/* ----- Mutex ----------------------------------------------------------- */
int tm_mutex_create(int mutex_id)
{
    g_tm_mutex[mutex_id] = xSemaphoreCreateMutexStatic(&g_tm_mutex_struct[mutex_id]);
    return TM_SUCCESS;
}

int tm_mutex_lock(int mutex_id)
{
    return (xSemaphoreTake(g_tm_mutex[mutex_id], portMAX_DELAY) == pdTRUE)
           ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    return (xSemaphoreGive(g_tm_mutex[mutex_id]) == pdTRUE)
           ? TM_SUCCESS : TM_ERROR;
}

/* ----- Memory pool (hand-rolled linked list, identical across ports) --- */
int tm_memory_pool_create(int pool_id)
{
    uint8_t *base = g_tm_pool_area[pool_id];
    g_tm_pool_free[pool_id] = base;
    for (uint32_t i = 0; i < TM_PORT_BLOCK_COUNT - 1u; ++i)
        *(void**)(base + i * TM_PORT_BLOCK_BYTES) = (void*)(base + (i + 1u) * TM_PORT_BLOCK_BYTES);
    *(void**)(base + (TM_PORT_BLOCK_COUNT - 1u) * TM_PORT_BLOCK_BYTES) = NULL;
    return TM_SUCCESS;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    void *b = g_tm_pool_free[pool_id];
    if (!b) return TM_ERROR;
    g_tm_pool_free[pool_id] = *(void**)b;
    *memory_ptr = (unsigned char*)b;
    return TM_SUCCESS;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    *(void**)memory_ptr = g_tm_pool_free[pool_id];
    g_tm_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

/* ----- TM5 software interrupt (arch-encapsulated by board.h) ----------- */
void tm_cause_interrupt(void)
{
    TmCauseInterrupt();
}

/* ----- FreeRTOS static-allocation hooks --------------------------------
 * vApplicationGetIdleTaskMemory: provided as WEAK so this works across
 * every FreeRTOS minor version:
 *   - V10.x: function is required, our weak version is the only definition.
 *   - V11.0+ with configKERNEL_PROVIDED_STATIC_MEMORY=1: kernel provides a
 *     strong default that overrides our weak one, no link conflict.
 *   - V11.0+ with configKERNEL_PROVIDED_STATIC_MEMORY=0: kernel does NOT
 *     provide one, our weak version is used.
 * Without 'weak' we'd get either "undefined reference" (V10) or "multiple
 * definition" (V11.0+ with the kernel default), depending on the kernel
 * revision  and the fix differs per version. Weak avoids that whole mess.
 */
__attribute__((weak))
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
