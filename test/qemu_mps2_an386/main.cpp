// QEMU mps2-an386 (Cortex-M4F) functional test for TaktOS signal / queue / mqueue changes.
#include <stdint.h>
#include <string.h>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSQueue.h"
#include "TaktOSSignal.h"
#include "posix/mqueue.h"

extern "C" uint32_t g_TickCount;

//--- UART / exit -------------------------------------------------------
#define UART0_DATA  (*(volatile uint32_t *)0x40004000u)
#define UART0_STATE (*(volatile uint32_t *)0x40004004u)
#define UART0_CTRL  (*(volatile uint32_t *)0x40004008u)
#define UART0_BDIV  (*(volatile uint32_t *)0x40004010u)

static void putc_(char c) { while (UART0_STATE & 1u) {} UART0_DATA = (uint8_t)c; }
static void puts_(const char *s) { while (*s) putc_(*s++); }
static void putu(uint32_t v) { char b[12]; int i = 0; do { b[i++] = '0' + v % 10u; v /= 10u; } while (v); while (i) putc_(b[--i]); }

static void qemu_exit(int code)
{
    uint32_t blk[2] = { 0x20026u, (uint32_t)code };
    register uint32_t r0 __asm("r0") = 0x20u;   // SYS_EXIT_EXTENDED
    register uint32_t r1 __asm("r1") = (uint32_t)blk;
    __asm volatile ("bkpt 0xAB" :: "r"(r0), "r"(r1) : "memory");
    for (;;) {}
}

static int g_fail, g_pass;
#define CHECK(c) do { if (c) { g_pass++; } else { g_fail++; puts_("FAIL line "); putu(__LINE__); puts_(": " #c "\n"); } } while (0)

//--- NVIC IRQ0 used as a software-pended "peripheral" interrupt ----------
#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ISPR0 (*(volatile uint32_t *)0xE000E200u)
#define NVIC_IPR0  (*(volatile uint8_t  *)0xE000E400u)

static void IrqPend(void) { NVIC_ISPR0 = 1u; __asm volatile ("dsb\n isb"); }

//--- Threads ----------------------------------------------------------
alignas(8) static uint8_t s_MainMem[TAKTOS_THREAD_MEM_SIZE(2048)];
alignas(8) static uint8_t s_HiMem  [TAKTOS_THREAD_MEM_SIZE(1024)];
alignas(8) static uint8_t s_ToMem  [TAKTOS_THREAD_MEM_SIZE(1024)];
alignas(8) static uint8_t s_QMem   [TAKTOS_THREAD_MEM_SIZE(1024)];
alignas(8) static uint8_t s_SMem   [TAKTOS_THREAD_MEM_SIZE(1024)];
alignas(8) static uint8_t s_WMem   [TAKTOS_THREAD_MEM_SIZE(1024)];

static hTaktOSThread_t hMain, hHi, hTo, hQ, hS, hW;

static volatile int      s_IsrMode;      // 0 = signal hHi, 1 = queue send
static volatile uint32_t s_IsrSeq;

// Hi: wake-only signal waiter, forever
static volatile uint32_t s_HiWakes, s_HiRet;

static void HiThread(void *)
{
    for (;;)
    {
        s_HiRet  = TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER);
        s_HiWakes++;
    }
}

// To: scripted phases
static volatile int      s_ToPhase;
static volatile uint32_t s_ToRet[8], s_ToDt[8];

static void ToThread(void *)
{
    uint32_t t0;
    // 1: timeout 5 ticks
    t0 = g_TickCount; s_ToRet[1] = TaktOSThreadSignalWait(true, 5u); s_ToDt[1] = g_TickCount - t0; s_ToPhase = 1;
    // 2: forever, cancelled by Resume
    s_ToRet[2] = TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER); s_ToPhase = 2;
    // 3: forever, Suspend + Signal + Resume
    s_ToRet[3] = TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER); s_ToPhase = 3;
    // 4: latched bit from phase 3 returns immediately
    s_ToRet[4] = TaktOSThreadSignalWait(false, TAKTOS_NO_WAIT); s_ToPhase = 4;
    // 5: timed wait woken early by signal
    t0 = g_TickCount; s_ToRet[5] = TaktOSThreadSignalWait(true, 100u); s_ToDt[5] = g_TickCount - t0; s_ToPhase = 5;
    // 6: empty non-blocking
    s_ToRet[6] = TaktOSThreadSignalWait(false, TAKTOS_NO_WAIT); s_ToPhase = 6;
    for (;;) { TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER); }
}

