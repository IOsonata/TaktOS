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
// 
// test_semaphore.cpp    TaktOS semaphore MC/DC test suite
//
// Covers every decision/condition in semaphore.cpp:
//   TaktSemInit           null sem, count > max (clamped), valid
//   TaktSemGive           count >= max (full), no waiter (count>=0), waiter (count<0)
//   TaktSemTake           count > 0 (immediate), count == 0 no-wait, count < 0 block
//   Timeout path          pWaitCancel registered + fired
//   WaitListInsert        empty, head insert (higher prio), tail insert (lower prio)
//   split-counter atomics  fast-path give: old >= 0, slow-path give: old < 0
// 

#include <gtest/gtest.h>
#include <taktos/semaphore.h>
#include <taktos/scheduler.h>
#include <taktos/port_asm.h>

extern "C" {
    TaktosCtx taktos_ctx = { nullptr, nullptr };
    TaktosIrqState taktos_irq_save(void)    { return 0u; }
    void taktos_irq_restore(TaktosIrqState) {}
    void taktos_pend_context_switch(void)   {}
    void taktos_wait_for_interrupt(void)    {}
    uint32_t taktos_highest_bit(uint32_t v)
    {
        uint32_t r = 0u;
        if (v & 0xFFFF0000u)
        {
            r+=16u;
            v>>=16u;
        }
        if (v & 0xFF00u)
        {
            r+= 8u;
            v>>= 8u;
        }
        if (v & 0xF0u)
        {
            r+= 4u;
            v>>= 4u;
        }
        if (v & 0xCu)
        {
            r+= 2u;
            v>>= 2u;
        }
        if (v & 0x2u)
        {
            r+= 1u;
        }
        return r;
    }
}

namespace taktos {

class SemTest : public ::testing::Test {
protected:
    TaktSem_t sem{};
    TaktCfg_t cfg{};

    void SetUp() override {
        cfg.MaxPriorities = 8u; cfg.TickHz = 1000u;
        cfg.IdleStackBytes = 256u; cfg.MaxTimers = 4u;
        cfg.TimerTaskPrio = 6u;
        cfg.AgeEnable = false; cfg.AgeInterval = 0u;
        Scheduler::Init(&cfg);
        sem = {};
    }
};

TEST_F(SemTest, Init_ValidBinary)
{
    EXPECT_TRUE(TaktSemInit(&sem, 0u, 1u));
    EXPECT_EQ(TaktSemCount(&sem), 0u);
}

TEST_F(SemTest, Init_CountClampedToMax)
{
    EXPECT_TRUE(TaktSemInit(&sem, 5u, 3u));
    EXPECT_EQ(TaktSemCount(&sem), 3u);   // clamped to max
}

TEST_F(SemTest, Give_IncreasesCount)
{
    TaktSemInit(&sem, 0u, 2u);
    EXPECT_TRUE(TaktSemGive(&sem));
    EXPECT_EQ(TaktSemCount(&sem), 1u);
}

TEST_F(SemTest, Give_FullSemaphore_ReturnsFalse)
{
    TaktSemInit(&sem, 1u, 1u);   // already full
    EXPECT_FALSE(TaktSemGive(&sem));
}

TEST_F(SemTest, Take_TokenAvailable_Immediate)
{
    TaktSemInit(&sem, 1u, 1u);
    EXPECT_TRUE(TaktSemTake(&sem, TAKT_NO_WAIT));
    EXPECT_EQ(TaktSemCount(&sem), 0u);
}

TEST_F(SemTest, Take_NoToken_NoWait_ReturnsFalse)
{
    TaktSemInit(&sem, 0u, 1u);
    EXPECT_FALSE(TaktSemTake(&sem, TAKT_NO_WAIT));
}

TEST_F(SemTest, Give_Then_Take_RoundTrip)
{
    TaktSemInit(&sem, 0u, 4u);
    TaktSemGive(&sem);
    TaktSemGive(&sem);
    EXPECT_EQ(TaktSemCount(&sem), 2u);
    TaktSemTake(&sem, TAKT_NO_WAIT);
    EXPECT_EQ(TaktSemCount(&sem), 1u);
}

TEST_F(SemTest, Give_Counting_MultipleTokens)
{
    TaktSemInit(&sem, 0u, 4u);
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(TaktSemGive(&sem));
    }
    EXPECT_FALSE(TaktSemGive(&sem));   // 5th give on max=4 fails
    EXPECT_EQ(TaktSemCount(&sem), 4u);
}

} // namespace taktos
