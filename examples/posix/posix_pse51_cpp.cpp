/**---------------------------------------------------------------------------
@file   posix_pse51_cpp.cpp

@brief  POSIX PSE51 layer example  C++

Same scenario as posix_pse51.c using C++ idioms.
A MutexGuard RAII wrapper handles the pthread_mutex_t lock/unlock in the
producer thread.  All other POSIX calls are identical to the C version.

Check gProducedCount == gConsumedCount == 5 and gTimerThreadUnblocked == 1
in a debugger after gBootstrapComplete is set.

@author Nguyen Hoan Hoang
@date   Apr. 2026

@license

MIT License

Copyright (c) 2026 I-SYST inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----------------------------------------------------------------------------*/
#include <cstddef>
#include <cstdint>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "posix/pthread.h"
#include "posix/semaphore.h"
#include "posix/time.h"
#include "posix/ptimer.h"

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

class MutexGuard {
public:
    explicit MutexGuard(pthread_mutex_t &mutex) : mutex_(mutex)
    {
        pthread_mutex_lock(&mutex_);
    }
    ~MutexGuard()
    {
        pthread_mutex_unlock(&mutex_);
    }
    MutexGuard(const MutexGuard &) = delete;
    MutexGuard &operator=(const MutexGuard &) = delete;
private:
    pthread_mutex_t &mutex_;
};

static uint8_t gBootstrapThreadMem[TAKTOS_THREAD_MEM_SIZE(1024u)] __attribute__((aligned(4)));
static TaktOSThread gBootstrapThread;

static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  gCond  = PTHREAD_COND_INITIALIZER;
static sem_t           gSem;

volatile uint32_t gProducedCount = 0u;
volatile uint32_t gConsumedCount = 0u;
volatile uint32_t gTimerCallbackCount = 0u;
volatile uint32_t gTimerThreadUnblocked = 0u;
volatile uint32_t gBootstrapComplete = 0u;
static volatile int gPending = 0;

static void SleepMs(uint32_t ms)
{
    timespec ts{};
    ts.tv_sec = static_cast<time_t>(ms / 1000u);
    ts.tv_nsec = static_cast<long>((ms % 1000u) * 1000000u);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
}

static void *ProducerThread(void *)
{
    for (uint32_t i = 0u; i < 5u; ++i)
    {
        SleepMs(20u);
        {
            MutexGuard lock(gMutex);
            ++gPending;
            ++gProducedCount;
            pthread_cond_signal(&gCond);
        }
    }
    return nullptr;
}

static void *ConsumerThread(void *)
{
    for (uint32_t i = 0u; i < 5u; ++i)
    {
        pthread_mutex_lock(&gMutex);
        while (gPending == 0)
        {
            pthread_cond_wait(&gCond, &gMutex);
        }
        --gPending;
        ++gConsumedCount;
        pthread_mutex_unlock(&gMutex);
    }
    return nullptr;
}

static void *SemWaiterThread(void *)
{
    sem_wait(&gSem);
    ++gTimerThreadUnblocked;
    return nullptr;
}

static void TimerCallback(union sigval value)
{
    (void)value;
    ++gTimerCallbackCount;
    sem_post(&gSem);
}

static pthread_t SpawnThread(void *(*entry)(void *), int priority)
{
    pthread_t tid{};
    pthread_attr_t attr{};
    sched_param sched{};

    pthread_attr_init(&attr);
    sched.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &sched);
    pthread_create(&tid, &attr, entry, nullptr);
    pthread_attr_destroy(&attr);
    return tid;
}

static void BootstrapThreadEntry(void *)
{
    sem_init(&gSem, 0, 0u);

    timer_t timerId{};
    sigevent sev{};
    itimerspec its{};

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_int = 1;
    sev.sigev_notify_function = TimerCallback;
    sev.sigev_notify_attributes = nullptr;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 100000000L;

    timer_create(CLOCK_MONOTONIC, &sev, &timerId);
    timer_settime(timerId, 0, &its, nullptr);

    pthread_t producer = SpawnThread(ProducerThread, TAKTOS_PRIORITY_NORMAL);
    pthread_t consumer = SpawnThread(ConsumerThread, TAKTOS_PRIORITY_NORMAL);
    pthread_t waiter   = SpawnThread(SemWaiterThread, TAKTOS_PRIORITY_LOW);

    pthread_join(producer, nullptr);
    pthread_join(consumer, nullptr);
    pthread_join(waiter, nullptr);

    timer_delete(timerId);
    sem_destroy(&gSem);
    gBootstrapComplete = 1u;

    for (;;)
    {
        TaktOSThreadSleep(TaktOSCurrentThread(), 1000u);
    }
}

int main()
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    gBootstrapThread.Create(gBootstrapThreadMem, sizeof(gBootstrapThreadMem),
                            BootstrapThreadEntry, nullptr, TAKTOS_PRIORITY_NORMAL);
    TaktOSStart();
}
