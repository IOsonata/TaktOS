#include <stdint.h>
#include <stddef.h>

#include "tm_api.h"

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"

// -----------------------------------------------------------------------------
// TaktOS port for Thread-Metric on NUCLEO-G474RE (STM32G474RE).
//   Cortex-M4 + FPv4-SP-D16, 170 MHz PLL, 512 KB flash, 128 KB RAM.
//   Hard-float build — links libTaktOS_M4.a from ARM/cm4/Eclipse/ReleaseFPU.
// LPUART1 console routed to ST-LINK/V3E VCP (see tm_console_nucleo_g474.cpp).
// SW-IRQ line: TIM7_DAC_IRQn = 55. TIM7 is never clocked; only NVIC pending
// is pulsed via STIR.
// -----------------------------------------------------------------------------

#ifndef TM_TAKTOS_MAX_SLOTS
#  define TM_TAKTOS_MAX_SLOTS      9
#endif
#define TM_TAKTOS_MAX_TM_IDS       32
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_MUTEXES      1

#ifndef TM_TAKTOS_STACK_BYTES
#  define TM_TAKTOS_STACK_BYTES    1024u
#endif
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_CORE_CLOCK_HZ    170000000u

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

// TIM7_DAC_IRQn = 55 on STM32G474. Peripheral never clocked; NVIC pend only.
#define TM_G4_SWI_IRQ_N            55u

#define SCB_SHPR3                  (*(volatile uint32_t*)0xE000ED20UL)
#define NVIC_ISER                  ((volatile uint32_t*)0xE000E100UL)
#define NVIC_IPR_BYTE              ((volatile uint8_t*) 0xE000E400UL)
#define NVIC_STIR                  (*(volatile uint32_t*)0xE000EF00UL)

#pragma pack(push,4)
typedef struct TmThreadDesc_s {
    bool            allocated;
    bool            resume_before_start;
    uint8_t         tm_priority;
    void          (*entry)(void);
    bool            materialized;
    TaktOSThread    thread;
    uint8_t         mem[TAKTOS_THREAD_MEM_SIZE(TM_TAKTOS_STACK_BYTES)] __attribute__((aligned(4)));
} TmThreadDesc_t;
#pragma pack(pop)

static TmThreadDesc_t  g_threads[TM_TAKTOS_MAX_SLOTS];
static int8_t          g_id_to_slot[TM_TAKTOS_MAX_TM_IDS];
static bool            g_kernel_started = false;

static int tm_alloc_slot(int thread_id)
{
    if (g_id_to_slot[thread_id] >= 0)
        return g_id_to_slot[thread_id];
    for (int s = 0; s < TM_TAKTOS_MAX_SLOTS; ++s) {
        if (!g_threads[s].allocated) {
            g_id_to_slot[thread_id] = (int8_t)s;
            return s;
        }
    }
    return -1;
}

static inline int tm_slot_of(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_TM_IDS) return -1;
    return g_id_to_slot[thread_id];
}

static TaktOSQueue     g_queues[TM_TAKTOS_MAX_QUEUES];
static uint8_t         g_queue_storage[TM_TAKTOS_MAX_QUEUES]
                                     [TM_TAKTOS_QUEUE_DEPTH * TM_TAKTOS_QUEUE_MSG_SIZE]
                                     __attribute__((aligned(4)));
static bool            g_queue_created[TM_TAKTOS_MAX_QUEUES];

static TaktOSSem       g_semaphores[TM_TAKTOS_MAX_SEMAPHORES];
static bool            g_semaphore_created[TM_TAKTOS_MAX_SEMAPHORES];

static TaktOSMutex     g_mutexes[TM_TAKTOS_MAX_MUTEXES];
static bool            g_mutex_created[TM_TAKTOS_MAX_MUTEXES];

extern "C" void tm_interrupt_handler(void)             __attribute__((weak));
extern "C" void tm_interrupt_preemption_handler(void)  __attribute__((weak));
extern "C" void tm_interrupt_handler(void)             {}
extern "C" void tm_interrupt_preemption_handler(void)  {}

extern "C" __attribute__((weak)) void tm_hw_console_init(void) { }
extern "C" __attribute__((weak)) void tm_putchar(int c)        { (void)c; }

static inline uint8_t tm_to_taktos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (uint8_t)(32 - tm_priority);
}

static void tm_set_system_handler_priorities(void)
{
    // STM32G4: 4 NVIC priority bits. PendSV/SysTick = 0xF0 (lowest).
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xF0u << 16);
    v |= (0xF0u << 24);
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    NVIC_IPR_BYTE[TM_G4_SWI_IRQ_N] = 0xC0u;
    NVIC_ISER[TM_G4_SWI_IRQ_N >> 5] = (1u << (TM_G4_SWI_IRQ_N & 31u));
}

static void tm_thread_trampoline(void *arg)
{
    int slot = (int)(uintptr_t)arg;
    if (slot >= 0 && slot < TM_TAKTOS_MAX_SLOTS && g_threads[slot].entry) {
        g_threads[slot].entry();
    }
    for (;;) {}
}

static int tm_materialize_slot(int slot)
{
    if (slot < 0 || slot >= TM_TAKTOS_MAX_SLOTS) return TM_ERROR;
    TmThreadDesc_t *t = &g_threads[slot];
    if (!t->allocated) return TM_ERROR;
    if (t->materialized) return TM_SUCCESS;

    // TaktOSThread::Create returns a handle (hTaktOSThread_t).
    // Success = non-null; failure = nullptr.
    if (t->thread.Create(t->mem,
                         (uint32_t)sizeof(t->mem),
                         tm_thread_trampoline,
                         (void*)(uintptr_t)slot,
                         tm_to_taktos_priority(t->tm_priority)) == nullptr) {
        return TM_ERROR;
    }
    t->materialized = true;
    return TM_SUCCESS;
}

