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
// Target: bare-metal Cortex-M7 / STM32H753ZI on NUCLEO-H753ZI.
//         64 MHz HSI (reset default), 128 KB DTCM SRAM, 2 MB Flash,
//         I-cache + D-cache enabled, MPU.
//         Soft-float build — links against libTaktOS_M7.a (Release config).
// Output: USART3 polling mode on PD8 (TX) / PD9 (RX) → ST-LINK/V3 VCP.
//         See tm_console_nucleo_h753.cpp.
// Tick  : 1000 Hz (1 ms).
//
// Behaviour notes:
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1  (highest) -> TaktOS 31
//       TM 31 (lowest)  -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a
//     two-phase materialisation model during tm_initialize().
//   - tm_cause_interrupt() pulses NVIC pending for TIM7_IRQn (IRQ 55). TIM7
//     is intentionally NEVER clocked (RCC_APB1LENR.TIM7EN stays 0), so no
//     peripheral event can fire on this line — only the NVIC pending bit
//     is touched via STIR. The startup file weak-aliases TIM7_IRQHandler
//     to Default_Handler; this port provides the strong symbol via
//     __attribute__((alias)) at the end of the file.
//   - Cortex-M7: 4 NVIC priority bits. PendSV/SysTick use 0xF0 (lowest);
//     the SWI line uses 0xC0 so it preempts and lets PendSV tail-chain.
//   - Soft-float: the compiler is not passed -mfpu / -mfloat-abi, matching
//     the soft-float libTaktOS_M7.a. CPACR CP10/CP11 are left at reset
//     (access denied). No VFP state, no lazy stacking concerns.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Slot-array design (copied from the STM32F0308 / Arduino Nano R4 ports):
// Thread-Metric hands out thread_ids in the range 0..31 and tests use
// sparse subsets. Rather than reserve a full TCB for every TM id (which
// wastes SRAM), we map each used id onto a compact slot array
// g_threads[TM_TAKTOS_MAX_SLOTS].
//
// TM_TAKTOS_MAX_SLOTS is defined per-project in the .cproject. Required:
//
//     BasicProcessing          : 2 slots  (threads 0, 5)
//     CooperativeScheduling    : 6        (0..5)
//     PreemptiveScheduling     : 6        (0..5)
//     MessageProcessing        : 2        (0..1)
//     SynchronizationProcessing: 2        (0..1)
//     MutexProcessing          : 4        (0..3)
//     MutexBargingTest         : 9        (0..7 + reporter)
//
// STM32H753ZI has 128 KB DTCM — slot count is not tight. Per-project
// scoping is kept for consistency with the other Cortex-M ports.
// -----------------------------------------------------------------------------

#ifndef TM_TAKTOS_MAX_SLOTS
#  define TM_TAKTOS_MAX_SLOTS      9       // fall-back for the worst case
#endif
#define TM_TAKTOS_MAX_TM_IDS       32      // Thread-Metric thread IDs 0..31
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_MUTEXES      1
#define TM_TAKTOS_MAX_POOLS        0       // TM5 not run on TaktOS (no heap)

#ifndef TM_TAKTOS_STACK_BYTES
#  define TM_TAKTOS_STACK_BYTES    2048u   // generous — 128 KB DTCM available
#endif
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_CORE_CLOCK_HZ    64000000u

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

// -----------------------------------------------------------------------------
// Software-interrupt wiring — TIM7_IRQn = 55 on STM32H7.
//
// Cortex-M7 provides STIR (0xE000EF00) which can trigger any NVIC line by
// software. We pick TIM7 because its peripheral clock (RCC_APB1LENR.TIM7EN)
// is off at reset and this port never enables it, so no legitimate TIM7
// event can ever compete with our software trigger.
// -----------------------------------------------------------------------------
#define TM_H7_SWI_IRQ_N            55u     // TIM7_IRQn on STM32H7

// Cortex-M7 NVIC + SCB register addresses.
#define SCB_SHPR3_ADDR             0xE000ED20UL
#define NVIC_ISER_ADDR             0xE000E100UL
#define NVIC_IPR_ADDR              0xE000E400UL  // byte-addressable on M7
#define NVIC_STIR_ADDR             0xE000EF00UL

#define SCB_SHPR3                  (*(volatile uint32_t*)SCB_SHPR3_ADDR)
#define NVIC_ISER                  ((volatile uint32_t*)NVIC_ISER_ADDR)
#define NVIC_IPR_BYTE              ((volatile uint8_t*)NVIC_IPR_ADDR)
#define NVIC_STIR                  (*(volatile uint32_t*)NVIC_STIR_ADDR)

// -----------------------------------------------------------------------------
// Per-thread descriptor. Thread stack is inlined so all port state is in
// .bss (DTCM) — no heap.
// -----------------------------------------------------------------------------
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

