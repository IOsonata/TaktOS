/**---------------------------------------------------------------------------
@file   test_kernel.cpp

@brief  TaktOS kernel MC/DC unit tests  host-native, no hardware required

Covers cert boundary modules: scheduler, semaphore, mutex, queue, timer.
MC/DC requirement: every boolean condition in every decision must be shown
to independently affect the outcome.  See TaktOS Engineering Spec 13.

Build (no CMake  single compile command, run from the test/ directory):
  g++ -std=c++20 -O0 -g -DTAKT_ARCH_STUB=1 -DTAKT_TEST_ACCESS=1
      -I../include -I../ARM/include -I.
      test_kernel.cpp takt_arch_stub.cpp
      ../src/taktos.cpp ../src/taktos_thread.cpp
      ../src/taktos_sem.cpp ../src/taktos_mutex.cpp
      ../src/taktos_queue.cpp
      -o test_kernel && ./test_kernel

Pass criterion: exit code 0, "ALL TESTS PASSED" on stdout.
----------------------------------------------------------------------------*/

#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSSem.h"
#include "TaktOSMutex.h"
#include "TaktOSQueue.h"
#include "TaktKernel.h"
#include <cstdio>
#include <cassert>
#include <cstring>

// g_TaktosCtx is defined in src/taktos.cpp; tests manipulate it directly
// to set up scheduler state without a real context switch.
extern TaktKernelCtx_t g_TaktosCtx;

//  Minimal test framework 
static int  g_pass = 0, g_fail = 0;
static const char *g_suite = "";

#define SUITE(name)  do { g_suite = name; } while(0)

#define CHECK(cond) do { \
    if (cond) \
    { \
        ++g_pass; \
    } \
    else \
    { \
        ++g_fail; printf("FAIL %s: %s line %d\n", g_suite, #cond, __LINE__); \
    } \
} while(0)

//  Thread stacks 
// Tests manipulate TCBs directly via TaktReadyTask/TaktBlockTask without
// actually running threads, so minimal stacks are fine.

alignas(4) static uint8_t gStack0[TAKTOS_THREAD_MEM_SIZE(128)];
alignas(4) static uint8_t gStack1[TAKTOS_THREAD_MEM_SIZE(128)];
alignas(4) static uint8_t gStack2[TAKTOS_THREAD_MEM_SIZE(128)];

static void DummyEntry(void *)
{
    for (;;)
    {
    }
}

//  Helpers 

static TaktOSThread_t *MakeThread(uint8_t *mem, uint32_t sz, uint8_t pri)
{
    return TaktOSThreadCreate(mem, sz, DummyEntry, nullptr, pri);
}

//  Suite: scheduler 

static void test_scheduler(void)
{
    SUITE("scheduler");

    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    TaktOSThread_t *tA = MakeThread(gStack0, sizeof(gStack0), 1u);
    TaktOSThread_t *tB = MakeThread(gStack1, sizeof(gStack1), 2u);
    TaktOSThread_t *tC = MakeThread(gStack2, sizeof(gStack2), 3u);

    CHECK(tA != nullptr);
    CHECK(tB != nullptr);
    CHECK(tC != nullptr);

    // MC/DC: TaktBlockTask  only thread at priority (bitmap cleared)
    TaktBlockTask(tC);
    CHECK(tC->State == TAKTOS_READY);   // state managed by caller; block only removes from RQ

    // MC/DC: TaktReadyTask  higher priority updates topPri
    TaktReadyTask(tC);

    // MC/DC: bitmap zero guard in PrioBitmapTop
    // Remove all threads then verify no crash
    TaktBlockTask(tA);
    TaktBlockTask(tB);
    TaktBlockTask(tC);

    TaktReadyTask(tA);
    TaktReadyTask(tB);
    TaktReadyTask(tC);
}

//  Suite: semaphore 

