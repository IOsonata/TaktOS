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
// Target: bare-metal Cortex-M0 / STM32F030R8 on STM32F0308-DISCO.
//         48 MHz HSI+PLL, no FPU, 8 KB RAM, 64 KB Flash.
// Output: USART1 polling mode on PA9 (TX) / PA10 (RX), exposed on the board
//         as the ST-LINK/V2-1 Virtual COM Port. See tm_console_stm32f0308.cpp.
// Tick  : 1000 Hz (1 ms).
//
// Behavior notes:
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1 (highest) -> TaktOS 31
//       TM 31 (lowest) -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a two-phase
//     materialization model during tm_initialize().
//   - tm_cause_interrupt() uses NVIC ISPR to pend TIM17_IRQn (IRQ 22). TIM17
//     is never clocked or configured here; only its NVIC pending bit is
//     touched. A weak alias wires TIM17_IRQHandler to tm_irq_vector_handler().
//
// RAM budget (8 KB total on F030R8):
//   g_threads (9 × (16 hdr + ~718 stack)) ~= 6.5 KB
//   queue storage + sem/mutex/queue objects + UART  ~= 0.5 KB
//   kernel state + main MSP + bss headroom          ~= 1.0 KB
//   Total                                           ~= 8.0 KB  (tight)
//
// If a test fails to fit or stack-overflows, reduce TM_TAKTOS_MAX_SLOTS or
// TM_TAKTOS_STACK_BYTES below. The basic/cooperative/preemptive/message/
// synchronization/mutex tests need at most 6 threads; only mutex_barging
// needs 9 (4 workers + 4 interlopers + reporter).
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Sizing note: 8 KB RAM is tight. The basic_processing test reserves a 4 KB
// workload array of its own, leaving ~3 KB for all TaktOS thread state. On
// M0 each thread costs ~622 B of fixed overhead (256 B guard align + 256 B
// guard region + 64 B init frame + alignment slack) + its stack. To keep
// every test buildable with the same port, this port remaps Thread-Metric
// thread_ids (0..31) to a compact slot array `g_threads[TM_TAKTOS_MAX_SLOTS]`.
// Tests that only use thread_id 0 and 5 (like basic_processing) consume
// exactly 2 slots rather than 6.
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
// -----------------------------------------------------------------------------

#ifndef TM_TAKTOS_MAX_SLOTS
#  define TM_TAKTOS_MAX_SLOTS      9       // fall-back for the worst case
#endif
#define TM_TAKTOS_MAX_TM_IDS       32      // Thread-Metric thread IDs 0..31
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_MUTEXES      1
#define TM_TAKTOS_MAX_POOLS        0       // TM5 not run on F0308 (no heap)

#ifndef TM_TAKTOS_STACK_BYTES
#  define TM_TAKTOS_STACK_BYTES    64u     // tight but verified to link in 8 KB RAM
#endif
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_CORE_CLOCK_HZ    48000000u

#define TM_TAKTOS_QUEUE_DEPTH      4u      // each entry is 4 words = 16 B
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

// -----------------------------------------------------------------------------
// Software-interrupt wiring
//
// STM32F030R8 has no STIR (Cortex-M0). tm_cause_interrupt() pends the chosen
// IRQ via NVIC_ISPR. TIM17_IRQn (IRQ 22) is unused by the port and its CMSIS
// handler name aliases cleanly via __attribute__((alias)).
//
// ARMv6-M NVIC register access: NVIC_IPR is word-access only on M0 (byte
// access is unpredictable). This port always uses 32-bit reads/modify/writes.
// -----------------------------------------------------------------------------
#define TM_F0_SWI_IRQ_N            22u     // TIM17_IRQn

// Cortex-M0 NVIC + SCB register addresses (ARMv6-M).
#define SCB_ICSR_ADDR              0xE000ED04UL
#define SCB_SHPR3_ADDR             0xE000ED20UL
#define NVIC_ISER_ADDR             0xE000E100UL
#define NVIC_ISPR_ADDR             0xE000E200UL
#define NVIC_IPR_ADDR              0xE000E400UL

#define SCB_SHPR3                  (*(volatile uint32_t*)SCB_SHPR3_ADDR)
#define NVIC_ISER                  ((volatile uint32_t*)NVIC_ISER_ADDR)
#define NVIC_ISPR                  ((volatile uint32_t*)NVIC_ISPR_ADDR)
#define NVIC_IPR_WORD              ((volatile uint32_t*)NVIC_IPR_ADDR)

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
    uint8_t         mem[TAKTOS_THREAD_MEM_SIZE(TM_TAKTOS_STACK_BYTES)] __attribute__((aligned(4)));
} TmThreadDesc_t;
#pragma pack(pop)