// Q: blocking consumer
static TaktOSQueue_t s_Q;  alignas(4) static uint32_t s_QBuf[4 * 4];
static volatile uint32_t s_QRecv, s_QErr, s_QLast;

static void QThread(void *)
{
    uint32_t msg[4];
    for (;;)
    {
        if (TaktOSQueueReceive(&s_Q, msg, true, TAKTOS_WAIT_FOREVER) != TAKTOS_OK) { s_QErr++; continue; }
        if (msg[0] != s_QRecv || msg[3] != ~msg[0]) s_QErr++;
        s_QLast = msg[0];
        s_QRecv++;
    }
}

// S: blocking producer into a small queue
static TaktOSQueue_t s_Q3; alignas(4) static uint32_t s_Q3Buf[2 * 4];
static volatile uint32_t s_SSent, s_SErr;

static void SThread(void *)
{
    for (uint32_t i = 0; i < 50u; i++)
    {
        uint32_t msg[4] = { i, 1, 2, ~i };
        if (TaktOSQueueSend(&s_Q3, msg, true, TAKTOS_WAIT_FOREVER) != TAKTOS_OK) s_SErr++;
        s_SSent++;
    }
    for (;;) { TaktOSThreadSuspend(TaktOSCurrentThread()); }
}

// W: semaphore timed wait whose deadline lands on tick 0
static TaktOSSem_t s_WSem;
static volatile uint32_t s_WRet, s_WDone, s_WSpurious;

static void WThread(void *)
{
    s_WRet = TaktOSSemTake(&s_WSem, true, 3u);
    s_WDone = 1;
    for (;;) { TaktOSThreadSuspend(TaktOSCurrentThread()); s_WSpurious++; }
}

extern "C" void Irq0_Handler(void)
{
    if (s_IsrMode == 0)
    {
        TaktOSThreadSignal(hHi);
    }
    else
    {
        uint32_t msg[4] = { s_IsrSeq, 0, 0, ~s_IsrSeq };
        if (TaktOSQueueSend(&s_Q, msg, false, TAKTOS_NO_WAIT) == TAKTOS_OK)
        {
            s_IsrSeq++;
        }
    }
}

static void Sleep(uint32_t t) { TaktOSThreadSleepTicks(TaktOSCurrentThread(), t); }