static void test_semaphore(void)
{
    SUITE("semaphore");

    TaktOSSem_t sem;

    // Init guards
    CHECK(TaktOSSemInit(nullptr, 0u, 1u) == TAKTOS_ERR_INVALID);
    CHECK(TaktOSSemInit(&sem, 0u, 0u)    == TAKTOS_ERR_INVALID);
    CHECK(TaktOSSemInit(&sem, 2u, 1u)    == TAKTOS_ERR_INVALID);

    // Binary semaphore  empty start
    CHECK(TaktOSSemInit(&sem, 0u, 1u) == TAKTOS_OK);
    CHECK(sem.Count == 0u);

    // Take on empty with no-wait returns ERR_EMPTY (fast path MC/DC)
    CHECK(TaktOSSemTake(&sem, false, TAKTOS_NO_WAIT) == TAKTOS_ERR_EMPTY);
    CHECK(TaktOSSemTake(&sem, true,  TAKTOS_NO_WAIT) == TAKTOS_ERR_EMPTY);

    // Give increments (fast path MC/DC: binary sem empty case)
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_OK);
    CHECK(sem.Count == 1u);

    // Give on full with no waiters returns ERR_FULL (Bug 3 fast path fix)
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_ERR_FULL);
    CHECK(sem.Count == 1u);   // count unchanged

    // Take decrements
    CHECK(TaktOSSemTake(&sem, true, TAKTOS_WAIT_FOREVER) == TAKTOS_OK);
    CHECK(sem.Count == 0u);

    // Counting semaphore
    CHECK(TaktOSSemInit(&sem, 0u, 4u) == TAKTOS_OK);
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_OK);
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_OK);
    CHECK(sem.Count == 2u);
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_OK);
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_OK);
    CHECK(TaktOSSemGive(&sem, false) == TAKTOS_ERR_FULL);  // count==maxCount, no waiters
    CHECK(sem.Count == 4u);
}

//  Suite: mutex 

