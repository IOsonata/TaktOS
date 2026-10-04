#include <stdint.h>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSQueue.h"
#ifdef BENCH_MQ
#include "posix/mqueue.h"
#include <string.h>
#endif
#ifdef BENCH_SIG
#include "TaktOSSignal.h"
#endif
#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ISPR0 (*(volatile uint32_t *)0xE000E200u)
#define NVIC_IPR0  (*(volatile uint8_t  *)0xE000E400u)
extern "C" __attribute__((noinline)) void mark_start(void) { __asm volatile (""); }
extern "C" __attribute__((noinline)) void mark_woke(void)  { __asm volatile (""); }
extern "C" __attribute__((noinline)) void mark_end(void)   { __asm volatile (""); }
extern "C" __attribute__((noinline)) void bench_done(void) { for (;;) {} }
alignas(8) static uint8_t s_MainMem[TAKTOS_THREAD_MEM_SIZE(1024)];
alignas(8) static uint8_t s_HiMem[TAKTOS_THREAD_MEM_SIZE(1024)];
static hTaktOSThread_t hHi;
static TaktOSSem_t s_Sem;
static TaktOSQueue_t s_Q; alignas(4) static uint32_t s_QBuf[16];
static void HiThread(void *)
{
    for (;;)
    {
#if defined(BENCH_QNB) || defined(BENCH_MQ)
        TaktOSThreadSuspend(TaktOSCurrentThread());
#elif defined(BENCH_SIG)
        TaktOSThreadSignalWait(true, TAKTOS_WAIT_FOREVER);
#elif defined(BENCH_SEM)
        TaktOSSemTake(&s_Sem, true, TAKTOS_WAIT_FOREVER);
#else
        uint32_t m[4]; TaktOSQueueReceive(&s_Q, m, true, TAKTOS_WAIT_FOREVER);
#endif
        mark_woke();
    }
}
extern "C" void Irq0_Handler(void)
{
#if defined(BENCH_SIG)
    TaktOSThreadSignal(hHi);
#elif defined(BENCH_SEM)
    TaktOSSemGive(&s_Sem, false);
#else
    static uint32_t m[4] = { 1, 2, 3, 4 };
    TaktOSQueueSend(&s_Q, m, false, TAKTOS_NO_WAIT);
#endif
}
static void MainThread(void *)
{
#if defined(BENCH_QNB)
    *(volatile uint32_t *)0xE000E010u = 0u;
    static uint32_t m4[4] = { 1, 2, 3, 4 };
    for (int i = 0; i < 4; i++)
    {
        mark_start();
        TaktOSQueueSend(&s_Q, m4, false, TAKTOS_NO_WAIT);
        TaktOSQueueReceive(&s_Q, m4, false, TAKTOS_NO_WAIT);
        mark_end();
    }
    bench_done();
#endif
#if defined(BENCH_MQ)
    *(volatile uint32_t *)0xE000E010u = 0u;
    struct mq_attr a; memset(&a, 0, sizeof(a)); a.mq_maxmsg = 16; a.mq_msgsize = 16;
    mqd_t q = mq_open("/b", O_CREAT | O_RDWR | O_NONBLOCK, 0, &a);
    static char m[16];
    for (int i = 0; i < 8; i++) mq_send(q, m, 16, 0);
    for (int i = 0; i < 4; i++)
    {
        mark_start();
        mq_send(q, m, 16, 0);
        mq_receive(q, m, 16, nullptr);
        mark_end();
    }
    bench_done();
#endif
    *(volatile uint32_t *)0xE000E010u = 0u;      // stop SysTick: no tick noise in counts
    for (int i = 0; i < 4; i++)
    {
        mark_start();
        NVIC_ISPR0 = 1u; __asm volatile ("dsb\n isb");
        mark_end();
    }
    bench_done();
}
int main(void)
{
    NVIC_IPR0 = 0x40u; NVIC_ISER0 = 1u;
    TaktOSSemInit(&s_Sem, 0u, 1u);
    TaktOSQueueInit(&s_Q, s_QBuf, 16u, 4u);
    TaktOSCfg_t cfg = {}; cfg.KernClockHz = 25000000u; TaktOSInit(&cfg);
    TaktOSThreadCreate(s_MainMem, sizeof(s_MainMem), MainThread, nullptr, 10);
    hHi = TaktOSThreadCreate(s_HiMem, sizeof(s_HiMem), HiThread, nullptr, 20);
    TaktOSStart();
}
