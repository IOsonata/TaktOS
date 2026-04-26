/**---------------------------------------------------------------------------
@file   takt_cpp_runtime.h

@brief  TaktOS C++ runtime hook declarations

The implementation supplies weak freestanding runtime hooks for newlib malloc
locking and C++ local-static guard locking.  These hooks are intentionally
outside the kernel safety boundary and can be replaced by an application or by
a future Partitura-backed runtime allocator.

QM - outside cert boundary.
----------------------------------------------------------------------------*/
#ifndef __TAKT_CPP_RUNTIME_H__
#define __TAKT_CPP_RUNTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

void TaktCppRuntimeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __TAKT_CPP_RUNTIME_H__ */