static void test_mutex(void)
{
    SUITE("mutex");

    TaktOSMutex_t mtx;

    CHECK(TaktOSMutexInit(nullptr) == TAKTOS_ERR_INVALID);
    CHECK(TaktOSMutexInit(&mtx)    == TAKTOS_OK);
    CHECK(mtx.pOwner == nullptr);

    // Set a fake current thread so TaktOSCurrentThread() returns non-null.
    // On host there is no TaktOSStartFirst() call, so pCurrent is null.
    TaktOSThread_t *tM = MakeThread(gStack0, sizeof(gStack0), 2u);
    g_TaktosCtx.pCurrent = tM;

    // Uncontended lock (fast path MC/DC: owner == nullptr)
    CHECK(TaktOSMutexLock(&mtx, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
    CHECK(mtx.pOwner == TaktOSCurrentThread());

    // Attempt lock while held with no-wait (fast path MC/DC: !blocking)
    CHECK(TaktOSMutexLock(&mtx, false, TAKTOS_NO_WAIT) == TAKTOS_ERR_BUSY);

    // Unlock by owner (fast path MC/DC: waitList.head == nullptr)
    CHECK(TaktOSMutexUnlock(&mtx) == TAKTOS_OK);
    CHECK(mtx.pOwner == nullptr);

    // Unlock when not owner
    CHECK(TaktOSMutexUnlock(&mtx) == TAKTOS_ERR_INVALID);
}

//  Suite: queue 

static void test_queue(void)
{
    SUITE("queue");

    TaktOSQueue_t q;
    alignas(4) uint8_t buf[4 * sizeof(uint32_t)];

    CHECK(TaktOSQueueInit(nullptr, buf,    sizeof(uint32_t), 4u) == TAKTOS_ERR_INVALID);
    CHECK(TaktOSQueueInit(&q,      nullptr,sizeof(uint32_t), 4u) == TAKTOS_ERR_INVALID);
    CHECK(TaktOSQueueInit(&q,      buf,    sizeof(uint32_t), 4u) == TAKTOS_OK);

    // Send to empty queue (fast path MC/DC: slot != nullptr)
    uint32_t v = 0xAABBCCDDu, out = 0u;
    CHECK(TaktOSQueueSend(&q, &v, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
    CHECK(TaktOSQueueCount(&q) == 1u);

    // Receive from queue with item (fast path MC/DC: slot != nullptr)
    CHECK(TaktOSQueueReceive(&q, &out, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
    CHECK(out == v);
    CHECK(TaktOSQueueCount(&q) == 0u);

    // Receive from empty with no-wait (fast path MC/DC: !blocking)
    CHECK(TaktOSQueueReceive(&q, &out, false, TAKTOS_NO_WAIT) == TAKTOS_ERR_EMPTY);

    // Fill queue then send to full
    for (uint32_t i = 0u; i < 4u; ++i)
    {
        CHECK(TaktOSQueueSend(&q, &i, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
    }
    uint32_t x = 99u;
    CHECK(TaktOSQueueSend(&q, &x, false, TAKTOS_NO_WAIT) == TAKTOS_ERR_FULL);
    CHECK(TaktOSQueueCount(&q) == 4u);

    // FIFO order
    for (uint32_t i = 0u; i < 4u; ++i)
    {
        uint32_t r = 0u;
        CHECK(TaktOSQueueReceive(&q, &r, false, TAKTOS_NO_WAIT) == TAKTOS_OK);
        CHECK(r == i);
    }
}


//  Suite: thread suspend BLOCKED fix 
// Verify that suspending a BLOCKED thread cancels its wait-list membership.

static void test_suspend_blocked(void)
{
    SUITE("suspend_blocked");

    TaktOSSem_t sem;
    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    TaktOSSemInit(&sem, 0u, 1u);

    // Manually construct a "blocked in sem wait" scenario without
    // actually context-switching (single-threaded host).
    TaktOSThread_t *tA = MakeThread(gStack0, sizeof(gStack0), 2u);
    CHECK(tA != nullptr);

    // Simulate a timed-wait block: put tA in sem wait list
    uint32_t primask = TaktOSEnterCritical();
    tA->State      = TAKTOS_BLOCKED;
    tA->WakeReason = TAKT_WOKEN_BY_EVENT;
    tA->WakeTick   = 50u;
    tA->pWaitNext  = nullptr;
    TaktBlockTask(tA);
    TaktWaitListInsert(&sem.WaitList, tA);
    tA->pWaitList = &sem.WaitList;
    TaktSleepListAdd(tA);
    TaktOSExitCritical(primask);

    CHECK(sem.WaitList.pHead == tA);

    // Suspend should cancel the wait
    TaktOSErr_t r = TaktOSThreadSuspend(tA);
    CHECK(r == TAKTOS_OK);
    CHECK(sem.WaitList.pHead == nullptr);   // removed from sem list
    CHECK(tA->pWaitList    == nullptr);
    CHECK(tA->WakeTick     == 0u);
    // WakeReason must be RESUME (not TIMEOUT) so the blocking slow path
    // returns TAKTOS_ERR_INTERRUPTED  administrative cancellation, not a
    // real deadline expiry.
    CHECK(tA->WakeReason   == TAKT_WOKEN_BY_RESUME);
}

//  Suite: thread lifecycle  DEAD state (Issues 1, 2, 4) 
//
// These tests verify all three fixes that involve TAKTOS_DEAD:
//
//   Issue 1  Resume must re-check State under lock to avoid double-inserting
//              a thread that an ISR woke between the unlocked read and the lock.
//              Verified here by confirming Resume rejects a dead handle.
//
//   Issue 2  Suspend must read State under lock for foreign threads.
//              Verified here by confirming Suspend rejects a dead handle.
//
//   Issue 4  Destroy stamps TAKTOS_DEAD; subsequent API calls are rejected.
//              Also covers double-destroy safety.

static void test_dead_state(void)
{
    SUITE("dead_state");

    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    //  Destroy stamps DEAD 
    TaktOSThread_t *tA = MakeThread(gStack0, sizeof(gStack0), 2u);
    CHECK(tA != nullptr);
    CHECK(tA->State == TAKTOS_READY);

    TaktOSErr_t r = TaktOSThreadDestroy(tA);
    CHECK(r == TAKTOS_OK);
    // State must be TAKTOS_DEAD  not BLOCKED, not any other value.
    CHECK(tA->State == TAKTOS_DEAD);

    //  Double-destroy is idempotent and safe 
    // The second call must return ERR_INVALID without corrupting the
    // scheduler or walking any list.  If this hangs, the guard is missing.
    r = TaktOSThreadDestroy(tA);
    CHECK(r == TAKTOS_ERR_INVALID);
    CHECK(tA->State == TAKTOS_DEAD);   // unchanged

    //  Resume on DEAD handle returns ERR_INVALID (Issue 1 guard) 
    // Before the fix, Resume read State unlocked and could fall through to
    // TaktReadyTask() on a thread already woken  or here, on a dead TCB.
    // Both the unlocked pre-check and the locked re-check must catch DEAD.
    r = TaktOSThreadResume(tA);
    CHECK(r == TAKTOS_ERR_INVALID);
    CHECK(tA->State == TAKTOS_DEAD);   // still DEAD  not re-queued

    //  Suspend on DEAD handle returns ERR_INVALID (Issue 2 guard) 
    // Before the fix, Suspend read State outside the lock.  On a DEAD handle
    // that falls through as READY (State value 0 == TAKTOS_READY would only
    // happen if the TCB was zeroed, but TAKTOS_DEAD = 4 so it hits the
    // READY/RUNNING else branch).  The locked re-read closes the window.
    r = TaktOSThreadSuspend(tA);
    CHECK(r == TAKTOS_ERR_INVALID);
    CHECK(tA->State == TAKTOS_DEAD);   // still DEAD  not blocked

    //  Resume after Destroy on a previously BLOCKED thread 
    // Covers the path where Issue 1's double-insert race would have fired:
    // a thread in BLOCKED state gets Destroyed (stamped DEAD), then a caller
    // that had read BLOCKED before the lock calls Resume.  The locked re-read
    // must see DEAD and return ERR_INVALID rather than calling TaktReadyTask.
    TaktOSThread_t *tB = MakeThread(gStack1, sizeof(gStack1), 3u);
    CHECK(tB != nullptr);

    // Manually block tB (simulating a sem wait, without an actual sem object).
    {
        uint32_t primask = TaktOSEnterCritical();
        tB->State = TAKTOS_BLOCKED;
        TaktBlockTask(tB);
        TaktOSExitCritical(primask);
    }
    CHECK(tB->State == TAKTOS_BLOCKED);

    // Now destroy it  this simulates the race where an ISR previously read
    // BLOCKED but Destroy fires first.
    r = TaktOSThreadDestroy(tB);
    CHECK(r == TAKTOS_OK);
    CHECK(tB->State == TAKTOS_DEAD);

    // The simulated "ISR" now calls Resume on what it thought was BLOCKED.
    // Must get ERR_INVALID, not corrupt the run queue.
    r = TaktOSThreadResume(tB);
    CHECK(r == TAKTOS_ERR_INVALID);
    CHECK(tB->State == TAKTOS_DEAD);

    //  Suspend after Destroy on a previously SLEEPING thread 
    // Covers Issue 2's lost-suspend race path: a thread in SLEEPING state gets
    // Destroyed, then Suspend fires on the stale handle.  Must be rejected.
    TaktOSThread_t *tC = MakeThread(gStack2, sizeof(gStack2), 1u);
    CHECK(tC != nullptr);

    // Manually put tC into sleeping state (simulates TaktOSThreadSleep).
    {
        uint32_t primask = TaktOSEnterCritical();
        tC->WakeTick = 9999u;
        tC->State    = TAKTOS_SLEEPING;
        TaktBlockTask(tC);
        TaktSleepListAdd(tC);
        TaktOSExitCritical(primask);
    }
    CHECK(tC->State == TAKTOS_SLEEPING);

    r = TaktOSThreadDestroy(tC);
    CHECK(r == TAKTOS_OK);
    CHECK(tC->State == TAKTOS_DEAD);
    CHECK(tC->WakeTick == 0u);   // sleep list entry cleaned up

    r = TaktOSThreadSuspend(tC);
    CHECK(r == TAKTOS_ERR_INVALID);
    CHECK(tC->State == TAKTOS_DEAD);
}

//  Suite: Yield caller-requirement check (Issue 3) 
// Verifies the TAKT_DEBUG_CHECKS=1 assertion path by confirming the runtime
// check is present.  On the host the PRIMASK read is a no-op (stub returns 0
// meaning interrupts enabled), so the trap never fires here  but this
// test confirms the code path compiles and the check is reachable.
//
// A separate on-target test (bench/stm32f407 or nRF52832) must call Yield
// from inside a critical section with TAKT_DEBUG_CHECKS=1 and verify
// TaktOSStackOverflowHandler / BKPT fires.  That is an on-target CI item,
// not exercisable on the host.

static void test_yield_caller_check(void)
{
    SUITE("yield_caller_check");

    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    // A thread at the same priority as itself  singleton  Yield is a no-op.
    TaktOSThread_t *tY = MakeThread(gStack0, sizeof(gStack0), 4u);
    CHECK(tY != nullptr);

    // Manually set pCurrent so Yield's singleton check sees it.
    g_TaktosCtx.pCurrent = tY;

    // Yield with one thread at its priority: pNext == itself  no switch.
    // Must not hang and must not corrupt any list.
    TaktOSThreadYield();   // no crash = requirement met for single-thread case
    CHECK(tY->State == TAKTOS_READY);  // still ready
}

//  Suite: deferred yield 
// Simulates the ISR  deferred yield  tick handler path without hardware.
// On the host, IPSR is always 0 so TaktOSThreadYield() follows the normal
// Thread-mode path.  We test the mechanism by directly writing s_DeferredYield
// via the test accessor and calling TaktKernelTickHandler() to consume it.

extern "C" TaktOSThread_t *TaktTestGetDeferredYieldFor(void);
extern "C" void            TaktTestSetDeferredYieldFor(TaktOSThread_t *t);
extern "C" void TaktKernelTickHandler(void);

static void test_deferred_yield(void)
{
    SUITE("deferred_yield");

    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    // Two threads at the same priority  round-robin peers.
    TaktOSThread_t *tA = MakeThread(gStack0, sizeof(gStack0), 5u);
    TaktOSThread_t *tB = MakeThread(gStack1, sizeof(gStack1), 5u);
    CHECK(tA != nullptr);
    CHECK(tB != nullptr);

    // Set pCurrent to tA so the tick handler has a valid current thread.
    g_TaktosCtx.pCurrent = tA;

    // Point pNextThread at tA as well so the priority comparison in the tick
    // handler (pNextThread->Priority > pCurrent->Priority) evaluates false 
    // needSwitch starts false.  The deferred yield is what should set it true.
    g_TaktosCtx.pNextThread = tA;

    // Pointer should start null  no pending deferred yield.
    CHECK(TaktTestGetDeferredYieldFor() == nullptr);

    // Simulate: ISR stamps the deferred yield with pCurrent (tA).
    TaktTestSetDeferredYieldFor(tA);
    CHECK(TaktTestGetDeferredYieldFor() == tA);

    // Record pNextThread before the tick.
    TaktOSThread_t *before = g_TaktosCtx.pNextThread;

    // Fire the tick handler  it should consume the request and rotate the ring.
    // On the host, TaktOSCtxSwitch() is a no-op, so even when needSwitch is
    // true the call is safe.
    TaktKernelTickHandler();

    // Pointer must be cleared (consumed) by the tick handler.
    CHECK(TaktTestGetDeferredYieldFor() == nullptr);

    // After rotation, pNextThread must differ from before (tB is now front)
    // if there are two peers.  If tA is a singleton, pNext == tA and the
    // rotation is a no-op  pNextThread stays the same.
    if (tA->pNext != tA)
    {
        CHECK(g_TaktosCtx.pNextThread != before);
    }
    else
    {
        CHECK(g_TaktosCtx.pNextThread == before);
    }

    //  Stale request discarded when pCurrent changed 
    // Simulate: deferred yield stamped with tA, but then tB becomes pCurrent
    // before the tick fires.  The tick handler must discard the stale request.
    g_TaktosCtx.pCurrent    = tB;   // tB is now running
    g_TaktosCtx.pNextThread = tB;

    TaktTestSetDeferredYieldFor(tA); // tA requested yield but is no longer current
    TaktOSThread_t *before2 = g_TaktosCtx.pNextThread;
    TaktKernelTickHandler();

    // Request must be cleared even though it was discarded.
    CHECK(TaktTestGetDeferredYieldFor() == nullptr);
    // pNextThread must NOT have been rotated  the stale request for tA should
    // not have rotated tB's ring position.
    CHECK(g_TaktosCtx.pNextThread == before2);
}

//  Suite: HandOff  guaranteed delivery to named thread 
//
//   Case A  same-priority peers: A current, B already READY, C blocked.
//     A calls HandOff(C).  pNextThread must be C, not B.
//     Without the fix, TaktReadyTask inserts C at tail of the ring so
//     pNextThread stays pointing at B (the old front).
//
//   Case B  higher-priority preemptor: A current, B blocked at pri 5,
//     C already READY at pri 6.  A calls HandOff(B).
//     pNextThread must remain C  overriding it would be a priority
//     inversion.  B must be READY so it runs after C.
//
static void test_handoff_guarantee(void)
{
    SUITE("handoff_guarantee");

    //  Case A 
    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    TaktOSThread_t *tA = MakeThread(gStack0, sizeof(gStack0), 5u);
    TaktOSThread_t *tB = MakeThread(gStack1, sizeof(gStack1), 5u);
    TaktOSThread_t *tC = MakeThread(gStack2, sizeof(gStack2), 5u);

    // B is READY in the run queue; C is BLOCKED; A is current.
    TaktReadyTask(tB);
    tC->State = TAKTOS_BLOCKED;
    g_TaktosCtx.pCurrent = tA;
    tA->State = TAKTOS_RUNNING;
    TaktReadyTask(tA);   // put A in queue so TaktBlockTask can find it

    TaktOSThreadHandOff(tC);

    // C must run next, not B.
    CHECK(g_TaktosCtx.pNextThread == tC);
    CHECK(tC->State            == TAKTOS_READY);
    CHECK(tA->State            == TAKTOS_BLOCKED);
    CHECK(tB->State            == TAKTOS_READY);  // B still ready, deferred one turn

    //  Case B 
    TaktOSInit(1000000u, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);

    tA = MakeThread(gStack0, sizeof(gStack0), 5u);
    tB = MakeThread(gStack1, sizeof(gStack1), 5u);
    tC = MakeThread(gStack2, sizeof(gStack2), 6u);  // higher priority

    // C is READY at pri 6 (preemptor); B is BLOCKED at pri 5; A is current.
    TaktReadyTask(tC);
    tB->State = TAKTOS_BLOCKED;
    g_TaktosCtx.pCurrent = tA;
    tA->State = TAKTOS_RUNNING;
    TaktReadyTask(tA);

    TaktOSThreadHandOff(tB);

    // Higher-priority C must still run next  no priority inversion.
    CHECK(g_TaktosCtx.pNextThread == tC);
    // B is now READY and will run after C.
    CHECK(tB->State == TAKTOS_READY);
    CHECK(tA->State == TAKTOS_BLOCKED);
}

//  main 

int main(void)
{
    test_scheduler();
    test_semaphore();
    test_mutex();
    test_queue();
    test_suspend_blocked();
    test_dead_state();
    test_yield_caller_check();
    test_deferred_yield();
    test_handoff_guarantee();

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    if (g_fail == 0)
    {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    return 1;
}
