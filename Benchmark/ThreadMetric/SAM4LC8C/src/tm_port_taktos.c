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
// Target: bare-metal Cortex-M4 / ATSAM4LC8C (Atmel / Microchip SAM4L, 48 MHz,
//         no FPU, soft-float ABI).
// Output: USART2 polling mode (no DMA) on PC11 (RXD func A) / PC12 (TXD func A)
//         by default. Adjust TM_SAM4L_USART_* defines below to match your
//         board wiring if different.
// Tick  : 1000 Hz (1 ms).
//
// Important behavior notes:
//   - Thread-Metric priorities are inverted relative to TaktOS:
//       TM 1 (highest)  -> TaktOS 31
//       TM 31 (lowest)  -> TaktOS 1
//   - tm_thread_create() semantics are "create suspended". TaktOS does not
//     natively support pre-start resume/suspend, so this port uses a two-phase
//     materialization model during tm_initialize().
//   - Queue and memory-pool semantics match the official harness expectations.
//   - tm_cause_interrupt() uses a pended peripheral IRQ via NVIC_ISPR. SAM4L
//     does not implement STIR, and has no dedicated software-interrupt block,
//     so we borrow an unused peripheral's NVIC line (TRNG by default, IRQ 69).
//     The TRNG peripheral itself is never clocked or configured here; only its
//     NVIC pending bit is touched. This is the same pattern vendor RTOS ports
//     use on MCUs without a dedicated SWI peripheral.
// -----------------------------------------------------------------------------

#define TM_TAKTOS_MAX_THREADS      15
#define TM_TAKTOS_MAX_QUEUES       1
#define TM_TAKTOS_MAX_SEMAPHORES   1
#define TM_TAKTOS_MAX_MUTEXES      1
#define TM_TAKTOS_MAX_POOLS        1

#define TM_TAKTOS_STACK_BYTES      1024u
#define TM_TAKTOS_TICK_HZ          1000u
#define TM_TAKTOS_CORE_CLOCK_HZ    48000000u   // post-PLL CPU clock

#define TM_TAKTOS_QUEUE_DEPTH      10u
#define TM_TAKTOS_QUEUE_MSG_SIZE   (4u * sizeof(unsigned long))

#define TM_BLOCK_SIZE              128u
#define TM_POOL_SIZE               2048u
#define TM_BLOCK_COUNT             (TM_POOL_SIZE / TM_BLOCK_SIZE)

// -----------------------------------------------------------------------------
// SAM4L board-specific configuration - adjust for your hardware
// -----------------------------------------------------------------------------
// Default: USART2 on Port C pins 11 (RXD) / 12 (TXD), peripheral function A.
// Valid combinations vary by package; see SAM4L datasheet "GPIO Function
// Multiplexing" table. Change _USART_BASE, _TX_PORT/_TX_PIN, and _PBA_BIT
// together if you pick a different USART.
#define TM_SAM4L_USART_BASE        0x40028000u   // USART2 base (PBA)
#define TM_SAM4L_USART_PBA_BIT     10u           // PBAMASK bit for USART2
#define TM_SAM4L_TX_PORT           2u            // PortC = 2  (A=0, B=1, C=2)
#define TM_SAM4L_TX_PIN            12u           // PC12 = USART2 TXD function A
#define TM_SAM4L_TX_PERIPH_FN      0u            // A=0, B=1, ... H=7
#define TM_SAM4L_UART_BAUDRATE     115200u

// Pended-IRQ used by tm_cause_interrupt(). TRNG (IRQ 73 on SAM4L - per
// sam4lc8c.h) is a good default: its peripheral clock is gated off by
// SystemInit, the peripheral never raises its line on its own, and the
// handler name matches CMSIS convention.
// If your IOsonata startup file uses a non-CMSIS handler name, add a weak
// alias with your name pointing at tm_irq_vector_handler below.
#define TM_SAM4L_SWI_IRQ_N         73u

