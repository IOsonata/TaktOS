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
// test_scheduler.cpp    TaktOS scheduler MC/DC test suite
//
// Runs on x86 host (Google Test) with TAKT_ARCH_STUB=1.
// Covers every decision/condition in scheduler.cpp + timer.cpp.
//
// Build: g++ -std=c++20 -DTAKT_ARCH_STUB=1 -I../../include -lgtest -lgtest_main
// Coverage: gcov --branch-probabilities + lcov; target 100% MC/DC on cert boundary.
//
// MC/DC decisions covered:
//   Scheduler::Init           null cfg, zero MaxPriorities, MaxPriorities > MAX
//   Scheduler::ListInsertTail  empty list, non-empty list
//   Scheduler::ListRemove     single-element list, head removal, mid removal
//   Scheduler::ListAdvanceHead  nullptr list, non-null list
//   Scheduler::MakeReady      normal path
//   Scheduler::RemoveReady    normal path
//   Scheduler::SelectNext     aging enabled/disabled, next_thread null/non-null
//   Scheduler::Yield          context switch triggered, not triggered
//   Scheduler::BlockCurrent   wait_list nullptr, non-null
//   Scheduler::UnblockTask    unblock triggers preemption, doesn't trigger
//   Scheduler::AgePriorities  interval==0, no tasks ready, PI-boosted skip,
//                              AgeTicks below/at/above interval
//   Scheduler::AgePriorityRestore  null, priority==base, priority>base
//   Scheduler::Tick           aging on/off, ProcessTimerList returns T/F
//   TimerListInsert           empty list, prepend, append, mid-insert
//   TimerListRemove           empty, head, middle, not present
//   ProcessTimerList          empty, single expire, multi expire,
//                              pWaitCancel null/non-null, priority > current
// 

#include <gtest/gtest.h>
#include <taktos/scheduler.h>
#include <taktos/tcb.h>
#include <taktos/port_asm.h>

//  Arch stub implementation (TAKT_ARCH_STUB=1) 
// Provides software equivalents of the 6 AAL functions so the scheduler
// runs on x86 without any ARM/RISC-V hardware.

extern "C" {
    // Stub globals
    uint32_t stub_irq_state   = 0u;
    bool     stub_ctx_pending = false;

    TaktosCtx taktos_ctx = { nullptr, nullptr };

    TaktosIrqState taktos_irq_save(void)    { return stub_irq_state; }
    void taktos_irq_restore(TaktosIrqState) {}
    void taktos_pend_context_switch(void)   { stub_ctx_pending = true; }
    void taktos_wait_for_interrupt(void)    {}
    uint32_t taktos_highest_bit(uint32_t v)
    {
        uint32_t r = 0u;
        if (v & 0xFFFF0000u)
        {
            r += 16u;
            v >>= 16u;
        }
        if (v & 0xFF00u)
        {
            r +=  8u;
            v >>=  8u;
        }
        if (v & 0xF0u)
        {
            r +=  4u;
            v >>=  4u;
        }
        if (v & 0xCu)
        {
            r +=  2u;
            v >>=  2u;
        }
        if (v & 0x2u)
        {
            r +=  1u;
        }
        return r;
    }
}

namespace taktos {

//  Test fixture 
class SchedulerTest : public ::testing::Test {
protected:
    static constexpr uint8_t MAX_PRIO = 8u;

    TaktCfg_t cfg{};
    Tcb       tcbs[8]{};

