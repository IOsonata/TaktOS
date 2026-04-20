/**---------------------------------------------------------------------------
@file   producer_consumer.cpp

@brief  Fixed-size queue example  C++ API

Same producer-consumer scenario as producer_consumer_c.c using the
C++ TaktOSQueue wrapper class.

ProducerThread sends a Sample struct (sequence, tick, checksum, flags)
every 10 ms.  ConsumerThread verifies ordering and checksum integrity,
incrementing gSequenceErrors on any mismatch.

Inspect the volatile counters in a debugger.

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
#include "TaktOSQueue.h"

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

static constexpr uint32_t kQueueDepth = 8u;

struct alignas(4) Sample {
    uint32_t sequence;
    uint32_t createdTick;
    uint32_t checksum;
    uint32_t flags;
};

static uint8_t gProducerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));
static uint8_t gConsumerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));
static uint8_t gQueueStorage[kQueueDepth * sizeof(Sample)] __attribute__((aligned(4)));

static TaktOSThread gProducerThread;
static TaktOSThread gConsumerThread;
static TaktOSQueue  gSampleQueue;

volatile uint32_t gProducedCount = 0u;
volatile uint32_t gConsumedCount = 0u;
volatile uint32_t gLastSequenceConsumed = 0u;
volatile uint32_t gSequenceErrors = 0u;
volatile uint32_t gQueueHighWater = 0u;

static void ProducerThreadEntry(void *)
{
    uint32_t sequence = 0u;

    for (;;)
    {
        Sample sample{};
        sample.sequence = sequence;
        sample.createdTick = TaktOSTickCount();
        sample.checksum = sample.sequence ^ sample.createdTick ^ 0xA55A5AA5u;
        sample.flags = 0xBEEF0000u | (sequence & 0xFFFFu);

        gSampleQueue.Send(&sample, true);

        ++gProducedCount;
        ++sequence;

        uint32_t depth = gSampleQueue.Count();
        if (depth > gQueueHighWater)
        {
            gQueueHighWater = depth;
        }

        TaktOSThreadSleep(TaktOSCurrentThread(), 10u);
    }
}

static void ConsumerThreadEntry(void *)
{
    uint32_t expected = 0u;

    for (;;)
    {
        Sample sample{};
        gSampleQueue.Receive(&sample, true, TAKTOS_WAIT_FOREVER);

        if (sample.sequence != expected)
        {
            ++gSequenceErrors;
            expected = sample.sequence;
        }

        if (sample.checksum != (sample.sequence ^ sample.createdTick ^ 0xA55A5AA5u))
        {
            ++gSequenceErrors;
        }

        gLastSequenceConsumed = sample.sequence;
        ++gConsumedCount;
        ++expected;
    }
}

int main()
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    gSampleQueue.Init(gQueueStorage, sizeof(Sample), kQueueDepth);

    gProducerThread.Create(gProducerThreadMem, sizeof(gProducerThreadMem),
                           ProducerThreadEntry, nullptr, TAKTOS_PRIORITY_LOW);
    gConsumerThread.Create(gConsumerThreadMem, sizeof(gConsumerThreadMem),
                           ConsumerThreadEntry, nullptr, TAKTOS_PRIORITY_NORMAL);

    TaktOSStart();
}
