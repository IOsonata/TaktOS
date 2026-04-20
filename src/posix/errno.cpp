/**---------------------------------------------------------------------------
@file   errno.cpp

@brief  TaktOS PSE51  per-task errno storage

Maintains a per-task errno value with a global fallback used before the
scheduler is running.

TaktOSThread_t is the cert-boundary TCB and must not grow for QM purposes.
A parallel lookup table indexed by thread pointer is used instead, sized
to TAKT_POSIX_MAX_THREADS (default 16).

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
#include "posix/posix_types.h"
#include "TaktOSThread.h"

// Per-task errno storage.
// TaktOSThread_t is the cert-boundary TCB  we do not add fields to it.
// Instead, keep a parallel table indexed by thread pointer.
// Maximum entries = TAKT_POSIX_MAX_THREADS, same pool as pthread.cpp.

#ifndef TAKT_POSIX_MAX_THREADS
#  define TAKT_POSIX_MAX_THREADS  16u
#endif

struct ErrnoEntry {
    TaktOSThread_t *thread;
    int             value;
};

static ErrnoEntry s_errno_table[TAKT_POSIX_MAX_THREADS];
static int        s_global_errno = 0;

int* takt_posix_errno_location(void)
{
    TaktOSThread_t* cur = TaktOSCurrentThread();
    if (cur == nullptr)
    {
        return &s_global_errno;
    }
    // Find existing entry.
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (s_errno_table[i].thread == cur)
        {
            return &s_errno_table[i].value;
        }
    }
    // Allocate a new entry for this thread.
    for (uint32_t i = 0u; i < TAKT_POSIX_MAX_THREADS; ++i)
    {
        if (s_errno_table[i].thread == nullptr)
        {
            s_errno_table[i].thread = cur;
            s_errno_table[i].value  = 0;
            return &s_errno_table[i].value;
        }
    }
    // Table full  fall back to global (should not happen in a well-sized system).
    return &s_global_errno;
}