static void MainThread(void *)
{
    puts_("TaktOS QEMU test start\n");

    //--- 1. ISR -> thread signal wake, synchronous preemption --------------
    s_IsrMode = 0;
    hHi = TaktOSThreadCreate(s_HiMem, sizeof(s_HiMem), HiThread, nullptr, 20);
    Sleep(1);                                    // let Hi block
    {
        int bad = 0;
        for (uint32_t i = 0; i < 1000u; i++)
        {
            IrqPend();
            if (s_HiWakes != i + 1u || s_HiRet != TAKTOS_OK) bad++;
        }
        CHECK(bad == 0);
    }
    //--- 2. posts before waiting coalesce into one pending notification -----
    CHECK(TaktOSThreadSignal(hMain) == TAKTOS_OK);
    CHECK(TaktOSThreadSignal(hMain) == TAKTOS_OK);
    CHECK(hMain->SigPending == 1u);
    CHECK(TaktOSThreadSignalWait(false, TAKTOS_NO_WAIT) == TAKTOS_OK);
    CHECK(TaktOSThreadSignalWait(false, TAKTOS_NO_WAIT) == TAKTOS_ERR_EMPTY);

    //--- 3. second post after direct delivery survives until the next wait --
    {
        uint32_t w = s_HiWakes;
        uint32_t state = TaktOSEnterCritical();
        CHECK(TaktOSThreadSignal(hHi) == TAKTOS_OK); // direct delivery
        CHECK(TaktOSThreadSignal(hHi) == TAKTOS_OK); // another wake pending
        CHECK(hHi->SigPending == 1u);
        TaktOSExitCritical(state);
        CHECK(s_HiWakes == w + 2u);
        CHECK(hHi->SigPending == 0u);
    }
    CHECK(TaktOSThreadSignal(nullptr) == TAKTOS_ERR_INVALID);

    //--- 4..9. scripted waiter --------------------------------------------
    hTo = TaktOSThreadCreate(s_ToMem, sizeof(s_ToMem), ToThread, nullptr, 15);
    Sleep(10);
    CHECK(s_ToPhase == 1);
    CHECK(s_ToRet[1] == TAKTOS_ERR_TIMEOUT);
    CHECK(s_ToDt[1] >= 5u && s_ToDt[1] <= 6u);

    Sleep(1);                                    // phase 2 blocked forever
    CHECK(s_ToPhase == 1);
    TaktOSThreadResume(hTo);
    CHECK(s_ToPhase == 2);
    CHECK(s_ToRet[2] == TAKTOS_ERR_INTERRUPTED);

    Sleep(1);                                    // phase 3 blocked forever
    TaktOSThreadSuspend(hTo);
    TaktOSThreadSignal(hTo);                 // must not wake a suspended thread
    Sleep(2);
    CHECK(s_ToPhase == 2);
    CHECK(hTo->SigPending == 1u);
    TaktOSThreadResume(hTo);                     // phase 3 -> INTERRUPTED, phase 4 -> pending wake OK
    Sleep(1);
    CHECK(s_ToRet[3] == TAKTOS_ERR_INTERRUPTED);
    CHECK(s_ToRet[4] == TAKTOS_OK);

    // phase 5: timed wait, early signal from ISR context path (thread here)
    Sleep(2);
    CHECK(s_ToPhase == 4);
    TaktOSThreadSignal(hTo);
    CHECK(s_ToPhase >= 5);
    CHECK(s_ToRet[5] == TAKTOS_OK && s_ToDt[5] <= 3u);
    CHECK(s_ToPhase == 6 && s_ToRet[6] == TAKTOS_ERR_EMPTY);
    // sleep list still sane after early removal of a timed waiter
    {
        uint32_t t0 = g_TickCount;
        Sleep(7);
        uint32_t dt = g_TickCount - t0;
        CHECK(dt >= 7u && dt <= 8u);
    }

    //--- 10. queue: blocked consumer, thread sends ------------------------
    CHECK(TaktOSQueueInit(&s_Q, s_QBuf, 16u, 4u) == TAKTOS_OK);
    hQ = TaktOSThreadCreate(s_QMem, sizeof(s_QMem), QThread, nullptr, 18);
    Sleep(1);
    for (uint32_t i = 0; i < 500u; i++)
    {
        uint32_t msg[4] = { i, 0, 0, ~i };
        TaktOSQueueSend(&s_Q, msg, true, TAKTOS_WAIT_FOREVER);
        if (s_QRecv != i + 1u) { s_QErr++; }       // consumer preempted and took it
    }
    CHECK(s_QRecv == 500u && s_QErr == 0u);
    //--- 11. queue: ISR sends to blocked consumer ------------------------
    s_IsrMode = 1; s_IsrSeq = 500u;
    for (uint32_t i = 0; i < 500u; i++) { IrqPend(); }
    CHECK(s_QRecv == 1000u && s_QErr == 0u);
    s_IsrMode = 0;

    //--- 12. queue: blocked sender, thread receives ----------------------
    CHECK(TaktOSQueueInit(&s_Q3, s_Q3Buf, 16u, 2u) == TAKTOS_OK);
    hS = TaktOSThreadCreate(s_SMem, sizeof(s_SMem), SThread, nullptr, 18);
    {
        int bad = 0;
        for (uint32_t i = 0; i < 50u; i++)
        {
            uint32_t msg[4];
            if (TaktOSQueueReceive(&s_Q3, msg, true, 10u) != TAKTOS_OK || msg[0] != i || msg[3] != ~i) bad++;
        }
        CHECK(bad == 0 && s_SSent == 50u && s_SErr == 0u);
        uint32_t msg[4];
        uint32_t t0 = g_TickCount;
        CHECK(TaktOSQueueReceive(&s_Q3, msg, true, 4u) == TAKTOS_ERR_TIMEOUT);
        uint32_t dt = g_TickCount - t0;
        CHECK(dt >= 4u && dt <= 5u);
        // full queue, timed send times out
        msg[0] = 1; msg[3] = 2;
        CHECK(TaktOSQueueSend(&s_Q3, msg, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
        CHECK(TaktOSQueueSend(&s_Q3, msg, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
        CHECK(TaktOSQueueSend(&s_Q3, msg, false, TAKTOS_NO_WAIT) == TAKTOS_ERR_FULL);
        t0 = g_TickCount;
        CHECK(TaktOSQueueSend(&s_Q3, msg, true, 3u) == TAKTOS_ERR_TIMEOUT);
        dt = g_TickCount - t0;
        CHECK(dt >= 3u && dt <= 4u);
    }

    //--- 13. timed wait whose deadline lands on tick 0 -------------------
    TaktOSSemInit(&s_WSem, 0u, 1u);
    {
        uint32_t st = TaktOSEnterCritical();
        g_TickCount = 0xFFFFFFFDu;                // now + 3 == 0 (wraps)
        TaktOSExitCritical(st);
    }
    hW = TaktOSThreadCreate(s_WMem, sizeof(s_WMem), WThread, nullptr, 19);
    TaktOSCtxSwitch();                            // run hW now; it blocks on the sem
    CHECK(hW->WakeTick != 0u);                    // deadline never encoded as 0
    TaktOSSemGive(&s_WSem, false);                // early wake must unlink from sleep list
    CHECK(s_WDone == 1u && s_WRet == TAKTOS_OK);
    {
        uint32_t t0 = g_TickCount;
        Sleep(6);                                 // crosses the wrap
        uint32_t dt = g_TickCount - t0;
        CHECK(dt >= 6u && dt <= 7u);
        CHECK(s_WSpurious == 0u);                 // suspended thread never woken by a stale sleep entry
    }
    // signal path still fine after wrap
    {
        uint32_t w = s_HiWakes;
        IrqPend();
        CHECK(s_HiWakes == w + 1u);
    }

    //--- 14. POSIX mqueue ordering --------------------------------------
    {
        struct mq_attr a; memset(&a, 0, sizeof(a));
        a.mq_maxmsg = 8; a.mq_msgsize = 8;
        mqd_t q = mq_open("/t", O_CREAT | O_RDWR | O_NONBLOCK, 0, &a);
        CHECK(q != MQ_FAILED);
        const unsigned pr[8] = { 1, 5, 1, 9, 5, 0, 9, 1 };
        for (uint32_t i = 0; i < 8u; i++)
        {
            char m[8] = { (char)('A' + i) };
            CHECK(mq_send(q, m, 8, pr[i]) == 0);
        }
        char m[8] = { 0 };
        CHECK(mq_send(q, m, 8, 3) == -1);         // full, non-blocking
        mq_getattr(q, &a);
        CHECK(a.mq_curmsgs == 8);
        const char exp[] = "DGBEACHF";            // 9:D,G 5:B,E 1:A,C,H 0:F
        char got[9] = { 0 };
        for (uint32_t i = 0; i < 8u; i++)
        {
            unsigned p = 0;
            CHECK(mq_receive(q, m, 8, &p) == 8);
            got[i] = m[0];
        }
        CHECK(memcmp(got, exp, 8) == 0);
        if (memcmp(got, exp, 8) != 0) { puts_("mq order: "); puts_(got); puts_("\n"); }
        CHECK(mq_receive(q, m, 8, nullptr) == -1); // empty, non-blocking
        // refill/drain cycles exercise the free list
        int bad = 0;
        for (uint32_t r = 0; r < 100u; r++)
        {
            for (uint32_t i = 0; i < 8u; i++) { m[0] = (char)i; if (mq_send(q, m, 8, i & 1u) != 0) bad++; }
            for (uint32_t i = 0; i < 8u; i++) { unsigned p; if (mq_receive(q, m, 8, &p) != 8 || p != (i < 4u ? 1u : 0u)) bad++; }
        }
        CHECK(bad == 0);
    }

    puts_("pass "); putu(g_pass); puts_(" fail "); putu(g_fail); puts_("\n");
    puts_(g_fail ? "TEST FAILED\n" : "ALL TESTS PASSED\n");
    qemu_exit(g_fail ? 1 : 0);
}

int main(void)
{
    UART0_BDIV = 16u; UART0_CTRL = 1u;
    NVIC_IPR0  = 0x40u;                           // above SysTick / PendSV (0xFF)
    NVIC_ISER0 = 1u;

    TaktOSCfg_t cfg = {};
    cfg.KernClockHz = 25000000u;
    cfg.TickHz = 1000u;
    TaktOSInit(&cfg);
    hMain = TaktOSThreadCreate(s_MainMem, sizeof(s_MainMem), MainThread, nullptr, 10);
    TaktOSStart();
    return 0;
}
