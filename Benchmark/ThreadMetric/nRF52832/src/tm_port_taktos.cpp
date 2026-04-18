#include <stdint.h>
#include <stddef.h>

#include "tm_api.h"

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"


// -----------------------------------------------------------------------------
// TaktOS porting layer for the official Thread-Metric test suite.
//
// Target: bare-metal Cortex-M33 / nRF54L15 application core
// Output: UART via UARTE30 EasyDMA (P0.00 by default on the nRF54L15 DK VCOM)
// Tick  : 100 Hz (10 ms), matching Thread-Metric requirements
//
// Important behavior notes:
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1 (highest)  -> TaktOS 31
//       TM 31 (lowest)  -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a two-phase
//     materialization model during tm_initialize().
//   - Queue and memory-pool semantics match the official harness expectations.
//   - tm_cause_interrupt() uses SWI00 (IRQ 28) via STIR.
// -----------------------------------------------------------------------------

#define TM_TAKTOS_MAX_THREADS      15
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_MUTEXES      1
#define TM_TAKTOS_MAX_POOLS        1

#define TM_TAKTOS_STACK_BYTES      1024u
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_TICK_CLOCK_HZ    64000000

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

#define TM_BLOCK_SIZE              128u
#define TM_POOL_SIZE               2048u
#define TM_BLOCK_COUNT             (TM_POOL_SIZE / TM_BLOCK_SIZE)


// nRF52 UARTE0 (adjust pin for your board if needed)
#define UARTE0_BASE                0x40002000u
#define UARTE0_STARTTX             (*(volatile uint32_t*)(UARTE0_BASE + 0x008u))
#define UARTE0_STOPTX              (*(volatile uint32_t*)(UARTE0_BASE + 0x00Cu))
#define UARTE0_ENDTX               (*(volatile uint32_t*)(UARTE0_BASE + 0x120u))
#define UARTE0_TXD_PTR             (*(volatile uint32_t*)(UARTE0_BASE + 0x544u))
#define UARTE0_TXD_MAXCNT          (*(volatile uint32_t*)(UARTE0_BASE + 0x548u))
#define UARTE0_ENABLE              (*(volatile uint32_t*)(UARTE0_BASE + 0x500u))
#define UARTE0_PSEL_TXD            (*(volatile uint32_t*)(UARTE0_BASE + 0x50Cu))
#define UARTE0_BAUDRATE            (*(volatile uint32_t*)(UARTE0_BASE + 0x524u))
#define UARTE0_CONFIG              (*(volatile uint32_t*)(UARTE0_BASE + 0x56Cu))
#define NRF52_GPIO_BASE            0x50000000u
#define GPIO_DIRSET                (*(volatile uint32_t*)(NRF52_GPIO_BASE + 0x518u))
#define GPIO_OUTSET                (*(volatile uint32_t*)(NRF52_GPIO_BASE + 0x508u))
#define UART_TX_PIN                7u
#define UART_BAUDRATE_115200       0x01D7E000u

// NVIC / system handler priority registers
#define SCB_SHPR3                  (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER0                 (*(volatile uint32_t*)0xE000E100u)
#define NVIC_IPR5                  (*(volatile uint32_t*)0xE000E414u)  // IRQ20..23
#define NVIC_STIR                  (*(volatile uint32_t*)0xE000EF00u)
#define SW_IRQ_N                   21u

#pragma pack(push,4)
typedef struct TmThreadDesc_s {
    bool            allocated;
    bool            resume_before_start;
    uint8_t         tm_priority;
    void          (*entry)(void);
    bool            materialized;
    TaktOSThread    thread;
    uint8_t         mem[TAKTOS_THREAD_STACK_SIZE(TM_TAKTOS_STACK_BYTES)] __attribute__((aligned(4)));
} TmThreadDesc_t;
#pragma pack(pop)

static TmThreadDesc_t  g_threads[TM_TAKTOS_MAX_THREADS];
static bool          g_kernel_started = false;

static TaktOSQueue g_queues[TM_TAKTOS_MAX_QUEUES];
static uint8_t       g_queue_storage[TM_TAKTOS_MAX_QUEUES]
                                   [TM_TAKTOS_QUEUE_DEPTH * TM_TAKTOS_QUEUE_MSG_SIZE]
                                   __attribute__((aligned(4)));
static bool          g_queue_created[TM_TAKTOS_MAX_QUEUES];

