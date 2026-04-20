#include <stdint.h>
#include <stddef.h>

extern "C" {
#include "tm_api.h"
#include "tm_target_esp32c.h"
}

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSQueue.h"

// -----------------------------------------------------------------------------
// TaktOS porting layer for the official Thread-Metric test suite.
//
// Target: ESP32-C series using the in-progress TaktOS port
// Notes :
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1 (highest)  -> TaktOS 31
//       TM 31 (lowest)  -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a two-phase
//     materialization model during tm_initialize().
//   - Queue and memory-pool semantics match the official harness expectations.
//   - Software interrupt support is isolated behind tm_target_install_soft_irq()
//     and tm_target_raise_soft_irq().
// -----------------------------------------------------------------------------

#define TM_TAKTOS_MAX_THREADS      10
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_POOLS        1

#define TM_TAKTOS_STACK_BYTES      1024u
#define TM_TAKTOS_TICK_HZ          TM_ESP32C_TICK_HZ
#define TM_TAKTOS_CORE_CLOCK_HZ    TM_ESP32C_CORE_CLOCK_HZ

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

#define TM_BLOCK_SIZE              128u
#define TM_POOL_SIZE               2048u
#define TM_BLOCK_COUNT             (TM_POOL_SIZE / TM_BLOCK_SIZE)

struct TmThreadDesc {
    bool            allocated;
    bool            resume_before_start;
    uint8_t         tm_priority;
    void          (*entry)(void);
    TaktOSThread_t *handle;
    uint8_t         mem[TAKTOS_THREAD_MEM_SIZE(TM_TAKTOS_STACK_BYTES)] __attribute__((aligned(8)));
};

static TmThreadDesc  g_threads[TM_TAKTOS_MAX_THREADS];
static bool          g_kernel_started = false;
static bool          g_soft_irq_installed = false;

static TaktOSQueue_t g_queues[TM_TAKTOS_MAX_QUEUES];
static uint8_t       g_queue_storage[TM_TAKTOS_MAX_QUEUES]
                                   [TM_TAKTOS_QUEUE_DEPTH * TM_TAKTOS_QUEUE_MSG_SIZE]
                                   __attribute__((aligned(8)));
static bool          g_queue_created[TM_TAKTOS_MAX_QUEUES];

static TaktOSSem_t   g_semaphores[TM_TAKTOS_MAX_SEMAPHORES];
static bool          g_semaphore_created[TM_TAKTOS_MAX_SEMAPHORES];

static uint8_t       g_pool_area[TM_TAKTOS_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static void         *g_pool_free[TM_TAKTOS_MAX_POOLS];
static bool          g_pool_created[TM_TAKTOS_MAX_POOLS];

#define TM_UART_BUF_SIZE 512
static char          g_uart_tx_buf[TM_UART_BUF_SIZE];
static uint32_t      g_uart_tx_idx = 0;

extern "C" void tm_interrupt_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_preemption_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_handler(void) {}
extern "C" void tm_interrupt_preemption_handler(void) {}

static inline uint8_t tm_to_taktos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (uint8_t)(32 - tm_priority);
}

static void tm_uart_flush(void)
{
    if (g_uart_tx_idx > 0u) {
        tm_target_console_write(g_uart_tx_buf, g_uart_tx_idx);
        g_uart_tx_idx = 0u;
    }
}

extern "C" void tm_hw_console_init(void)
{
    tm_target_console_init();
}

static void tm_soft_irq_dispatch(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

static void tm_enable_software_interrupt(void)
{
    g_soft_irq_installed = (tm_target_install_soft_irq(tm_soft_irq_dispatch) != 0);
}

static void tm_thread_trampoline(void *arg)
{
    int id = (int)(uintptr_t)arg;
    if (id >= 0 && id < TM_TAKTOS_MAX_THREADS && g_threads[id].entry) {
        g_threads[id].entry();
    }
    for (;;) {}
}

static int tm_materialize_thread(int thread_id)
{
    TmThreadDesc *t;

    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS)
        return TM_ERROR;

    t = &g_threads[thread_id];
    if (!t->allocated)
        return TM_ERROR;

    if (t->handle != NULL)
        return TM_SUCCESS;

    t->handle = TaktOSThreadCreate(t->mem,
                                   (uint32_t)sizeof(t->mem),
                                   tm_thread_trampoline,
                                   (void*)(uintptr_t)thread_id,
                                   tm_to_taktos_priority(t->tm_priority));
    return (t->handle != NULL) ? TM_SUCCESS : TM_ERROR;
}

static void tm_materialize_all_threads(void)
{
    int i;

    for (i = 0; i < TM_TAKTOS_MAX_THREADS; ++i) {
        if (g_threads[i].allocated && tm_materialize_thread(i) != TM_SUCCESS) {
            tm_check_fail("FATAL: TaktOS thread creation failed\n");
        }
    }
    for (i = 0; i < TM_TAKTOS_MAX_THREADS; ++i) {
        if (g_threads[i].allocated && !g_threads[i].resume_before_start) {
            if (TaktOSThreadSuspend(g_threads[i].handle) != TAKTOS_OK) {
                tm_check_fail("FATAL: initial thread suspend failed\n");
            }
        }
    }
}