// -----------------------------------------------------------------------------
// SAM4L Power Manager (PM) - used to ungate the USART peripheral clock
// -----------------------------------------------------------------------------
#define SAM4L_PM_BASE              0x400E0000u
#define SAM4L_PM_PBAMASK           (*(volatile uint32_t*)(SAM4L_PM_BASE + 0x028u))
#define SAM4L_PM_UNLOCK            (*(volatile uint32_t*)(SAM4L_PM_BASE + 0x058u))
// Unlock key: 0xAA << 24 | register_offset. For PBAMASK (offset 0x028):
#define SAM4L_PM_UNLOCK_PBAMASK    (0xAAu << 24 | 0x028u)

// -----------------------------------------------------------------------------
// SAM4L GPIO controller - per-port, 0x200 stride
// -----------------------------------------------------------------------------
#define SAM4L_GPIO_BASE            0x400E1000u
#define SAM4L_GPIO_PORT(n)         (SAM4L_GPIO_BASE + (n) * 0x200u)
#define SAM4L_GPIO_GPERC(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x008u))
#define SAM4L_GPIO_PMR0S(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x014u))
#define SAM4L_GPIO_PMR0C(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x018u))
#define SAM4L_GPIO_PMR1S(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x024u))
#define SAM4L_GPIO_PMR1C(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x028u))
#define SAM4L_GPIO_PMR2S(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x034u))
#define SAM4L_GPIO_PMR2C(n)        (*(volatile uint32_t*)(SAM4L_GPIO_PORT(n) + 0x038u))

// -----------------------------------------------------------------------------
// SAM4L USART register offsets (same as other Atmel USARTs)
// -----------------------------------------------------------------------------
#define USART_CR                   (*(volatile uint32_t*)(TM_SAM4L_USART_BASE + 0x00u))
#define USART_MR                   (*(volatile uint32_t*)(TM_SAM4L_USART_BASE + 0x04u))
#define USART_CSR                  (*(volatile uint32_t*)(TM_SAM4L_USART_BASE + 0x14u))
#define USART_THR                  (*(volatile uint32_t*)(TM_SAM4L_USART_BASE + 0x1Cu))
#define USART_BRGR                 (*(volatile uint32_t*)(TM_SAM4L_USART_BASE + 0x20u))
// CR bits
#define USART_CR_RSTRX             (1u << 2)
#define USART_CR_RSTTX             (1u << 3)
#define USART_CR_TXEN              (1u << 6)
#define USART_CR_TXDIS             (1u << 7)
// CSR bits
#define USART_CSR_TXRDY            (1u << 1)
#define USART_CSR_TXEMPTY          (1u << 9)
// MR: normal mode, 8N1, CHRL=3, NBSTOP=0, PAR=4 (none)
#define USART_MR_NORMAL_8N1        ((3u << 6) | (4u << 9))

// -----------------------------------------------------------------------------
// NVIC / system handler priority registers (ARMv7-M standard)
// -----------------------------------------------------------------------------
#define SCB_SHPR3                  (*(volatile uint32_t*)0xE000ED20u)
#define NVIC_ISER                  ((volatile uint32_t*)0xE000E100u)
#define NVIC_IPR                   ((volatile uint8_t *)0xE000E400u)
#define NVIC_ISPR                  ((volatile uint32_t*)0xE000E200u)

typedef struct TmThreadDesc_s {
    bool            allocated;
    bool            resume_before_start;
    uint8_t         tm_priority;
    void          (*entry)(void);
    TaktOSThread_t *handle;
    uint8_t         mem[TAKTOS_THREAD_MEM_SIZE(TM_TAKTOS_STACK_BYTES)] __attribute__((aligned(4)));
} TmThreadDesc_t;

static TmThreadDesc_t  g_threads[TM_TAKTOS_MAX_THREADS];
static bool          g_kernel_started = false;

