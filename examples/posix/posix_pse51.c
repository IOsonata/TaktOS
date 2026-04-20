/**---------------------------------------------------------------------------
@file   posix_pse51.c

@brief  POSIX PSE51 layer example  C

Starts TaktOS then launches a native TaktOS bootstrap thread that creates:
  - A producer/consumer pair using pthread_mutex_t and pthread_cond_t
  - A sem_t waiter thread unblocked by a one-shot POSIX timer callback
  - A timer_t (SIGEV_THREAD) firing after 100 ms

All threads are joined before the bootstrap thread returns to idle.
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
#include <stddef.h>
#include <stdint.h>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "posix/pthread.h"
#include "posix/semaphore.h"
#include "posix/time.h"
#include "posix/ptimer.h"

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

static uint8_t gBootstrapThreadMem[TAKTOS_THREAD_MEM_SIZE(1024u)] __attribute__((aligned(4)));

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
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
}

static void *ProducerThread(void *arg)
{
    (void)arg;
    for (uint32_t i = 0u; i < 5u; ++i)
    {
        SleepMs(20u);
        pthread_mutex_lock(&gMutex);
        ++gPending;
        ++gProducedCount;
        pthread_cond_signal(&gCond);
        pthread_mutex_unlock(&gMutex);
    }
    return NULL;
}

static void *ConsumerThread(void *arg)
{
    (void)arg;
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
    return NULL;
}

static void *SemWaiterThread(void *arg)
{
    (void)arg;
    sem_wait(&gSem);
    ++gTimerThreadUnblocked;
    return NULL;
}

static void TimerCallback(union sigval value)
{
    (void)value;
    ++gTimerCallbackCount;
    sem_post(&gSem);
}

static pthread_t SpawnThread(void *(*entry)(void *), int priority)
{
    pthread_t tid;
    pthread_attr_t attr;
    struct sched_param sched;

    pthread_attr_init(&attr);
    sched.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &sched);
    pthread_create(&tid, &attr, entry, NULL);
    pthread_attr_destroy(&attr);
    return tid;
}

static void BootstrapThread(void *arg)
{
    (void)arg;

    sem_init(&gSem, 0, 0u);

    timer_t timerId;
    struct sigevent sev;
    struct itimerspec its;
    pthread_t producer;
    pthread_t consumer;
    pthread_t waiter;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_signo = 0;
    sev.sigev_value.sival_int = 1;
    sev.sigev_notify_function = TimerCallback;
    sev.sigev_notify_attributes = NULL;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 100000000L;

    timer_create(CLOCK_MONOTONIC, &sev, &timerId);
    timer_settime(timerId, 0, &its, NULL);

    producer = SpawnThread(ProducerThread, TAKTOS_PRIORITY_NORMAL);
    consumer = SpawnThread(ConsumerThread, TAKTOS_PRIORITY_NORMAL);
    waiter = SpawnThread(SemWaiterThread, TAKTOS_PRIORITY_LOW);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    pthread_join(waiter, NULL);

    timer_delete(timerId);
    sem_destroy(&gSem);
    gBootstrapComplete = 1u;

    for (;;)
    {
        TaktOSThreadSleep(TaktOSCurrentThread(), 1000u);
    }
}

int main(void)
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    TaktOSThreadCreate(gBootstrapThreadMem, sizeof(gBootstrapThreadMem),
                       BootstrapThread, NULL, TAKTOS_PRIORITY_NORMAL);
    TaktOSStart();
}
