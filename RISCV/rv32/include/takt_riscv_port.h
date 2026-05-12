#ifndef __TAKT_RISCV_RV32_PORT_H__
#define __TAKT_RISCV_RV32_PORT_H__

#include "../port_rv32.h"

#ifndef TAKT_CLINT_BASE
#  error "TAKT_CLINT_BASE not defined - set -DTAKT_CLINT_BASE=<addr> in project build settings"
#endif

#define TAKT_RISCV_STACK_GUARD_ALIGN       32u
#define TAKT_RISCV_THREAD_INIT_FRAME_SIZE  CTX_FRAME_BYTES
#define TAKT_RISCV_THREAD_INIT_ALIGN_SLACK 15u

#define TAKT_RISCV_PORT_DECLARE_START_FIRST 1
#define TAKT_RISCV_PORT_DECLARE_TICK_HANDLER 0

#define TAKT_RISCV_PORT_CTX_SWITCH_TRIGGER() \
    (*((volatile uint32_t*)TAKT_CLINT_BASE) = 1u)

#endif // __TAKT_RISCV_RV32_PORT_H__