static TaktOSQueue_t g_queues[TM_TAKTOS_MAX_QUEUES];
static uint8_t       g_queue_storage[TM_TAKTOS_MAX_QUEUES]
                                   [TM_TAKTOS_QUEUE_DEPTH * TM_TAKTOS_QUEUE_MSG_SIZE]
                                   __attribute__((aligned(4)));
static bool          g_queue_created[TM_TAKTOS_MAX_QUEUES];

static TaktOSSem_t   g_semaphores[TM_TAKTOS_MAX_SEMAPHORES];
static bool          g_semaphore_created[TM_TAKTOS_MAX_SEMAPHORES];
static TaktOSMutex_t g_mutexes[TM_TAKTOS_MAX_MUTEXES];
static bool          g_mutex_created[TM_TAKTOS_MAX_MUTEXES];

static uint8_t       g_pool_area[TM_TAKTOS_MAX_POOLS][TM_POOL_SIZE] __attribute__((aligned(sizeof(void*))));
static void         *g_pool_free[TM_TAKTOS_MAX_POOLS];
static bool          g_pool_created[TM_TAKTOS_MAX_POOLS];

void tm_interrupt_handler(void) __attribute__((weak));
void tm_interrupt_preemption_handler(void) __attribute__((weak));
void tm_interrupt_handler(void) {}
void tm_interrupt_preemption_handler(void) {}

static inline uint8_t tm_to_taktos_priority(int tm_priority)
{
    if (tm_priority < 1)  tm_priority = 1;
    if (tm_priority > 31) tm_priority = 31;
    return (uint8_t)(32 - tm_priority);
}

// -----------------------------------------------------------------------------
// SAM4L peripheral clock + GPIO pin-mux + USART init
// -----------------------------------------------------------------------------
// Gated behind TM_SAM4L_ENABLE_DIRECT_UART. The register addresses below have
// NOT been verified against the SAM4L datasheet memory map. Default build
// does NOT poke these registers. The user should override tm_putchar() and
// tm_hw_console_init() in their own BSP file using IOsonata's UART driver.
// -----------------------------------------------------------------------------
#if defined(TM_SAM4L_ENABLE_DIRECT_UART) && TM_SAM4L_ENABLE_DIRECT_UART

static void tm_sam4l_ungate_usart_clock(void)
{
    // SAM4L protects clock-mask registers. Write UNLOCK with the mask register
    // offset + 0xAA key immediately before each mask write.
    SAM4L_PM_UNLOCK = SAM4L_PM_UNLOCK_PBAMASK;
    SAM4L_PM_PBAMASK |= (1u << TM_SAM4L_USART_PBA_BIT);
}

static void tm_sam4l_mux_tx_pin(void)
{
    const uint32_t port = TM_SAM4L_TX_PORT;
    const uint32_t mask = (1u << TM_SAM4L_TX_PIN);
    const uint32_t fn   = TM_SAM4L_TX_PERIPH_FN;

    // Program 3-bit peripheral-mux selector (PMR0/PMR1/PMR2 = fn bit0/bit1/bit2)
    if (fn & 1u) { SAM4L_GPIO_PMR0S(port) = mask; } else { SAM4L_GPIO_PMR0C(port) = mask; }
    if (fn & 2u) { SAM4L_GPIO_PMR1S(port) = mask; } else { SAM4L_GPIO_PMR1C(port) = mask; }
    if (fn & 4u) { SAM4L_GPIO_PMR2S(port) = mask; } else { SAM4L_GPIO_PMR2C(port) = mask; }

    // Disable GPIO function -> pin driven by selected peripheral
    SAM4L_GPIO_GPERC(port) = mask;
}

