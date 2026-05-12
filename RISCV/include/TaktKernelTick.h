/**---------------------------------------------------------------------------
@file   TaktKernelTick.h

@brief  Generic RISC-V tick handler declaration for TaktOS

Declares TaktKernelTickHandler, the portable kernel's per-tick entry point.
Every RISC-V HAL that owns the system tick calls this from its tick ISR (or
from the trap dispatcher when the tick source is the platform timer).

This header is shared by the arch library and by every HAL.  No HAL-specific
macros are required; the arch library compiles standalone.
----------------------------------------------------------------------------*/
#ifndef __TAKTKERNELTICK_H__
#define __TAKTKERNELTICK_H__

#ifdef __cplusplus
extern "C" {
#endif

void TaktKernelTickHandler(void);

#ifdef __cplusplus
}
#endif

#endif // __TAKTKERNELTICK_H__
