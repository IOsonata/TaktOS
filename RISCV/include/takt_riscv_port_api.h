/**---------------------------------------------------------------------------
@file   takt_riscv_port_api.h

@brief  Generic RISC-V target-port API for TaktOS

This header defines the small set of target-port macros that the shared
RISC-V architecture headers expect.  TaktOS does not pull in a vendor HAL
here.  Only the kernel-facing hooks required by the scheduler are exposed:

- thread stack/frame sizing
- deferred context-switch trigger
- optional first-task launcher declaration
- optional tick-handler declaration when the target trap path calls it by name

Each target include wrapper must define the macros below before including the
shared headers from RISCV/include/.

Required macros:
  TAKT_RISCV_STACK_GUARD_ALIGN
  TAKT_RISCV_THREAD_INIT_FRAME_SIZE
  TAKT_RISCV_THREAD_INIT_ALIGN_SLACK
  TAKT_RISCV_PORT_CTX_SWITCH_TRIGGER()

Optional macros:
  TAKT_RISCV_PORT_DECLARE_START_FIRST      default: 1
  TAKT_RISCV_PORT_DECLARE_TICK_HANDLER     default: 0
----------------------------------------------------------------------------*/
#ifndef __TAKT_RISCV_PORT_CONTRACT_H__
#define __TAKT_RISCV_PORT_CONTRACT_H__

#ifndef TAKT_RISCV_STACK_GUARD_ALIGN
#  error "TAKT_RISCV_STACK_GUARD_ALIGN must be defined by the target port"
#endif

#ifndef TAKT_RISCV_THREAD_INIT_FRAME_SIZE
#  error "TAKT_RISCV_THREAD_INIT_FRAME_SIZE must be defined by the target port"
#endif

#ifndef TAKT_RISCV_THREAD_INIT_ALIGN_SLACK
#  error "TAKT_RISCV_THREAD_INIT_ALIGN_SLACK must be defined by the target port"
#endif

#ifndef TAKT_RISCV_PORT_CTX_SWITCH_TRIGGER
#  error "TAKT_RISCV_PORT_CTX_SWITCH_TRIGGER() must be defined by the target port"
#endif

#ifndef TAKT_RISCV_PORT_DECLARE_START_FIRST
#  define TAKT_RISCV_PORT_DECLARE_START_FIRST 1
#endif

#ifndef TAKT_RISCV_PORT_DECLARE_TICK_HANDLER
#  define TAKT_RISCV_PORT_DECLARE_TICK_HANDLER 0
#endif

#endif // __TAKT_RISCV_PORT_CONTRACT_H__