extern "C" void tm_initialize(void (*test_initialization_function)(void))
{
    int i;

    for (i = 0; i < TM_TAKTOS_MAX_THREADS; ++i) {
        g_threads[i].allocated = false;
        g_threads[i].resume_before_start = false;
        g_threads[i].tm_priority = 31u;
        g_threads[i].entry = NULL;
        g_threads[i].handle = NULL;
    }
    for (i = 0; i < TM_TAKTOS_MAX_QUEUES; ++i) {
        g_queue_created[i] = false;
    }
    for (i = 0; i < TM_TAKTOS_MAX_SEMAPHORES; ++i) {
        g_semaphore_created[i] = false;
    }
    for (i = 0; i < TM_TAKTOS_MAX_POOLS; ++i) {
        g_pool_created[i] = false;
        g_pool_free[i] = NULL;
    }

    g_kernel_started = false;
    g_soft_irq_installed = false;

    if (TaktOSInit(TM_TAKTOS_CORE_CLOCK_HZ, TM_TAKTOS_TICK_HZ, TAKTOS_TICK_CLOCK_PROCESSOR, 0u) != TAKTOS_OK) {
        tm_check_fail("FATAL: TaktOSInit failed\n");
    }

    test_initialization_function();

    tm_materialize_all_threads();
    tm_enable_software_interrupt();
    g_kernel_started = true;

    TaktOSStart();
    for (;;) {}
}

extern "C" int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    TmThreadDesc *t;

    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS ||
        priority < 1 || priority > 31 || entry_function == NULL)
        return TM_ERROR;

    t = &g_threads[thread_id];
    t->allocated = true;
    t->resume_before_start = false;
    t->tm_priority = (uint8_t)priority;
    t->entry = entry_function;

    if (g_kernel_started) {
        if (tm_materialize_thread(thread_id) != TM_SUCCESS)
            return TM_ERROR;
        return (TaktOSThreadSuspend(t->handle) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
    }

    return TM_SUCCESS;
}

extern "C" int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;

    if (!g_kernel_started) {
        g_threads[thread_id].resume_before_start = true;
        return TM_SUCCESS;
    }

    return (TaktOSThreadResume(g_threads[thread_id].handle) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;

    if (!g_kernel_started) {
        g_threads[thread_id].resume_before_start = false;
        return TM_SUCCESS;
    }

    return (TaktOSThreadSuspend(g_threads[thread_id].handle) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" void tm_thread_relinquish(void)
{
    TaktOSThreadYield();
}

extern "C" void tm_thread_sleep(int seconds)
{
    uint32_t ticks = (seconds <= 0) ? 0u : (uint32_t)seconds * TM_TAKTOS_TICK_HZ;
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), ticks);
}

extern "C" int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES)
        return TM_ERROR;

    if (TaktOSQueueInit(&g_queues[queue_id],
                        g_queue_storage[queue_id],
                        TM_TAKTOS_QUEUE_MSG_SIZE,
                        TM_TAKTOS_QUEUE_DEPTH) != TAKTOS_OK) {
        return TM_ERROR;
    }

    g_queue_created[queue_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;

    return (TaktOSQueueSend(&g_queues[queue_id], message_ptr, false, TAKTOS_WAIT_FOREVER) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;

    return (TaktOSQueueReceive(&g_queues[queue_id], message_ptr, false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES)
        return TM_ERROR;

    if (TaktOSSemInit(&g_semaphores[semaphore_id], 1u, 1u) != TAKTOS_OK)
        return TM_ERROR;

    g_semaphore_created[semaphore_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id])
        return TM_ERROR;

    return (TaktOSSemTake(&g_semaphores[semaphore_id], false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id])
        return TM_ERROR;

    return (TaktOSSemGive(&g_semaphores[semaphore_id], false) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_memory_pool_create(int pool_id)
{
    int i;
    uint8_t *base;

    if (pool_id < 0 || pool_id >= TM_TAKTOS_MAX_POOLS)
        return TM_ERROR;

    base = g_pool_area[pool_id];
    g_pool_free[pool_id] = (void*)base;

    for (i = 0; i < (int)TM_BLOCK_COUNT - 1; ++i) {
        void **block = (void**)(base + (uint32_t)i * TM_BLOCK_SIZE);
        *block = (void*)(base + (uint32_t)(i + 1) * TM_BLOCK_SIZE);
    }
    {
        void **last = (void**)(base + (uint32_t)(TM_BLOCK_COUNT - 1) * TM_BLOCK_SIZE);
        *last = NULL;
    }

    g_pool_created[pool_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    void *block;

    if (pool_id < 0 || pool_id >= TM_TAKTOS_MAX_POOLS || !g_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;

    block = g_pool_free[pool_id];
    if (block == NULL)
        return TM_ERROR;

    g_pool_free[pool_id] = *((void**)block);
    *memory_ptr = (unsigned char*)block;
    return TM_SUCCESS;
}

extern "C" int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_TAKTOS_MAX_POOLS || !g_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;

    *((void**)memory_ptr) = g_pool_free[pool_id];
    g_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

extern "C" void tm_cause_interrupt(void)
{
    if (!g_soft_irq_installed) {
        tm_check_fail("FATAL: software interrupt path not installed");
    }
    tm_target_raise_soft_irq();
}

extern "C" void tm_putchar(int c)
{
    static char last_c = 0;

    if (g_uart_tx_idx < TM_UART_BUF_SIZE) {
        g_uart_tx_buf[g_uart_tx_idx++] = (char)c;
    }

    if ((c == '\n' && last_c == '\n') || (g_uart_tx_idx >= TM_UART_BUF_SIZE)) {
        tm_uart_flush();
    }

    last_c = (char)c;
}