static void tm_sam4l_usart_init(void)
{
    USART_CR   = USART_CR_RSTRX | USART_CR_RSTTX | USART_CR_TXDIS;
    USART_MR   = USART_MR_NORMAL_8N1;
    // Oversample 16: baud = CLK / (16 * CD). CD = CLK / (16 * baud).
    USART_BRGR = TM_TAKTOS_CORE_CLOCK_HZ / (16u * TM_SAM4L_UART_BAUDRATE);
    USART_CR   = USART_CR_TXEN;
}

void tm_hw_console_init(void)
{
    tm_sam4l_ungate_usart_clock();
    tm_sam4l_mux_tx_pin();
    tm_sam4l_usart_init();
}

void tm_putchar(int c)
{
    uint32_t timeout = 1000000u;
    while ((USART_CSR & USART_CSR_TXRDY) == 0u) {
        if (--timeout == 0u) return;
    }
    USART_THR = (uint32_t)(c & 0xFFu);
}

#else  // TM_SAM4L_ENABLE_DIRECT_UART not defined - weak no-op fallbacks

__attribute__((weak)) void tm_hw_console_init(void) { }
__attribute__((weak)) void tm_putchar(int c)        { (void)c; }

#endif  // TM_SAM4L_ENABLE_DIRECT_UART

// -----------------------------------------------------------------------------
// Priority configuration
// -----------------------------------------------------------------------------
static void tm_set_system_handler_priorities(void)
{
    // PendSV = SHPR3[23:16], SysTick = SHPR3[31:24]
    uint32_t v = SCB_SHPR3;
    v &= 0x0000FFFFu;
    v |= (0xFFu << 16);     // PendSV = lowest
    v |= (0xFFu << 24);     // SysTick = lowest
    SCB_SHPR3 = v;
}

static void tm_enable_software_interrupt(void)
{
    // Priority: above PendSV / SysTick so the ISR preempts test tasks, then
    // PendSV tail-chains to complete any scheduling work.
    NVIC_IPR[TM_SAM4L_SWI_IRQ_N] = 0xC0u;
    NVIC_ISER[TM_SAM4L_SWI_IRQ_N >> 5] = (1u << (TM_SAM4L_SWI_IRQ_N & 31u));
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

// -----------------------------------------------------------------------------
// TM API implementation
// -----------------------------------------------------------------------------
void tm_initialize(void (*test_initialization_function)(void))
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

    tm_set_system_handler_priorities();
    if (TaktOSInit(TM_TAKTOS_CORE_CLOCK_HZ, TM_TAKTOS_TICK_HZ, TAKTOS_TICK_CLOCK_PROCESSOR, 0) != TAKTOS_OK) {
        tm_check_fail("FATAL: TaktOSInit failed\n");
    }

    test_initialization_function();

    tm_materialize_all_threads();
    tm_enable_software_interrupt();
    g_kernel_started = true;

    TaktOSStart();
    for (;;) {}
}

int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    TmThreadDesc_t *t;

    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS)
        return TM_ERROR;

    t = &g_threads[thread_id];
    t->allocated = true;
    t->tm_priority = (uint8_t)priority;
    t->entry = entry_function;

    if (g_kernel_started) {
        if (tm_materialize_thread(thread_id) != TM_SUCCESS)
            return TM_ERROR;
        return (TaktOSThreadSuspend(t->handle) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
    }

    return TM_SUCCESS;
}

