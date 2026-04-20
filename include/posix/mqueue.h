/**---------------------------------------------------------------------------
@file   mqueue.h

@brief  TaktOS PSE51  POSIX message queues (mq_*)

Implements IEEE 1003.1-2017 PSE51 mq_open / mq_close / mq_unlink /
mq_send / mq_receive / mq_getattr / mq_setattr.

POSIX message queues differ from TaktOS Queue in two ways:
  1. Named: mq_open / mq_unlink use a string name stored in a flat table.
  2. Priority: messages carry a uint priority; mq_receive returns the
     highest-priority message first, with FIFO ordering within a priority.

Each named queue maps to a static TaktOS-backed slot.

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
#ifndef __MQUEUE_H__
#define __MQUEUE_H__

#include <stdint.h>

#include "posix_types.h"

#ifndef TAKT_POSIX_MAX_MQS
#  define TAKT_POSIX_MAX_MQS         8u
#endif
#ifndef TAKT_POSIX_MQ_MAX_MSG
#  define TAKT_POSIX_MQ_MAX_MSG      16u
#endif
#ifndef TAKT_POSIX_MQ_MAX_MSGSIZE
#  define TAKT_POSIX_MQ_MAX_MSGSIZE  64u
#endif
#ifndef TAKT_POSIX_MQ_NAMEMAX
#  define TAKT_POSIX_MQ_NAMEMAX      16u
#endif

typedef int  mqd_t;
#define MQ_FAILED  ((mqd_t)-1)

struct mq_attr {
    long mq_flags;    // O_NONBLOCK
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;  // read-only in mq_getattr
};

#ifndef O_RDONLY
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_EXCL      0x0080
#define O_NONBLOCK  0x0800
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief	Open or create a named message queue.
 *
 * @p name must begin with '/'.  The variadic arguments follow POSIX:
 * when O_CREAT is set they are (mode_t mode, struct mq_attr* attr)  mode is
 * ignored on bare-metal; attr sets mq_maxmsg and mq_msgsize.
 *
 * @param	name   : Queue name (e.g. "/myqueue").
 * @param	oflag  : O_RDONLY, O_WRONLY, or O_RDWR optionally OR'd with
 *                  O_CREAT, O_EXCL, and O_NONBLOCK.
 * @return	Message queue descriptor on success, MQ_FAILED on error
 *          (errno set to EINVAL, ENOENT, EEXIST, or EMFILE).
 */
mqd_t mq_open(const char* name, int oflag, ...);

/**
 * @brief	Close a message queue descriptor.
 *
 * Releases the descriptor; the queue itself persists until mq_unlink.
 *
 * @param	mqdes : Descriptor returned by mq_open.
 * @return	0 on success, -1 on error (EBADF).
 */
int mq_close(mqd_t mqdes);

/**
 * @brief	Remove a named message queue.
 *
 * The queue is deleted once all open descriptors are closed.
 *
 * @param	name : Queue name as passed to mq_open.
 * @return	0 on success, -1 on error (ENOENT).
 */
int mq_unlink(const char* name);

/**
 * @brief	Send a message to a queue.
 *
 * Blocks if the queue is full and O_NONBLOCK is not set.
 *
 * @param	mqdes     : Open message queue descriptor.
 * @param	msg_ptr  : Pointer to the message data.
 * @param	msg_len  : Length of the message in bytes (must be  mq_msgsize).
 * @param	msg_prio : Message priority (higher values are dequeued first).
 * @return	0 on success, -1 on error (EAGAIN if non-blocking and full,
 *          EMSGSIZE if msg_len > mq_msgsize, EBADF if invalid descriptor).
 */
int mq_send(mqd_t mqdes, const char* msg_ptr, size_t msg_len,
                        unsigned int msg_prio);

/**
 * @brief	Receive the highest-priority message from a queue.
 *
 * Blocks if the queue is empty and O_NONBLOCK is not set.
 *
 * @param	mqdes     : Open message queue descriptor.
 * @param	msg_ptr  : Buffer to receive the message.
 * @param	msg_len  : Size of the receive buffer (must be  mq_msgsize).
 * @param	msg_prio : If non-NULL, receives the message priority.
 * @return	Number of bytes received on success, -1 on error (EAGAIN if
 *          non-blocking and empty, EMSGSIZE if buffer too small, EBADF if invalid).
 */
ssize_t mq_receive(mqd_t mqdes, char* msg_ptr, size_t msg_len,
                        unsigned int* msg_prio);

/**
 * @brief	Send a message with an absolute timeout.
 *
 * Like mq_send but returns ETIMEDOUT if @p abs_timeout (CLOCK_MONOTONIC)
 * expires before space becomes available.
 *
 * @param	mqdes        : Open message queue descriptor.
 * @param	msg_ptr     : Pointer to the message data.
 * @param	msg_len     : Length of the message in bytes.
 * @param	msg_prio    : Message priority.
 * @param	abs_timeout : Absolute deadline as a timespec.
 * @return	0 on success, -1 on error (ETIMEDOUT on deadline expiry).
 */
int mq_timedsend(mqd_t mqdes, const char* msg_ptr, size_t msg_len,
                        unsigned int msg_prio, const struct timespec* abs_timeout);

/**
 * @brief	Receive the highest-priority message with an absolute timeout.
 *
 * Like mq_receive but returns ETIMEDOUT if @p abs_timeout (CLOCK_MONOTONIC)
 * expires before a message arrives.
 *
 * @param	mqdes        : Open message queue descriptor.
 * @param	msg_ptr     : Buffer to receive the message.
 * @param	msg_len     : Size of the receive buffer.
 * @param	msg_prio    : If non-NULL, receives the message priority.
 * @param	abs_timeout : Absolute deadline as a timespec.
 * @return	Number of bytes received on success, -1 on error (ETIMEDOUT on
 *          deadline expiry).
 */
ssize_t mq_timedreceive(mqd_t mqdes, char* msg_ptr, size_t msg_len,
                        unsigned int* msg_prio, const struct timespec* abs_timeout);

/**
 * @brief	Get message queue attributes.
 *
 * @param	mqdes  : Open message queue descriptor.
 * @param	attr   : Receives the current queue attributes.
 * @return	0 on success, -1 on error (EBADF).
 */
int mq_getattr(mqd_t mqdes, struct mq_attr* attr);

/**
 * @brief	Set message queue attributes (only O_NONBLOCK flag is writable).
 *
 * @param	mqdes    : Open message queue descriptor.
 * @param	newattr  : New attributes  only mq_flags (O_NONBLOCK) is honoured.
 * @param	oldattr  : If non-NULL, receives the previous attributes.
 * @return	0 on success, -1 on error (EBADF, EINVAL).
 */
int mq_setattr(mqd_t mqdes, const struct mq_attr* newattr,
                        struct mq_attr* oldattr);

/**
 * @brief	Request asynchronous notification when a message arrives  stub.
 *
 * SIGEV_SIGNAL is not supported on bare-metal.  Returns ENOTSUP.
 *
 * @param	mqdes  : Open message queue descriptor.
 * @param	sevp   : Notification request (ignored).
 * @return	-1 with errno set to ENOTSUP.
 */
int mq_notify(mqd_t mqdes, const struct sigevent* sevp);

#ifdef __cplusplus
}
#endif

#endif // __MQUEUE_H__
