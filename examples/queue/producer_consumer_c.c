/**---------------------------------------------------------------------------
@file   producer_consumer_c.c

@brief  Fixed-size queue example  C API

ProducerThread fills a Sample_t with a sequence number, tick stamp,
checksum, and flags, then sends it to the queue every 10 ms.
ConsumerThread drains the queue, checks sequence ordering and checksum
integrity, and increments gSequenceErrors on any mismatch.

Queue depth is 8; gQueueHighWater records the peak fill level.
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
#include <stddef.h>
#include <stdint.h>
#include "TaktOS.h"
#include "TaktOSThread.h"
#include "TaktOSQueue.h"

#ifndef APP_CORE_CLOCK_HZ
#define APP_CORE_CLOCK_HZ 48000000u
#endif

#define SAMPLE_QUEUE_DEPTH  8u

typedef struct __attribute__((aligned(4)))
{
    uint32_t sequence;
    uint32_t createdTick;
    uint32_t checksum;
    uint32_t flags;
} Sample_t;

static uint8_t gProducerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));
static uint8_t gConsumerThreadMem[TAKTOS_THREAD_MEM_SIZE(512u)] __attribute__((aligned(4)));
static uint8_t gQueueStorage[SAMPLE_QUEUE_DEPTH * sizeof(Sample_t)] __attribute__((aligned(4)));

static TaktOSQueue_t gSampleQueue;

volatile uint32_t gProducedCount = 0u;
volatile uint32_t gConsumedCount = 0u;
volatile uint32_t gLastSequenceConsumed = 0u;
volatile uint32_t gSequenceErrors = 0u;
volatile uint32_t gQueueHighWater = 0u;

static void ProducerThread(void *arg)
{
    (void)arg;
    uint32_t sequence = 0u;

    for (;;)
    {
        Sample_t sample;
        sample.sequence = sequence;
        sample.createdTick = TaktOSTickCount();
        sample.checksum = sample.sequence ^ sample.createdTick ^ 0x5A5AA55Au;
        sample.flags = 0x12340000u | (sequence & 0xFFFFu);

        TaktOSQueueSend(&gSampleQueue, &sample, true, TAKTOS_WAIT_FOREVER);

        ++gProducedCount;
        ++sequence;

        uint32_t depth = TaktOSQueueCount(&gSampleQueue);
        if (depth > gQueueHighWater)
        {
            gQueueHighWater = depth;
        }

        TaktOSThreadSleep(TaktOSCurrentThread(), 10u);
    }
}

static void ConsumerThread(void *arg)
{
    (void)arg;
    uint32_t expected = 0u;

    for (;;)
    {
        Sample_t sample;
        TaktOSQueueReceive(&gSampleQueue, &sample, true, TAKTOS_WAIT_FOREVER);

        if (sample.sequence != expected)
        {
            ++gSequenceErrors;
            expected = sample.sequence;
        }

        if (sample.checksum != (sample.sequence ^ sample.createdTick ^ 0x5A5AA55Au))
        {
            ++gSequenceErrors;
        }

        gLastSequenceConsumed = sample.sequence;
        ++gConsumedCount;
        ++expected;
    }
}

int main(void)
{
    TaktOSInit(APP_CORE_CLOCK_HZ, 1000u, TAKTOS_TICK_CLOCK_PROCESSOR, 0u);
    TaktOSQueueInit(&gSampleQueue, gQueueStorage, sizeof(Sample_t), SAMPLE_QUEUE_DEPTH);

    TaktOSThreadCreate(gProducerThreadMem, sizeof(gProducerThreadMem),
                       ProducerThread, NULL, TAKTOS_PRIORITY_LOW);
    TaktOSThreadCreate(gConsumerThreadMem, sizeof(gConsumerThreadMem),
                       ConsumerThread, NULL, TAKTOS_PRIORITY_NORMAL);

    TaktOSStart();
}