static TaktOSSem     g_semaphores[TM_TAKTOS_MAX_SEMAPHORES];
static bool          g_semaphore_created[TM_TAKTOS_MAX_SEMAPHORES];
static TaktOSMutex   g_mutexes[TM_TAKTOS_MAX_MUTEXES];
static bool          g_mutex_created[TM_TAKTOS_MAX_MUTEXES];

static uint8_t       g_pool_area[TM_TAKTOS_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static void         *g_pool_free[TM_TAKTOS_MAX_POOLS];
static bool          g_pool_created[TM_TAKTOS_MAX_POOLS];

static char          g_uart_buf[128];

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

static void tm_uart_send(const char *buf, uint32_t len)
{
    while (len > 0u) {
        uint32_t chunk = (len > sizeof(g_uart_buf)) ? (uint32_t)sizeof(g_uart_buf) : len;
        for (uint32_t i = 0; i < chunk; ++i) {
            g_uart_buf[i] = buf[i];
        }
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

extern "C" void tm_hw_console_init(void)
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
    // PendSV = SHPR3[23:16], SysTick = SHPR3[31:24]
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xFFu << 16);
    v |= (0xFFu << 24);
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    // IRQ21 above PendSV/SysTick so the ISR runs first, then PendSV tail-chains.
    NVIC_IPR5 &= ~(0xFFu << 8);
    NVIC_IPR5 |=  (0xC0u << 8);
    NVIC_ISER0 = (1u << SW_IRQ_N);
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
	TmThreadDesc_t *t;

    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS)
        return TM_ERROR;

    t = &g_threads[thread_id];
    if (!t->allocated)
        return TM_ERROR;

    if (t->materialized)
        return TM_SUCCESS;

    if (t->thread.Create(t->mem,
                         (uint32_t)sizeof(t->mem),
                         tm_thread_trampoline,
                         (void*)(uintptr_t)thread_id,
                         tm_to_taktos_priority(t->tm_priority)) == NULL)
        return TM_ERROR;

    t->materialized = true;
    return TM_SUCCESS;
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
            if (g_threads[i].thread.Suspend() != TAKTOS_OK) {
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
        g_threads[i].materialized = false;
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

    tm_set_system_handler_priorities();
    if (TaktOSInit(TM_TAKTOS_TICK_CLOCK_HZ, TM_TAKTOS_TICK_HZ, TAKTOS_TICK_CLOCK_PROCESSOR, 0u) != TAKTOS_OK) {
        tm_check_fail("FATAL: TaktOSInit failed\n");
    }

    tm_hw_console_init();

    test_initialization_function();

    tm_materialize_all_threads();
    tm_enable_software_interrupt();
    g_kernel_started = true;

    TaktOSStart();
    for (;;) {}
}

extern "C" int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
	TmThreadDesc_t *t;

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
        return (t->thread.Suspend() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
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

    return (g_threads[thread_id].thread.Resume() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;

    if (!g_kernel_started) {
        g_threads[thread_id].resume_before_start = false;
        return TM_SUCCESS;
    }

    return (g_threads[thread_id].thread.Suspend() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
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
void tm_thread_sleep_ticks(int ticks)
{
    uint32_t t = (uint32_t)(ticks <= 0 ? 1 : ticks);
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), t);
}


extern "C" int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES)
        return TM_ERROR;

    if (g_queues[queue_id].Init(g_queue_storage[queue_id],
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

    return (g_queues[queue_id].Send(message_ptr, false) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;

    return (g_queues[queue_id].Receive(message_ptr, false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES)
        return TM_ERROR;

    if (g_semaphores[semaphore_id].Init(1u, 1u) != TAKTOS_OK)
        return TM_ERROR;

    g_semaphore_created[semaphore_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id])
        return TM_ERROR;

    return (g_semaphores[semaphore_id].Take(false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id])
        return TM_ERROR;

    return (g_semaphores[semaphore_id].Give(false) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES)
        return TM_ERROR;

    if (g_mutexes[mutex_id].Init() != TAKTOS_OK)
        return TM_ERROR;

    g_mutex_created[mutex_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES || !g_mutex_created[mutex_id])
        return TM_ERROR;

    return (g_mutexes[mutex_id].Lock(true, TAKTOS_WAIT_FOREVER) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES || !g_mutex_created[mutex_id])
        return TM_ERROR;

    return (g_mutexes[mutex_id].Unlock() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
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

extern "C" void SWI1_EGU1_IRQHandler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}
