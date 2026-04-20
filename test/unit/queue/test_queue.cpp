/*===========================================================================
 * OBSOLETE  DO NOT BUILD
 *
 * This file was written against a prior TaktOS API family and does not
 * compile against the current kernel (renamed types: Tcb  TaktOSThread_t,
 * taktos_ctx  g_TaktosCtx; renamed functions: TaktMutexAcquire 
 * TaktOSMutexLock, TaktMutexRelease  TaktOSMutexUnlock, etc.).
 *
 * The current regression suite is test/test_kernel.cpp.
 * Do not include this file in any build or CI sweep.
 *===========================================================================*/
#include <gtest/gtest.h>
#include <taktos/queue.h>
#include <taktos/scheduler.h>
#include <taktos/port_asm.h>
#include <cstring>

extern "C" {
    TaktosCtx taktos_ctx = { nullptr, nullptr };
    TaktosIrqState taktos_irq_save(void)    { return 0u; }
    void taktos_irq_restore(TaktosIrqState) {}
    void taktos_pend_context_switch(void)   {}
    void taktos_wait_for_interrupt(void)    {}
    uint32_t taktos_highest_bit(uint32_t v)
    {
        uint32_t r=0u;
        if(v&0xFFFF0000u)
        {
            r+=16u;
            v>>=16u;
        }
        if(v&0xFF00u)
        {
            r+=8u;
            v>>=8u;
        }
        if(v&0xF0u)
        {
            r+=4u;
            v>>=4u;
        }
        if(v&0xCu)
        {
            r+=2u;
            v>>=2u;
        }
        if(v&0x2u)
        {
            r+=1u;
        }
        return r;
    }
}
namespace taktos {
class QueueTest : public ::testing::Test {
protected:
    TaktQueue_t q{}; TaktCfg_t cfg{};
    uint8_t buf[CFIFO_TOTAL_MEMSIZE(4, sizeof(uint32_t))]{};
    void SetUp() override {
        cfg.MaxPriorities=8u; cfg.TickHz=1000u; cfg.IdleStackBytes=256u;
        cfg.MaxTimers=4u; cfg.TimerTaskPrio=6u; cfg.AgeEnable=false; cfg.AgeInterval=0u;
        Scheduler::Init(&cfg);
        std::memset(buf,0,sizeof(buf));
        TaktQueueInit(&q, buf, sizeof(uint32_t), 4u);
    }
};
TEST_F(QueueTest, Init_Empty)
{
    EXPECT_EQ(TaktQueueCount(&q), 0u);
    EXPECT_EQ(TaktQueueFree(&q), 4u);
}
TEST_F(QueueTest, Send_Enqueues)
{
    uint32_t v = 42u;
    EXPECT_TRUE(TaktQueueSend(&q, &v, TAKT_NO_WAIT));
    EXPECT_EQ(TaktQueueCount(&q), 1u);
}
TEST_F(QueueTest, Receive_Dequeues)
{
    uint32_t v = 99u;
    uint32_t out = 0u;
    TaktQueueSend(&q, &v, TAKT_NO_WAIT);
    EXPECT_TRUE(TaktQueueReceive(&q, &out, TAKT_NO_WAIT));
    EXPECT_EQ(out, 99u);
}
TEST_F(QueueTest, Send_Full_NoWait_False)
{
    uint32_t v = 1u;
    for (int i = 0; i < 4; i++)
    {
        TaktQueueSend(&q, &v, TAKT_NO_WAIT);
    }
    EXPECT_FALSE(TaktQueueSend(&q, &v, TAKT_NO_WAIT));
}
TEST_F(QueueTest, Receive_Empty_NoWait_False)
{
    uint32_t out = 0u;
    EXPECT_FALSE(TaktQueueReceive(&q, &out, TAKT_NO_WAIT));
}
TEST_F(QueueTest, FIFO_Order)
{
    for (uint32_t i = 1u; i <= 4u; i++)
    {
        TaktQueueSend(&q, &i, TAKT_NO_WAIT);
    }
    for (uint32_t i = 1u; i <= 4u; i++)
    {
        uint32_t out = 0u;
        TaktQueueReceive(&q, &out, TAKT_NO_WAIT);
        EXPECT_EQ(out, i);
    }
}
} // namespace taktos
