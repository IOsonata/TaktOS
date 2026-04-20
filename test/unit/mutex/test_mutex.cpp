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
#include <taktos/mutex.h>
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
class MutexTest : public ::testing::Test {
protected:
    TaktMutex_t mtx{}; TaktCfg_t cfg{};
    void SetUp() override {
        cfg.MaxPriorities=8u; cfg.TickHz=1000u; cfg.IdleStackBytes=256u;
        cfg.MaxTimers=4u; cfg.TimerTaskPrio=6u; cfg.AgeEnable=false; cfg.AgeInterval=0u;
        Scheduler::Init(&cfg); TaktMutexInit(&mtx);
    }
};
TEST_F(MutexTest, Init_Unlocked)
{
    EXPECT_FALSE(TaktMutexIsLocked(&mtx));
}
TEST_F(MutexTest, Acquire_Free)
{
    Tcb o{};
    o.Priority = 3u;
    o.BasePriority = 3u;
    taktos_ctx.current = &o;
    EXPECT_TRUE(TaktMutexAcquire(&mtx, TAKT_NO_WAIT));
    EXPECT_TRUE(TaktMutexIsLocked(&mtx));
}
TEST_F(MutexTest, Release_Unlocks)
{
    Tcb o{};
    o.Priority = 3u;
    o.BasePriority = 3u;
    taktos_ctx.current = &o;
    TaktMutexAcquire(&mtx, TAKT_NO_WAIT);
    TaktMutexRelease(&mtx);
    EXPECT_FALSE(TaktMutexIsLocked(&mtx));
}
TEST_F(MutexTest, Acquire_Locked_NoWait_False)
{
    Tcb o{}, x{};
    o.Priority = 3u; o.BasePriority = 3u;
    x.Priority = 4u; x.BasePriority = 4u;
    taktos_ctx.current = &o;
    TaktMutexAcquire(&mtx, TAKT_NO_WAIT);
    taktos_ctx.current = &x;
    EXPECT_FALSE(TaktMutexAcquire(&mtx, TAKT_NO_WAIT));
}
TEST_F(MutexTest, IsLocked_RoundTrip)
{
    Tcb o{};
    o.Priority = 2u; o.BasePriority = 2u;
    taktos_ctx.current = &o;
    EXPECT_FALSE(TaktMutexIsLocked(&mtx));
    TaktMutexAcquire(&mtx, TAKT_NO_WAIT);
    EXPECT_TRUE(TaktMutexIsLocked(&mtx));
    TaktMutexRelease(&mtx);
    EXPECT_FALSE(TaktMutexIsLocked(&mtx));
}
} // namespace taktos
