/**---------------------------------------------------------------------------
@file   mqueue.cpp

@brief  TaktOS PSE51  POSIX message queue implementation (mq_*)

Implements mq_open / mq_close / mq_unlink / mq_send / mq_receive /
mq_getattr / mq_setattr.

Each named queue maps to a static pool slot.  Two TaktOSSem_t (spaces,
items) provide blocking and timeouts.  Message slots are kept on index-linked
free / message lists updated under short kernel critical sections; payload
copies run outside the critical section.  Messages are retrieved in
descending priority order with FIFO ordering within the same priority level.

QM  outside cert boundary.

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
#include <stdarg.h>
#include <string.h>
#include "posix/mqueue.h"
#include "TaktOSSem.h"

// Helper: translate ticks to blocking/non-blocking sem calls
static inline TaktOSErr_t sem_take_timed(TaktOSSem_t* s, uint32_t ticks)
{
    if (ticks == TAKTOS_NO_WAIT)
    {
        return TaktOSSemTake(s, false, TAKTOS_NO_WAIT);
    }
    return TaktOSSemTake(s, true, ticks);
}

// Message slots are linked by 8-bit index.  MQ_NIL terminates a list.
#define MQ_NIL  0xFFu
static_assert(TAKT_POSIX_MQ_MAX_MSG < MQ_NIL, "TAKT_POSIX_MQ_MAX_MSG must be < 255");

struct MqMsg {
    uint8_t      next;      // next index in the free list or the message list
    unsigned int prio;
    uint32_t     len;
    uint8_t      data[TAKT_POSIX_MQ_MAX_MSGSIZE];
};

// Flow control:
//   spaces : one token per free slot  -> a sender holding a token is
//            guaranteed a slot on the free list
//   items  : one token per linked message -> a receiver holding a token is
//            guaranteed a message on the message list
// Tokens are given only after the list operation is published, so both
// guarantees hold with any number of concurrent senders / receivers.
//
// List manipulation is a few loads/stores done under the kernel critical
// section.  The message payload copy is done outside it: between unlink and
// relink the slot index is owned by exactly one thread.
//
// Message list order: descending priority, FIFO within a priority.  Append
// at tail is O(1) whenever the new priority is <= the tail priority (all
// messages at one priority is the common case).  A higher-priority message
// walks from the head to its insert point (at most maxmsg nodes).
struct MqSlot {
    TaktOSSem_t   items;
    TaktOSSem_t   spaces;
    MqMsg         msgs[TAKT_POSIX_MQ_MAX_MSG];
    uint8_t       freeHead;
    uint8_t       head;     // highest priority, oldest
    uint8_t       tail;     // lowest priority, newest
    uint32_t      msgsize;
    uint32_t      maxmsg;
    uint32_t      curmsgs;
    int           flags;
    char          name[TAKT_POSIX_MQ_NAMEMAX];
    bool          in_use;
};

static MqSlot s_mqs[TAKT_POSIX_MAX_MQS];

static MqSlot* mq_find(const char* name)
{
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_MQS; ++i)
    {
        if (s_mqs[i].in_use &&
            strncmp(s_mqs[i].name, name, TAKT_POSIX_MQ_NAMEMAX) == 0)
        {
            return &s_mqs[i];
        }
    }
    return nullptr;
}

static MqSlot* mq_slot(mqd_t d)
{
    if (d < 0 || (uint32_t)d >= TAKT_POSIX_MAX_MQS)
    {
        return nullptr;
    }
    return s_mqs[d].in_use ? &s_mqs[d] : nullptr;
}

static void mq_init_slot(MqSlot* slot, const char* name, int oflag,
                         uint32_t msgsz, uint32_t maxmsg)
{
    memset(slot, 0, sizeof(*slot));
    TaktOSSemInit(&slot->items,  0u,     maxmsg);
    TaktOSSemInit(&slot->spaces, maxmsg, maxmsg);

    for (uint32_t i = 0u; i < maxmsg; ++i)
    {
        slot->msgs[i].next = (i + 1u < maxmsg) ? (uint8_t)(i + 1u) : (uint8_t)MQ_NIL;
    }
    slot->freeHead = 0u;
    slot->head     = MQ_NIL;
    slot->tail     = MQ_NIL;
    slot->msgsize  = msgsz;
    slot->maxmsg   = maxmsg;
    slot->flags    = (oflag & O_NONBLOCK) ? O_NONBLOCK : 0;
    slot->in_use   = true;
    strncpy(slot->name, name, TAKT_POSIX_MQ_NAMEMAX - 1u);
    slot->name[TAKT_POSIX_MQ_NAMEMAX - 1u] = '\0';
}

// Link message idx into the priority-ordered list.  Critical section held.
static inline void mq_link(MqSlot* slot, uint8_t idx)
{
    MqMsg* msgs = slot->msgs;
    const unsigned int prio = msgs[idx].prio;

    if (slot->tail == MQ_NIL)
    {
        msgs[idx].next = MQ_NIL;
        slot->head = idx;
        slot->tail = idx;
        return;
    }

    if (msgs[slot->tail].prio >= prio)
    {
        msgs[idx].next = MQ_NIL;
        msgs[slot->tail].next = idx;
        slot->tail = idx;
        return;
    }

    // Higher than the tail: insert before the first node of lower priority.
    // One exists (at least the tail), so the walk never runs off the list.
    uint8_t* p = &slot->head;
    while (msgs[*p].prio >= prio)
    {
        p = &msgs[*p].next;
    }
    msgs[idx].next = *p;
    *p = idx;
}

static int mq_send_internal(MqSlot* slot, const char* msg, size_t len,
                             unsigned int prio, uint32_t ticks)
{
    if (len > slot->msgsize)
    {
        errno = EMSGSIZE;
        return -1;
    }
    if (sem_take_timed(&slot->spaces, ticks) != TAKTOS_OK)
    {
        errno = (ticks == TAKTOS_NO_WAIT) ? EAGAIN : ETIMEDOUT;
        return -1;
    }

    uint32_t state = TaktOSEnterCritical();
    const uint8_t idx = slot->freeHead;
    if (idx != MQ_NIL)
    {
        slot->freeHead = slot->msgs[idx].next;
    }
    TaktOSExitCritical(state);

    if (idx == MQ_NIL)
    {
        // Unreachable while the spaces invariant holds.
        TaktOSSemGive(&slot->spaces, false);
        errno = EAGAIN;
        return -1;
    }

    MqMsg& m = slot->msgs[idx];
    m.prio = prio;
    m.len  = (uint32_t)len;
    if (len > 0u)
    {
        memcpy(m.data, msg, len);
    }

    state = TaktOSEnterCritical();
    mq_link(slot, idx);
    slot->curmsgs++;
    TaktOSExitCritical(state);

    TaktOSSemGive(&slot->items, false);
    return 0;
}

static ssize_t mq_receive_internal(MqSlot* slot, char* buf, size_t len,
                                    unsigned int* prio, uint32_t ticks)
{
    if (len < slot->msgsize)
    {
        errno = EMSGSIZE;
        return -1;
    }
    if (sem_take_timed(&slot->items, ticks) != TAKTOS_OK)
    {
        errno = (ticks == TAKTOS_NO_WAIT) ? EAGAIN : ETIMEDOUT;
        return -1;
    }

    uint32_t state = TaktOSEnterCritical();
    const uint8_t idx = slot->head;
    if (idx != MQ_NIL)
    {
        slot->head = slot->msgs[idx].next;
        if (slot->head == MQ_NIL)
        {
            slot->tail = MQ_NIL;
        }
        slot->curmsgs--;
    }
    TaktOSExitCritical(state);

    if (idx == MQ_NIL)
    {
        // Unreachable while the items invariant holds.  Return the items
        // token that was taken (the old code returned a spaces token here).
        TaktOSSemGive(&slot->items, false);
        errno = EAGAIN;
        return -1;
    }

    MqMsg& m = slot->msgs[idx];
    if (prio)
    {
        *prio = m.prio;
    }
    const ssize_t out_len = (ssize_t)m.len;
    if (m.len > 0u)
    {
        memcpy(buf, m.data, m.len);
    }

    state = TaktOSEnterCritical();
    m.next = slot->freeHead;
    slot->freeHead = idx;
    TaktOSExitCritical(state);

    TaktOSSemGive(&slot->spaces, false);
    return out_len;
}

mqd_t mq_open(const char* name, int oflag, ...)
{
    if (!name)
    {
        errno = EINVAL;
        return MQ_FAILED;
    }
    MqSlot* existing = mq_find(name);

    if (oflag & O_CREAT)
    {
        if (existing)
        {
            if (oflag & O_EXCL)
            {
                errno = EEXIST;
                return MQ_FAILED;
            }
            if (oflag & O_NONBLOCK)
            {
                existing->flags |= O_NONBLOCK;
            }
            return static_cast<mqd_t>(existing - s_mqs);
        }
        uint32_t msgsz  = TAKT_POSIX_MQ_MAX_MSGSIZE;
        uint32_t maxmsg = TAKT_POSIX_MQ_MAX_MSG;
        va_list ap; va_start(ap, oflag);
        (void)va_arg(ap, int);
        const mq_attr* attr = va_arg(ap, const mq_attr*);
        va_end(ap);
        if (attr)
        {
            if (attr->mq_msgsize <= 0 ||
                attr->mq_msgsize > (long)TAKT_POSIX_MQ_MAX_MSGSIZE ||
                attr->mq_maxmsg  <= 0 ||
                attr->mq_maxmsg  > (long)TAKT_POSIX_MQ_MAX_MSG)
            {
                errno = EINVAL;
                return MQ_FAILED;
            }
            msgsz  = (uint32_t)attr->mq_msgsize;
            maxmsg = (uint32_t)attr->mq_maxmsg;
        }
        for (uint32_t i = 0u; i < TAKT_POSIX_MAX_MQS; ++i)
        {
            if (!s_mqs[i].in_use)
            {
                mq_init_slot(&s_mqs[i], name, oflag, msgsz, maxmsg);
                return static_cast<mqd_t>(i);
            }
        }
        errno = ENFILE; return MQ_FAILED;
    }
    if (!existing)
    {
        errno = ENOENT;
        return MQ_FAILED;
    }
    if (oflag & O_NONBLOCK)
    {
        existing->flags |= O_NONBLOCK;
    }
    return static_cast<mqd_t>(existing - s_mqs);
}

int mq_close(mqd_t) { return 0; }

int mq_unlink(const char* name)
{
    MqSlot* slot = mq_find(name);
    if (!slot)
    {
        errno = ENOENT;
        return -1;
    }
    slot->in_use = false;
    return 0;
}

int mq_send(mqd_t mqdes, const char* msg, size_t len, unsigned int prio)
{
    MqSlot* slot = mq_slot(mqdes);
    if (!slot || !msg)
    {
        errno = EINVAL;
        return -1;
    }
    uint32_t ticks = (slot->flags & O_NONBLOCK) ? TAKTOS_NO_WAIT : TAKTOS_WAIT_FOREVER;
    return mq_send_internal(slot, msg, len, prio, ticks);
}

int mq_timedsend(mqd_t mqdes, const char* msg, size_t len,
                 unsigned int prio, const struct timespec* abs)
{
    MqSlot* slot = mq_slot(mqdes);
    if (!slot || !msg || !abs)
    {
        errno = EINVAL;
        return -1;
    }
    return mq_send_internal(slot, msg, len, prio,
                            takt_timespec_to_ticks(abs, takt_posix_tick_hz()));
}

ssize_t mq_receive(mqd_t mqdes, char* buf, size_t len, unsigned int* prio)
{
    MqSlot* slot = mq_slot(mqdes);
    if (!slot || !buf)
    {
        errno = EINVAL;
        return -1;
    }
    uint32_t ticks = (slot->flags & O_NONBLOCK) ? TAKTOS_NO_WAIT : TAKTOS_WAIT_FOREVER;
    return mq_receive_internal(slot, buf, len, prio, ticks);
}

ssize_t mq_timedreceive(mqd_t mqdes, char* buf, size_t len,
                         unsigned int* prio, const struct timespec* abs)
{
    MqSlot* slot = mq_slot(mqdes);
    if (!slot || !buf || !abs)
    {
        errno = EINVAL;
        return -1;
    }
    return mq_receive_internal(slot, buf, len, prio,
                               takt_timespec_to_ticks(abs, takt_posix_tick_hz()));
}

int mq_getattr(mqd_t mqdes, struct mq_attr* attr)
{
    MqSlot* slot = mq_slot(mqdes);
    if (!slot || !attr)
    {
        errno = EINVAL;
        return -1;
    }
    attr->mq_flags   = slot->flags;
    attr->mq_maxmsg  = (long)slot->maxmsg;
    attr->mq_msgsize = (long)slot->msgsize;
    attr->mq_curmsgs = (long)slot->curmsgs;   // single aligned word read
    return 0;
}

int mq_setattr(mqd_t mqdes, const struct mq_attr* newattr, struct mq_attr* oldattr)
{
    MqSlot* slot = mq_slot(mqdes);
    if (!slot || !newattr)
    {
        errno = EINVAL;
        return -1;
    }
    if (oldattr)
    {
        mq_getattr(mqdes, oldattr);
    }
    slot->flags = (int)(newattr->mq_flags & O_NONBLOCK);
    return 0;
}

int mq_notify(mqd_t, const struct sigevent* sevp)
{
    if (sevp && sevp->sigev_notify == SIGEV_SIGNAL)
    {
        errno = ENOTSUP;
        return -1;
    }
    return 0;
}