int tm_thread_resume(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;

    if (!g_kernel_started) {
        g_threads[thread_id].resume_before_start = true;
        return TM_SUCCESS;
    }

    return (TaktOSThreadResume(g_threads[thread_id].handle) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_suspend(int thread_id)
{
    if (thread_id < 0 || thread_id >= TM_TAKTOS_MAX_THREADS || !g_threads[thread_id].allocated)
        return TM_ERROR;

    if (!g_kernel_started) {
        g_threads[thread_id].resume_before_start = false;
        return TM_SUCCESS;
    }

    return (TaktOSThreadSuspend(g_threads[thread_id].handle) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

void tm_thread_relinquish(void)
{
    TaktOSThreadYield();
}

void tm_thread_sleep(int seconds)
{
    uint32_t ticks = (seconds <= 0) ? 0u : (uint32_t)seconds * TM_TAKTOS_TICK_HZ;
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), ticks);
}

void tm_thread_sleep_ticks(int ticks)
{
    uint32_t t = (uint32_t)(ticks <= 0 ? 1 : ticks);
    (void)TaktOSThreadSleep(TaktOSCurrentThread(), t);
}

int tm_queue_create(int queue_id)
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

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;

    return (TaktOSQueueSend(&g_queues[queue_id], message_ptr, false, TAKTOS_WAIT_FOREVER) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    if (queue_id < 0 || queue_id >= TM_TAKTOS_MAX_QUEUES || !g_queue_created[queue_id] || message_ptr == NULL)
        return TM_ERROR;

    return (TaktOSQueueReceive(&g_queues[queue_id], message_ptr, false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES)
        return TM_ERROR;

    if (TaktOSSemInit(&g_semaphores[semaphore_id], 1u, 1u) != TAKTOS_OK)
        return TM_ERROR;

    g_semaphore_created[semaphore_id] = true;
    return TM_SUCCESS;
}

int tm_semaphore_get(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id])
        return TM_ERROR;

    return (TaktOSSemTake(&g_semaphores[semaphore_id], false, TAKTOS_NO_WAIT) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_put(int semaphore_id)
{
    if (semaphore_id < 0 || semaphore_id >= TM_TAKTOS_MAX_SEMAPHORES || !g_semaphore_created[semaphore_id])
        return TM_ERROR;

    return (TaktOSSemGive(&g_semaphores[semaphore_id], false) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_create(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES)
        return TM_ERROR;

    if (TaktOSMutexInit(&g_mutexes[mutex_id]) != TAKTOS_OK)
        return TM_ERROR;

    g_mutex_created[mutex_id] = true;
    return TM_SUCCESS;
}

int tm_mutex_lock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES || !g_mutex_created[mutex_id])
        return TM_ERROR;

    return (TaktOSMutexLock(&g_mutexes[mutex_id], true, TAKTOS_WAIT_FOREVER) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_mutex_unlock(int mutex_id)
{
    if (mutex_id < 0 || mutex_id >= TM_TAKTOS_MAX_MUTEXES || !g_mutex_created[mutex_id])
        return TM_ERROR;

    return (TaktOSMutexUnlock(&g_mutexes[mutex_id]) == TAKTOS_OK) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
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

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
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

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    if (pool_id < 0 || pool_id >= TM_TAKTOS_MAX_POOLS || !g_pool_created[pool_id] || memory_ptr == NULL)
        return TM_ERROR;

    *((void**)memory_ptr) = g_pool_free[pool_id];
    g_pool_free[pool_id] = (void*)memory_ptr;
    return TM_SUCCESS;
}

void tm_cause_interrupt(void)
{
    // Set pending bit in NVIC for our borrowed IRQ line. STIR is not
    // implemented on SAM4L, so we use ISPR directly.
    NVIC_ISPR[TM_SAM4L_SWI_IRQ_N >> 5] = (1u << (TM_SAM4L_SWI_IRQ_N & 31u));
    __asm volatile ("dsb" ::: "memory");
}

// -----------------------------------------------------------------------------
// Interrupt vector: borrowed peripheral handler for tm_cause_interrupt().
// Weak-alias the CMSIS-standard TRNG_Handler so it slots into IOsonata's
// startup vector table without edits. If your startup uses a different name,
// add another weak alias line below or rename TM_SAM4L_SWI_IRQ_HANDLER_NAME.
// -----------------------------------------------------------------------------
void tm_irq_vector_handler(void);
void tm_irq_vector_handler(void)
{
    tm_interrupt_handler();
    tm_interrupt_preemption_handler();
}

void TRNG_Handler(void) __attribute__((weak, alias("tm_irq_vector_handler")));