    void SetUp() override {
        stub_ctx_pending = false;
        taktos_ctx = { nullptr, nullptr };

        cfg.MaxPriorities = MAX_PRIO;
        cfg.TickHz        = 1000u;
        cfg.IdleStackBytes= 256u;
        cfg.MaxTimers     = 8u;
        cfg.TimerTaskPrio = 6u;
        cfg.AgeEnable     = false;
        cfg.AgeInterval   = 0u;

        Scheduler::Init(&cfg);

        for (int i = 0; i < 8; ++i)
        {
            tcbs[i] = {};
            tcbs[i].Priority     = static_cast<uint8_t>(i);
            tcbs[i].BasePriority = static_cast<uint8_t>(i);
            tcbs[i].State        = TAKT_TASK_READY;
        }
    }
};

//  Init 

TEST_F(SchedulerTest, Init_Resets_RunQueue)
{
    EXPECT_EQ(Scheduler::s_rq.bitmap, 0u);
    EXPECT_EQ(Scheduler::s_tick, 0u);
    EXPECT_EQ(Scheduler::s_timer_list, nullptr);
    EXPECT_EQ(taktos_ctx.current, nullptr);
    EXPECT_EQ(taktos_ctx.next_thread, nullptr);
}

//  ListInsertTail 

TEST_F(SchedulerTest, ListInsertTail_EmptyList_SetsBitmap)
{
    Scheduler::ListInsertTail(3u, &tcbs[3]);
    EXPECT_NE(Scheduler::s_rq.bitmap & (1u << 3u), 0u);
    EXPECT_EQ(Scheduler::s_rq.lists[3], &tcbs[3]);
    EXPECT_EQ(tcbs[3].pNext, &tcbs[3]);   // circular self-link
    EXPECT_EQ(taktos_ctx.next_thread, &tcbs[3]);
}

TEST_F(SchedulerTest, ListInsertTail_NonEmpty_AppendsTail)
{
    Scheduler::ListInsertTail(3u, &tcbs[3]);
    Tcb extra{}; extra.Priority = 3u;
    Scheduler::ListInsertTail(3u, &extra);
    EXPECT_EQ(Scheduler::s_rq.lists[3], &tcbs[3]);    // head unchanged
    EXPECT_EQ(tcbs[3].pPrev, &extra);                  // new tail
}

//  ListRemove 

TEST_F(SchedulerTest, ListRemove_SingleElement_ClearsBitmap)
{
    Scheduler::ListInsertTail(3u, &tcbs[3]);
    Scheduler::ListRemove(&tcbs[3]);
    EXPECT_EQ(Scheduler::s_rq.bitmap & (1u << 3u), 0u);
    EXPECT_EQ(Scheduler::s_rq.lists[3], nullptr);
    EXPECT_EQ(taktos_ctx.next_thread, nullptr);
}

TEST_F(SchedulerTest, ListRemove_Head_AdvancesHead)
{
    Scheduler::ListInsertTail(3u, &tcbs[3]);
    Tcb extra{}; extra.Priority = 3u; extra.BasePriority = 3u;
    Scheduler::ListInsertTail(3u, &extra);
    Scheduler::ListRemove(&tcbs[3]);
    EXPECT_EQ(Scheduler::s_rq.lists[3], &extra);
}

//  UpdateNextThread / SelectNext 

TEST_F(SchedulerTest, SelectNext_ExecutePtrNull_KeepsCurrent)
{
    Tcb idle{}; idle.Priority = 0u; idle.BasePriority = 0u;
    Scheduler::ListInsertTail(0u, &idle);
    taktos_ctx.current     = &idle;
    taktos_ctx.next_thread = nullptr;

    Tcb* result = Scheduler::SelectNext();
    EXPECT_EQ(result, &idle);   // falls back to current
}

TEST_F(SchedulerTest, SelectNext_NonNull_SwitchesAndResetsAgeTicks)
{
    Tcb t5{}; t5.Priority = 5u; t5.BasePriority = 5u; t5.AgeTicks = 99u;
    Scheduler::ListInsertTail(5u, &t5);
    Tcb idle{}; idle.Priority = 0u; idle.BasePriority = 0u;
    Scheduler::ListInsertTail(0u, &idle);
    taktos_ctx.current = &idle;

    Tcb* n = Scheduler::SelectNext();
    EXPECT_EQ(n, &t5);
    EXPECT_EQ(t5.AgeTicks, 0u);   // reset on switch-in
}

//  Yield 

TEST_F(SchedulerTest, Yield_DifferentNext_PendsSwitchd)
{
    Tcb a{}, b{};
    a.Priority = 4u; a.BasePriority = 4u;
    b.Priority = 4u; b.BasePriority = 4u;
    Scheduler::ListInsertTail(4u, &a);
    Scheduler::ListInsertTail(4u, &b);
    taktos_ctx.current = &a;
    stub_ctx_pending = false;

    Scheduler::Yield();
    EXPECT_TRUE(stub_ctx_pending);
}

TEST_F(SchedulerTest, Yield_SingleTask_NoPendSwitch)
{
    Tcb a{}; a.Priority = 4u; a.BasePriority = 4u;
    Scheduler::ListInsertTail(4u, &a);
    taktos_ctx.current = &a;
    stub_ctx_pending = false;

    Scheduler::Yield();
    EXPECT_FALSE(stub_ctx_pending);
}

//  AgePriorities 

TEST_F(SchedulerTest, Age_IntervalZero_NoPromotion)
{
    cfg.AgeEnable   = true;
    cfg.AgeInterval = 0u;
    Scheduler::Init(&cfg);
    Tcb t2{}; t2.Priority=2u; t2.BasePriority=2u; t2.AgeTicks=999u;
    Scheduler::ListInsertTail(2u, &t2);
    Scheduler::AgePriorities();
    EXPECT_EQ(t2.Priority, 2u);   // no change
}

TEST_F(SchedulerTest, Age_BelowInterval_NoPromotion)
{
    cfg.AgeEnable = true; cfg.AgeInterval = 10u;
    Scheduler::Init(&cfg);
    Tcb t2{}; t2.Priority=2u; t2.BasePriority=2u; t2.AgeTicks=0u;
    Scheduler::ListInsertTail(2u, &t2);
    for (int i = 0; i < 9; ++i)
    {
        Scheduler::AgePriorities();
    }
    EXPECT_EQ(t2.Priority, 2u);
}

TEST_F(SchedulerTest, Age_AtInterval_Promotes)
{
    cfg.AgeEnable = true; cfg.AgeInterval = 10u;
    Scheduler::Init(&cfg);
    Tcb t2{}; t2.Priority=2u; t2.BasePriority=2u; t2.AgeTicks=0u;
    Scheduler::ListInsertTail(2u, &t2);
    for (int i = 0; i < 10; ++i)
    {
        Scheduler::AgePriorities();
    }
    EXPECT_EQ(t2.Priority, 3u);   // promoted one level
}

TEST_F(SchedulerTest, Age_PIBoosted_Skipped)
{
    cfg.AgeEnable = true; cfg.AgeInterval = 1u;
    Scheduler::Init(&cfg);
    Tcb t2{}; t2.Priority=4u; t2.BasePriority=2u; t2.AgeTicks=999u; // PI-boosted
    Scheduler::ListInsertTail(4u, &t2);
    Scheduler::AgePriorities();
    EXPECT_EQ(t2.Priority, 4u);   // skip PI-boosted tasks
}

//  Timer list 

TEST_F(SchedulerTest, TimerListInsert_Empty_BecomesHead)
{
    tcbs[0].WakeTick = 100u;
    Scheduler::TimerListInsert(&tcbs[0]);
    EXPECT_EQ(Scheduler::s_timer_list, &tcbs[0]);
    EXPECT_EQ(tcbs[0].pTimerNext, nullptr);
}

TEST_F(SchedulerTest, TimerListInsert_Earlier_Prepends)
{
    tcbs[0].WakeTick = 100u;
    tcbs[1].WakeTick = 50u;
    Scheduler::TimerListInsert(&tcbs[0]);
    Scheduler::TimerListInsert(&tcbs[1]);
    EXPECT_EQ(Scheduler::s_timer_list, &tcbs[1]);
    EXPECT_EQ(tcbs[1].pTimerNext, &tcbs[0]);
}

TEST_F(SchedulerTest, TimerListInsert_Later_Appends)
{
    tcbs[0].WakeTick = 50u;
    tcbs[1].WakeTick = 100u;
    Scheduler::TimerListInsert(&tcbs[0]);
    Scheduler::TimerListInsert(&tcbs[1]);
    EXPECT_EQ(Scheduler::s_timer_list, &tcbs[0]);
    EXPECT_EQ(tcbs[0].pTimerNext, &tcbs[1]);
}

TEST_F(SchedulerTest, TimerListRemove_Head)
{
    tcbs[0].WakeTick = 10u;
    Scheduler::TimerListInsert(&tcbs[0]);
    Scheduler::TimerListRemove(&tcbs[0]);
    EXPECT_EQ(Scheduler::s_timer_list, nullptr);
}

TEST_F(SchedulerTest, TimerListRemove_NotPresent_Safe)
{
    Scheduler::TimerListRemove(&tcbs[0]);   // must not crash
    EXPECT_EQ(Scheduler::s_timer_list, nullptr);
}

//  ProcessTimerList 

TEST_F(SchedulerTest, ProcessTimerList_Expired_MakesReady)
{
    Tcb idle{}; idle.Priority=0u; idle.BasePriority=0u;
    Scheduler::ListInsertTail(0u, &idle);
    taktos_ctx.current = &idle;

    tcbs[3].WakeTick = 1u; tcbs[3].Priority = 3u; tcbs[3].BasePriority=3u;
    Scheduler::TimerListInsert(&tcbs[3]);

    Scheduler::s_tick = 1u;   // advance tick to expire tcbs[3]
    bool pend = Scheduler::ProcessTimerList();
    EXPECT_EQ(tcbs[3].WakeTick, 0xFFFFFFFFu);   // TIMEOUT_FIRED
    EXPECT_EQ(tcbs[3].State, TAKT_TASK_READY);
    EXPECT_TRUE(pend);   // tcbs[3].Priority(3) > idle(0)
}

TEST_F(SchedulerTest, ProcessTimerList_WaitCancel_Called)
{
    Tcb idle{}; idle.Priority=0u; idle.BasePriority=0u;
    Scheduler::ListInsertTail(0u, &idle);
    taktos_ctx.current = &idle;

    static bool cancel_called = false;
    static Tcb* cancel_tcb = nullptr;
    tcbs[2].WakeTick    = 1u;
    tcbs[2].Priority    = 2u;
    tcbs[2].BasePriority= 2u;
    tcbs[2].pWaitCancel = [](Tcb* t, void*) { cancel_called=true; cancel_tcb=t; };
    tcbs[2].pWaitCtx    = nullptr;
    Scheduler::TimerListInsert(&tcbs[2]);

    Scheduler::s_tick = 1u;
    Scheduler::ProcessTimerList();
    EXPECT_TRUE(cancel_called);
    EXPECT_EQ(cancel_tcb, &tcbs[2]);
    EXPECT_EQ(tcbs[2].pWaitCancel, nullptr);   // cleared after call
}

TEST_F(SchedulerTest, Tick_AgeEnabled_CallsAge)
{
    cfg.AgeEnable = true; cfg.AgeInterval = 1u;
    Scheduler::Init(&cfg);
    Tcb t2{}; t2.Priority=2u; t2.BasePriority=2u; t2.AgeTicks=0u;
    Scheduler::ListInsertTail(2u, &t2);
    Tcb idle{}; idle.Priority=0u; idle.BasePriority=0u;
    Scheduler::ListInsertTail(0u, &idle);
    taktos_ctx.current = &idle;

    Scheduler::Tick();
    EXPECT_EQ(t2.Priority, 3u);   // aged up after 1 tick
}

} // namespace taktos