static TmThreadDesc_t  g_threads[TM_TAKTOS_MAX_SLOTS];
// Map Thread-Metric ID (0..31) -> slot index (0..MAX_SLOTS-1), or -1 if unmapped.
static int8_t          g_id_to_slot[TM_TAKTOS_MAX_TM_IDS];
static bool            g_kernel_started = false;

// Helper: allocate a free slot for a new thread_id; return -1 if all full.
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

// Helper: look up existing slot (returns -1 if unmapped).
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

// Weak no-op hooks - a test file can override to run work from the ISR.
extern "C" void tm_interrupt_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_preemption_handler(void) __attribute__((weak));
extern "C" void tm_interrupt_handler(void) {}
extern "C" void tm_interrupt_preemption_handler(void) {}

// Weak console stubs - the concrete driver in tm_console_stm32f0308.cpp
// overrides these at link time.
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
// Cortex-M0 has only 2 implemented priority bits (top 2 of each 8-bit byte).
// Valid priority byte values: 0x00 (highest), 0x40, 0x80, 0xC0 (lowest).
//
// PendSV and SysTick -> 0xC0 (lowest). TaktOS context switch must be the
// lowest-priority exception so interrupts preempt it.
// -----------------------------------------------------------------------------
static void tm_set_system_handler_priorities(void)
{
    // SHPR3: [31:24] = SysTick, [23:16] = PendSV
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xC0u << 16);      // PendSV
    v |= (0xC0u << 24);      // SysTick
    SCB_SHPR3 = v;
}

// -----------------------------------------------------------------------------
// Boot-time diagnostic: verifies SysTick is actually counting and that the
// core clock matches what TaktOS is told (TM_TAKTOS_CORE_CLOCK_HZ).
//
// When TM_PORT_BOOT_DIAGNOSE is defined (set via -D in the .cproject), this
// runs BEFORE TaktOSInit. It prints one '.' every ~100 ms for ~1 s using the
// SysTick counter directly (poll COUNTFLAG), then releases SysTick back to
// TaktOS's kernel.
//
// Expected output: "SYS:" then ~10 dots in ~1 second, then "OK".
// - If dots come much faster: core clock is higher than expected, or LOAD is
//   being set to a smaller value than we compute here.
// - If dots come much slower: SystemInit() failed and we're running on HSI
//   (8 MHz) instead of PLL (48 MHz).
// - If no dots: SysTick isn't counting. Check CLKSOURCE bit; on F030 the
//   HCLK/8 external reference path is ~6 MHz and would give a slow count.
// -----------------------------------------------------------------------------
#define SYSTICK_CTRL               (*(volatile uint32_t*)0xE000E010UL)
#define SYSTICK_LOAD               (*(volatile uint32_t*)0xE000E014UL)
#define SYSTICK_VAL                (*(volatile uint32_t*)0xE000E018UL)
#define SYSTICK_CTRL_COUNTFLAG     (1u << 16)
#define SYSTICK_CTRL_CLKSOURCE     (1u << 2)
#define SYSTICK_CTRL_TICKINT       (1u << 1)
#define SYSTICK_CTRL_ENABLE        (1u << 0)

extern "C" void tm_putchar(int c);

static void tm_port_puts(const char *s) { while (*s) tm_putchar(*s++); }

static void tm_port_diagnose_boot(void)
{
    tm_port_puts("SYS:");

    // Disable any prior SysTick state. Configure for a 100 ms period at the
    // CORE CLOCK we believe we're running (48 MHz -> LOAD = 48000000/10 - 1).
    SYSTICK_CTRL = 0u;
    SYSTICK_LOAD = (TM_TAKTOS_CORE_CLOCK_HZ / 10u) - 1u;
    SYSTICK_VAL  = 0u;
    SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_ENABLE;  // processor clk, no IRQ

    // Poll COUNTFLAG 10 times = 10 wraps = 1 second wall-clock at 48 MHz.
    for (int i = 0; i < 10; ++i) {
        // COUNTFLAG sets on wrap, clears on CTRL read. Spin until set.
        uint32_t ctrl;
        do {
            ctrl = SYSTICK_CTRL;
        } while ((ctrl & SYSTICK_CTRL_COUNTFLAG) == 0u);
        tm_putchar('.');
    }

    // Release SysTick - TaktOSTickInit will reconfigure it for 1 kHz.
    SYSTICK_CTRL = 0u;
    SYSTICK_VAL  = 0u;
    tm_port_puts("OK\n");
}

