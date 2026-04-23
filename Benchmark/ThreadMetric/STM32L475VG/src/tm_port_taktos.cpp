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
// Targets: bare-metal Cortex-M4F / STM32L4 family
//   - NUCLEO-L432KC : STM32L432KC, 80 MHz PLL, 48 KB SRAM1, 256 KB flash
//   - B-L475E-IOT01A: STM32L475VG, 80 MHz PLL, 96 KB SRAM1, 1 MB flash
//
// Console output: board-specific. tm_console_*.cpp defines tm_hw_console_init
// and tm_putchar as strong symbols overriding the weak stubs in this file.
//
// Tick : 1000 Hz (1 ms).
//
// Behavior notes:
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1 (highest) -> TaktOS 31
//       TM 31 (lowest) -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a two-
//     phase materialization model during tm_initialize().
//   - tm_cause_interrupt() pends TIM7_IRQn (IRQ 55). TIM7 is never clocked
//     or configured here; only its NVIC pending bit is touched. A weak alias
//     wires TIM7_IRQHandler to tm_irq_vector_handler().
//
// RAM is plentiful on both boards; no slot-remapping gymnastics like the
// F0308 port needs. TM_TAKTOS_MAX_THREADS is fixed at 15 (matches nRF52832).
// -----------------------------------------------------------------------------

#ifndef TM_TAKTOS_MAX_THREADS
#  define TM_TAKTOS_MAX_THREADS    15
#endif
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_MUTEXES      1
#define TM_TAKTOS_MAX_POOLS        1       /* TM5 optional - harmless if unused */

#ifndef TM_TAKTOS_STACK_BYTES
#  define TM_TAKTOS_STACK_BYTES    1024u
#endif
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_CORE_CLOCK_HZ    80000000u    /* HSI16 -> PLL -> 80 MHz */

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

#define TM_BLOCK_SIZE              128u
#define TM_POOL_SIZE               2048u
#define TM_BLOCK_COUNT             (TM_POOL_SIZE / TM_BLOCK_SIZE)

// -----------------------------------------------------------------------------
// Software-interrupt wiring (STM32L4)
//
// TIM7 is a basic 16-bit timer that is unclocked by default (RCC.APB1ENR1 bit
// for TIM7EN is off at reset). tm_cause_interrupt() pends its NVIC IRQ line
// without ever touching the peripheral, so nothing on the bus moves.
//
// STM32L4 NVIC has 4 implementable priority bits, giving 16 usable levels.
// Byte-access NVIC_IPR is supported on ARMv7-M, but we use word access for
// portability.
// -----------------------------------------------------------------------------
#define TM_L4_SWI_IRQ_N            55u     /* TIM7_IRQn on both L432KC and L475VG */

// Cortex-M4 NVIC + SCB register addresses (ARMv7-M).
#define SCB_ICSR_ADDR              0xE000ED04UL
#define SCB_SHPR3_ADDR             0xE000ED20UL
#define NVIC_ISER_ADDR             0xE000E100UL
#define NVIC_ISPR_ADDR             0xE000E200UL
#define NVIC_IPR_ADDR              0xE000E400UL
#define NVIC_STIR_ADDR             0xE000EF00UL

#define SCB_SHPR3                  (*(volatile uint32_t*)SCB_SHPR3_ADDR)
#define NVIC_ISER                  ((volatile uint32_t*)NVIC_ISER_ADDR)
#define NVIC_ISPR                  ((volatile uint32_t*)NVIC_ISPR_ADDR)
#define NVIC_IPR_WORD              ((volatile uint32_t*)NVIC_IPR_ADDR)
#define NVIC_STIR                  (*(volatile uint32_t*)NVIC_STIR_ADDR)

// -----------------------------------------------------------------------------
// Per-thread descriptor. The thread's stack memory is held inline so all
// port state is in .bss, with no heap use at any point.
// -----------------------------------------------------------------------------
#pragma pack(push,4)
typedef struct TmThreadDesc_s {
    bool            allocated;
    bool            resume_before_start;
    uint8_t         tm_priority;
    void          (*entry)(void);
    bool            materialized;
    TaktOSThread    thread;
    uint8_t         mem[TAKTOS_THREAD_MEM_SIZE(TM_TAKTOS_STACK_BYTES)] __attribute__((aligned(8)));
} TmThreadDesc_t;
#pragma pack(pop)

static TmThreadDesc_t  g_threads[TM_TAKTOS_MAX_THREADS];
static bool            g_kernel_started = false;

static TaktOSQueue     g_queues[TM_TAKTOS_MAX_QUEUES];
static uint8_t         g_queue_storage[TM_TAKTOS_MAX_QUEUES]
                                     [TM_TAKTOS_QUEUE_DEPTH * TM_TAKTOS_QUEUE_MSG_SIZE]
                                     __attribute__((aligned(4)));
static bool            g_queue_created[TM_TAKTOS_MAX_QUEUES];

static TaktOSSem       g_semaphores[TM_TAKTOS_MAX_SEMAPHORES];
static bool            g_semaphore_created[TM_TAKTOS_MAX_SEMAPHORES];

static TaktOSMutex     g_mutexes[TM_TAKTOS_MAX_MUTEXES];
static bool            g_mutex_created[TM_TAKTOS_MAX_MUTEXES];

static uint8_t         g_pool_area[TM_TAKTOS_MAX_POOLS][TM_POOL_SIZE]
                                     __attribute__((aligned(sizeof(void*))));
static void           *g_pool_free[TM_TAKTOS_MAX_POOLS];
static bool            g_pool_created[TM_TAKTOS_MAX_POOLS];

// Weak no-op hooks - a test file can override these.
extern "C" void tm_interrupt_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_preemption_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_handler(void) {}
extern "C" void tm_interrupt_preemption_handler(void) {}