// Weak no-op hooks — a test file may override to run work from the ISR.
extern "C" void tm_interrupt_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_preemption_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_handler(void) {}
extern "C" void tm_interrupt_preemption_handler(void) {}

// Weak console stubs — tm_console_nucleo_h753.cpp overrides these at link time.
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
// STM32H7 implements 4 NVIC priority bits (top 4 of each 8-bit byte). Valid
// priority byte values range 0x00 (highest) through 0xF0 (lowest) in steps
// of 0x10.
//
// PendSV and SysTick -> 0xF0 (lowest) so TaktOS context switching does not
// preempt peripheral ISRs.
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

// -----------------------------------------------------------------------------
// Software-interrupt enable.
//
// Set TIM7_IRQ IPR byte to 0xC0 — higher priority than PendSV/SysTick
// (0xF0) so the ISR runs first and PendSV tail-chains afterwards if a
// higher-priority task was readied. Then enable the NVIC line via ISER.
// -----------------------------------------------------------------------------
static void tm_enable_software_interrupt(void)
{
    NVIC_IPR_BYTE[TM_H7_SWI_IRQ_N] = 0xC0u;                     // above PendSV/SysTick
    NVIC_ISER[TM_H7_SWI_IRQ_N >> 5] = (1u << (TM_H7_SWI_IRQ_N & 31u));
}

// -----------------------------------------------------------------------------
// Thread trampoline: launches the test's entry function and loops forever
// afterwards (Thread-Metric entry functions never return).
// -----------------------------------------------------------------------------
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

    // TaktOSThread::Create returns the thread handle (hTaktOSThread_t),
    // not TaktOSErr_t. Success = non-null handle; failure = nullptr.
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

// -----------------------------------------------------------------------------
// tm_initialize — Thread-Metric entry point. Called from tm_main() in each
// test harness.
// -----------------------------------------------------------------------------
extern "C" void tm_initialize(void (*test_initialization_function)(void))
{
    for (int i = 0; i < TM_TAKTOS_MAX_SLOTS; ++i) {
        g_threads[i].allocated = false;
        g_threads[i].resume_before_start = false;
        g_threads[i].tm_priority = 31u;
        g_threads[i].entry = NULL;
        g_threads[i].materialized = false;
    }
    for (int i = 0; i < TM_TAKTOS_MAX_TM_IDS; ++i) {
        g_id_to_slot[i] = -1;
    }
    for (int i = 0; i < TM_TAKTOS_MAX_QUEUES; ++i)    g_queue_created[i] = false;
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
    if (slot < 0) {
        // Out of slots — test asked for more simultaneous threads than
        // TM_TAKTOS_MAX_SLOTS. Either raise MAX_SLOTS (cost: RAM) or pick
        // a smaller test.
        return TM_ERROR;
    }

    TmThreadDesc_t *t = &g_threads[slot];
    t->allocated = true;
    t->resume_before_start = false;
    t->tm_priority = (uint8_t)priority;
    t->entry = entry_function;

    if (g_kernel_started) {
        if (tm_materialize_slot(slot) != TM_SUCCESS)
            return TM_ERROR;
        return (t->thread.Suspend() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
    }
    return TM_SUCCESS;
}

extern "C" int tm_thread_resume(int thread_id)
{
    int slot = tm_slot_of(thread_id);
    if (slot < 0 || !g_threads[slot].allocated)
        return TM_ERROR;
    if (!g_kernel_started) {
        g_threads[slot].resume_before_start = true;
        return TM_SUCCESS;
    }
    return (g_threads[slot].thread.Resume() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

extern "C" int tm_thread_suspend(int thread_id)
{
    int slot = tm_slot_of(thread_id);
    if (slot < 0 || !g_threads[slot].allocated)
        return TM_ERROR;
    if (!g_kernel_started) {
        g_threads[slot].resume_before_start = false;
        return TM_SUCCESS;
    }
    return (g_threads[slot].thread.Suspend() == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
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

// ----- Memory pool (not run — TaktOS has no heap) ---------------------------
extern "C" int tm_memory_pool_create(int pool_id)
{
    (void)pool_id;
    return TM_ERROR;
}
extern "C" int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
    (void)pool_id; (void)memory_ptr;
    return TM_ERROR;
}
extern "C" int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    (void)pool_id; (void)memory_ptr;
    return TM_ERROR;
}

// ----- Software interrupt ---------------------------------------------------
extern "C" void tm_cause_interrupt(void)
{
    // Cortex-M7 STIR — one MMIO write triggers the selected NVIC line.
    // Lower 9 bits = IRQ number.
    NVIC_STIR = TM_H7_SWI_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// Strong TIM7_IRQHandler — overrides the Default_Handler alias from startup.
// -----------------------------------------------------------------------------
extern "C" void tm_irq_vector_handler(void);
extern "C" void tm_irq_vector_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

extern "C" void TIM7_IRQHandler(void) __attribute__((weak, alias("tm_irq_vector_handler")));
