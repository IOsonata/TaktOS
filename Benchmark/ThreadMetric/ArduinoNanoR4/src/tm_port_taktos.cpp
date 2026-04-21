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
// Target: bare-metal Cortex-M4 with FPU (FPv4-SP-D16) / Renesas RA4M1 on
//         Arduino Nano R4. 48 MHz HOCO, 32 KB SRAM, 256 KB Flash, MPU.
// Output: SCI2 polling mode on P301 (D1, TX) / P302 (D0, RX). See
//         tm_console_nano_r4.cpp. Requires an external USB-UART adapter
//         (CP2102 / FTDI / CH340) on D1 — the Nano R4's native USB-C is
//         driven by the RA4M1 USBFS peripheral and is NOT used here.
// Tick  : 1000 Hz (1 ms).
//
// Behaviour notes:
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1  (highest) -> TaktOS 31
//       TM 31 (lowest)  -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a
//     two-phase materialisation model during tm_initialize().
//   - tm_cause_interrupt() pulses NVIC pending for IRQ 31. On the RA4M1 the
//     NVIC is fed by the ICU via IELSR0..IELSR31. Slot 31 is left unassigned
//     (IELSRn.IELS = 0x00, no peripheral event source), so only the NVIC
//     pending bit is touched — safe and collision-free. The startup file
//     weak-aliases IEL31_IRQHandler to Default_Handler; this port provides
//     the strong symbol via __attribute__((alias)).
//   - Cortex-M4 with FPU: the TaktOS_M4 ReleaseFPU library handles lazy VFP
//     stacking and S16–S31 callee-save in PendSV_M4.S; nothing port-specific
//     is required here. The startup code enables CPACR CP10/CP11 BEFORE
//     any C code or VFP instruction runs.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Slot-array design (copied from the STM32F0308 port): Thread-Metric hands
// out thread_ids in the range 0..31 and tests use sparse subsets. Rather
// than reserve a full TCB for every TM id (which wastes SRAM), we map each
// used id onto a compact slot array `g_threads[TM_TAKTOS_MAX_SLOTS]`.
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
// RA4M1 has 32 KB SRAM so slot count is not tight like it was on F030R8;
// we could pin MAX_SLOTS = 9 universally. Per-project scoping is retained
// for consistency with the other Cortex-M ports and so small tests keep
// their .bss footprint small.
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
#  define TM_TAKTOS_STACK_BYTES    1024u   // generous — 32 KB SRAM available
#endif
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_CORE_CLOCK_HZ    48000000u

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

// -----------------------------------------------------------------------------
// Software-interrupt wiring — RA4M1 ICU slot 31.
//
// RA4M1 delivers up to 32 peripheral events to the NVIC via IELSR0..IELSR31.
// Each IELSRn has IELS[7:0] selecting which event source fires that NVIC
// line. Slot 31 is deliberately left unassigned here (IELS = 0) so no
// peripheral ever competes for it; tm_cause_interrupt() just pends the NVIC
// bit via ISPR. ICU_IELSR31 is still touched once at init to make the
// "no event assigned" state explicit for review.
//
// The Cortex-M4 NVIC provides STIR (0xE000EF00) which can trigger any
// IRQ by software — simpler than the ISPR approach on Cortex-M0. We use
// STIR here because it is one instruction (str) and is the standard M4
// mechanism the nRF52832 port already exercises.
// -----------------------------------------------------------------------------
#define TM_RA4_SWI_IRQ_N           31u     // ICU event link slot 31

// Cortex-M4 NVIC + SCB register addresses.
#define SCB_SHPR3_ADDR             0xE000ED20UL
#define NVIC_ISER_ADDR             0xE000E100UL
#define NVIC_ISPR_ADDR             0xE000E200UL
#define NVIC_IPR_ADDR              0xE000E400UL  // byte-addressable on M4
#define NVIC_STIR_ADDR             0xE000EF00UL

#define SCB_SHPR3                  (*(volatile uint32_t*)SCB_SHPR3_ADDR)
#define NVIC_ISER                  ((volatile uint32_t*)NVIC_ISER_ADDR)
#define NVIC_ISPR                  ((volatile uint32_t*)NVIC_ISPR_ADDR)
#define NVIC_IPR_BYTE              ((volatile uint8_t*)NVIC_IPR_ADDR)
#define NVIC_STIR                  (*(volatile uint32_t*)NVIC_STIR_ADDR)

// RA4M1 ICU IELSRn — one 32-bit register per NVIC line, low byte = event.
#define ICU_IELSR_BASE             0x40006300UL
#define ICU_IELSR(n)               (*(volatile uint32_t*)(ICU_IELSR_BASE + 4u*(n)))

// -----------------------------------------------------------------------------
// Per-thread descriptor. Thread stack is inlined so all port state is in
// .bss — no heap.
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

// Weak console stubs — tm_console_nano_r4.cpp overrides these at link time.
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
// RA4M1 has 4 implemented priority bits (top 4 of each 8-bit byte). Valid
// priority byte values range 0x00 (highest) through 0xF0 (lowest) in steps
// of 0x10.
//
// PendSV and SysTick -> 0xF0 (lowest). TaktOS context switch must be the
// lowest-priority exception so interrupts preempt it cleanly.
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
// Software-interrupt slot init.
//
// IELSR31 is left at 0 (no event source assigned). IPR byte for IRQ 31 is
// set to 0xC0 — higher priority than PendSV/SysTick (0xF0) so the ISR runs
// first and PendSV tail-chains afterwards if a higher-priority task was
// readied. Then NVIC_ISER bit 31 is set to enable the line.
// -----------------------------------------------------------------------------
static void tm_enable_software_interrupt(void)
{
    ICU_IELSR(TM_RA4_SWI_IRQ_N) = 0u;              // no peripheral event
    NVIC_IPR_BYTE[TM_RA4_SWI_IRQ_N] = 0xC0u;       // above PendSV/SysTick
    NVIC_ISER[TM_RA4_SWI_IRQ_N >> 5] = (1u << (TM_RA4_SWI_IRQ_N & 31u));
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
    // Cortex-M4 STIR (Software Trigger Interrupt Register) — one MMIO write
    // triggers the selected NVIC line. Lower 9 bits = IRQ number.
    NVIC_STIR = TM_RA4_SWI_IRQ_N;
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// Strong IEL31_IRQHandler — overrides the Default_Handler alias from startup.
// -----------------------------------------------------------------------------
extern "C" void tm_irq_vector_handler(void);
extern "C" void tm_irq_vector_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

extern "C" void IEL31_IRQHandler(void) __attribute__((weak, alias("tm_irq_vector_handler")));