static void tm_enable_software_interrupt(void)
{
    // SW IRQ at 0x80 - higher than PendSV (0xC0, lowest) so the ISR runs
    // promptly; PendSV then tail-chains to do the scheduling work.
    //
    // ARMv6-M: NVIC_IPR access must be word-aligned. IPR[n/4] covers 4
    // consecutive IRQs; byte (n%4) within that word is the priority for IRQ n.
    const uint32_t word  = TM_F0_SWI_IRQ_N >> 2;
    const uint32_t shift = (TM_F0_SWI_IRQ_N & 3u) * 8u;
    uint32_t v = NVIC_IPR_WORD[word];
    v &= ~(0xFFu << shift);
    v |=  (0x80u << shift);
    NVIC_IPR_WORD[word] = v;

    NVIC_ISER[TM_F0_SWI_IRQ_N >> 5] = (1u << (TM_F0_SWI_IRQ_N & 31u));
}

// -----------------------------------------------------------------------------
// Thread materialization
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
    if (slot < 0 || slot >= TM_TAKTOS_MAX_SLOTS)
        return TM_ERROR;

    TmThreadDesc_t *t = &g_threads[slot];
    if (!t->allocated)
        return TM_ERROR;
    if (t->materialized)
        return TM_SUCCESS;

    if (t->thread.Create(t->mem,
                         (uint32_t)sizeof(t->mem),
                         tm_thread_trampoline,
                         (void*)(uintptr_t)slot,
                         tm_to_taktos_priority(t->tm_priority)) == NULL)
        return TM_ERROR;

    t->materialized = true;
    return TM_SUCCESS;
}

static void tm_materialize_all_threads(void)
{
    int s;
    for (s = 0; s < TM_TAKTOS_MAX_SLOTS; ++s) {
        if (g_threads[s].allocated && tm_materialize_slot(s) != TM_SUCCESS) {
            tm_check_fail("FATAL: TaktOS thread creation failed\n");
        }
    }
    for (s = 0; s < TM_TAKTOS_MAX_SLOTS; ++s) {
        if (g_threads[s].allocated && !g_threads[s].resume_before_start) {
            if (g_threads[s].thread.Suspend() != TAKTOS_OK) {
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

    for (i = 0; i < TM_TAKTOS_MAX_SLOTS; ++i) {
        g_threads[i].allocated = false;
        g_threads[i].resume_before_start = false;
        g_threads[i].tm_priority = 31u;
        g_threads[i].entry = NULL;
        g_threads[i].materialized = false;
    }
    for (i = 0; i < TM_TAKTOS_MAX_TM_IDS; ++i) {
        g_id_to_slot[i] = -1;
    }
    for (i = 0; i < TM_TAKTOS_MAX_QUEUES; ++i) {
        g_queue_created[i] = false;
    }
    for (i = 0; i < TM_TAKTOS_MAX_SEMAPHORES; ++i) {
        g_semaphore_created[i] = false;
    }
    for (i = 0; i < TM_TAKTOS_MAX_MUTEXES; ++i) {
        g_mutex_created[i] = false;
    }

    g_kernel_started = false;

    tm_hw_console_init();

#ifdef TM_PORT_BOOT_DIAGNOSE
    tm_port_diagnose_boot();
#endif

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
        // Out of slots - test asked for more simultaneous threads than
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

// ----- Memory pool (not supported on F0308 - 8 KB RAM is too tight) --------
extern "C" int tm_memory_pool_create(int pool_id)
{
    (void)pool_id;
    return TM_ERROR;   // TM5 is not run on TaktOS anyway
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
    NVIC_ISPR[TM_F0_SWI_IRQ_N >> 5] = (1u << (TM_F0_SWI_IRQ_N & 31u));
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// Interrupt handler: borrowed TIM17_IRQHandler for tm_cause_interrupt().
// The startup assembly weak-aliases TIM17_IRQHandler to Default_Handler,
// and this strong symbol (via __attribute__((alias))) overrides that.
// -----------------------------------------------------------------------------
extern "C" void tm_irq_vector_handler(void);
extern "C" void tm_irq_vector_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

extern "C" void TIM17_IRQHandler(void) __attribute__((weak, alias("tm_irq_vector_handler")));