static void tm_materialize_all_threads(void)
{
    for (int s = 0; s < TM_TAKTOS_MAX_SLOTS; ++s) {
        if (g_threads[s].allocated && tm_materialize_slot(s) != TM_SUCCESS) {
            tm_check_fail("FATAL: TaktOS thread creation failed\n");
        }
    }
    for (int s = 0; s < TM_TAKTOS_MAX_SLOTS; ++s) {
        if (g_threads[s].allocated && !g_threads[s].resume_before_start) {
            if (g_threads[s].thread.Suspend() != TAKTOS_OK) {
                tm_check_fail("FATAL: initial thread suspend failed\n");
            }
        }
    }
}

extern "C" void tm_initialize(void (*test_initialization_function)(void))
{
    for (int i = 0; i < TM_TAKTOS_MAX_SLOTS; ++i) {
        g_threads[i].allocated = false;
        g_threads[i].resume_before_start = false;
        g_threads[i].tm_priority = 31u;
        g_threads[i].entry = NULL;
        g_threads[i].materialized = false;
    }
    for (int i = 0; i < TM_TAKTOS_MAX_TM_IDS; ++i)
        g_id_to_slot[i] = -1;
    for (int i = 0; i < TM_TAKTOS_MAX_QUEUES; ++i)     g_queue_created[i] = false;
    for (int i = 0; i < TM_TAKTOS_MAX_SEMAPHORES; ++i) g_semaphore_created[i] = false;
    for (int i = 0; i < TM_TAKTOS_MAX_MUTEXES; ++i)    g_mutex_created[i] = false;

    g_kernel_started = false;

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
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_TM_IDS ||
        priority < 1 || priority > 31 || entry_function == NULL)
        return TM_ERROR;

    int slot = tm_alloc_slot(thread_id);
    if (slot < 0) return TM_ERROR;

    TmThreadDesc_t *t = &g_threads[slot];
    t->allocated = true;
    t->resume_before_start = false;
    t->tm_priority = (uint8_t)priority;
    t->entry = entry_function;

    if (g_kernel_started) {
        if (tm_materialize_slot(slot) != TM_SUCCESS) return TM_ERROR;
        return (t->thread.Suspend() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
    }
    return TM_SUCCESS;
}

extern "C" int tm_thread_resume(int thread_id)
{
    int slot = tm_slot_of(thread_id);
    if (slot < 0 || !g_threads[slot].allocated) return TM_ERROR;
    if (!g_kernel_started) {
        g_threads[slot].resume_before_start = true;
        return TM_SUCCESS;
    }
    return (g_threads[slot].thread.Resume() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_thread_suspend(int thread_id)
{
    int slot = tm_slot_of(thread_id);
    if (slot < 0 || !g_threads[slot].allocated) return TM_ERROR;
    if (!g_kernel_started) {
        g_threads[slot].resume_before_start = false;
        return TM_SUCCESS;
    }
    return (g_threads[slot].thread.Suspend() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" void tm_thread_relinquish(void)    { TaktOSThreadYield(); }
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

extern "C" int tm_queue_create(int queue_id)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES) return TM_ERROR;
    if (g_queues[queue_id].Init(g_queue_storage[queue_id],
                                TM_TAKTOS_QUEUE_MSG_SIZE,
                                TM_TAKTOS_QUEUE_DEPTH) != TAKTOS_OK)
        return TM_ERROR;
    g_queue_created[queue_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES ||
        !g_queue_created[queue_id] || message_ptr == NULL) return TM_ERROR;
    return (g_queues[queue_id].Send(message_ptr, false) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES ||
        !g_queue_created[queue_id] || message_ptr == NULL) return TM_ERROR;
    return (g_queues[queue_id].Receive(message_ptr, false, TAKTOS_NO_WAIT) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES) return TM_ERROR;
    if (g_semaphores[semaphore_id].Init(1u, 1u) != TAKTOS_OK) return TM_ERROR;
    g_semaphore_created[semaphore_id] = true;
    return TM_SUCCESS;
}

extern "C" int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES ||
        !g_semaphore_created[semaphore_id]) return TM_ERROR;
    return (g_semaphores[semaphore_id].Take(false, TAKTOS_NO_WAIT) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES ||
        !g_semaphore_created[semaphore_id]) return TM_ERROR;
    return (g_semaphores[semaphore_id].Give(false) == TAKTOS_OK)
                ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES) return TM_ERROR;
    if (g_mutexes[mutex_id].Init() != TAKTOS_OK) return TM_ERROR;
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

// TM5 not run on TaktOS (no heap).
extern "C" int tm_memory_pool_create(int pool_id)                             { (void)pool_id; return TM_ERROR; }
extern "C" int tm_memory_pool_allocate(int pool_id, unsigned char **p)        { (void)pool_id; (void)p; return TM_ERROR; }
extern "C" int tm_memory_pool_deallocate(int pool_id, unsigned char *p)       { (void)pool_id; (void)p; return TM_ERROR; }

extern "C" void tm_cause_interrupt(void)
{
    NVIC_STIR = TM_G4_SWI_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

extern "C" void tm_irq_vector_handler(void);
extern "C" void tm_irq_vector_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

// Strong override of startup's weak TIM7_DAC_IRQHandler → our handler.
extern "C" void TIM7_DAC_IRQHandler(void) __attribute__((weak, alias("tm_irq_vector_handler")));