// Weak console stubs - the board-specific tm_console_*.cpp overrides these.
extern "C" __attribute__((weak)) void tm_hw_console_init(void) { }
extern "C" __attribute__((weak)) void tm_putchar(int c)        { (void)c; }

// -----------------------------------------------------------------------------
// Priority mapping: TM 1 highest -> TaktOS 31; TM 31 lowest -> TaktOS 1.
// -----------------------------------------------------------------------------
static inline uint8_t tm_to_taktos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (uint8_t)(32 - tm_priority);
}

// -----------------------------------------------------------------------------
// System exception priority configuration.
//
// STM32L4 implements 4 priority bits (top 4 of each 8-bit byte). Valid byte
// values are multiples of 0x10: 0x00 (highest), 0x10, 0x20, ..., 0xF0 (lowest).
//
// PendSV and SysTick -> 0xF0 (lowest). TaktOS context switch must be the
// lowest-priority exception so ISRs preempt it.
// -----------------------------------------------------------------------------
static void tm_set_system_handler_priorities(void)
{
    // SHPR3: [31:24] = SysTick, [23:16] = PendSV
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xF0u << 16);      // PendSV
    v |= (0xF0u << 24);      // SysTick
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    // SW IRQ at 0x80 - higher than PendSV (0xF0, lowest) so the ISR runs
    // promptly; PendSV then tail-chains to do the scheduling work.
    const uint32_t word  = TM_L4_SWI_IRQ_N >> 2;
    const uint32_t shift = (TM_L4_SWI_IRQ_N & 3u) * 8u;
    uint32_t v = NVIC_IPR_WORD[word];
    v &= ~(0xFFu << shift);
    v |=  (0x80u << shift);
    NVIC_IPR_WORD[word] = v;

    NVIC_ISER[TM_L4_SWI_IRQ_N >> 5] = (1u << (TM_L4_SWI_IRQ_N & 31u));
}

// -----------------------------------------------------------------------------
// Thread materialization
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Thread-Metric API implementation
// -----------------------------------------------------------------------------
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
    for (i = 0; i < TM_TAKTOS_MAX_QUEUES; ++i)     g_queue_created[i]     = false;
    for (i = 0; i < TM_TAKTOS_MAX_SEMAPHORES; ++i) g_semaphore_created[i] = false;
    for (i = 0; i < TM_TAKTOS_MAX_MUTEXES; ++i)    g_mutex_created[i]     = false;
    for (i = 0; i < TM_TAKTOS_MAX_POOLS; ++i) {
        g_pool_created[i] = false;
        g_pool_free[i]    = NULL;
    }

    g_kernel_started = false;

    tm_hw_console_init();
    tm_set_system_handler_priorities();

    if (TaktOSInit(TM_TAKTOS_CORE_CLOCK_HZ, TM_TAKTOS_TICK_HZ,
                   TAKTOS_TICK_CLOCK_PROCESSOR, 0u) != TAKTOS_OK) {
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
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS ||
        !g_threads[thread_id].allocated)
        return TM_ERROR;
    if (!g_kernel_started) {
        g_threads[thread_id].resume_before_start = true;
        return TM_SUCCESS;
    }
    return (g_threads[thread_id].thread.Resume() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS ||
        !g_threads[thread_id].allocated)
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

extern "C" void tm_thread_sleep_ticks(int ticks)
{
    uint32_t t = (uint32_t)(ticks <= 0 ? 1 : ticks);
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), t);
}

// ----- Queue ----------------------------------------------------------------
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
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES ||
        !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;
    return (g_queues[queue_id].Send(message_ptr, false) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES ||
        !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;
    return (g_queues[queue_id].Receive(message_ptr, false, TAKTOS_NO_WAIT) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

// ----- Semaphore ------------------------------------------------------------
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
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES ||
        !g_semaphore_created[semaphore_id])
        return TM_ERROR;
    return (g_semaphores[semaphore_id].Take(false, TAKTOS_NO_WAIT) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES ||
        !g_semaphore_created[semaphore_id])
        return TM_ERROR;
    return (g_semaphores[semaphore_id].Give(false) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

// ----- Mutex ----------------------------------------------------------------
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
    return (g_mutexes[mutex_id].Lock(true, TAKTOS_WAIT_FOREVER) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES || !g_mutex_created[mutex_id])
        return TM_ERROR;
    return (g_mutexes[mutex_id].Unlock() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

// ----- Memory pool (simple free-list; optional - TM5 is not in the official TaktOS set) ---
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
    if (pool_id < 0 || pool_id >= TM_TAKTOS_MAX_POOLS ||
        !g_pool_created[pool_id] || memory_ptr == NULL)
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
    if (pool_id < 0 || pool_id >= TM_TAKTOS_MAX_POOLS ||
        !g_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;
    *((void**)memory_ptr) = g_pool_free[pool_id];
    g_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

// ----- Software interrupt ---------------------------------------------------
extern "C" void tm_cause_interrupt(void)
{
    // ARMv7-M: STIR is the fastest way to pend an IRQ. Falls back to
    // ISPR-write if STIR is inaccessible from user mode (it isn't here -
    // tasks run privileged in this port), but STIR is preferred.
    NVIC_STIR = TM_L4_SWI_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// Interrupt handler: TIM7_IRQHandler borrowed for tm_cause_interrupt().
// The startup assembly weak-aliases TIM7_IRQHandler to Default_Handler;
// this strong symbol (via __attribute__((alias))) overrides that.
// -----------------------------------------------------------------------------
extern "C" void tm_irq_vector_handler(void);
extern "C" void tm_irq_vector_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

extern "C" void TIM7_IRQHandler(void) __attribute__((weak, alias("tm_irq_vector_handler")));
