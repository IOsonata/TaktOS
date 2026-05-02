/**
 * @file    tm_port_common.h
 * @brief   Thread-Metric port constants shared by every RTOS port.
 *
 * Every port file (tm_port_taktos.cpp, tm_port_freertos.c, tm_port_threadx.c)
 * uses THESE values. Discrepancies here are the biggest source of
 * apples-vs-oranges benchmark results, so the same sizing is applied
 * uniformly to every MCU and every RTOS in this benchmark suite.
 *
 * Sizing rationale
 * ----------------
 * The values below are sized for the actual Thread-Metric workload, not for
 * the largest MCU. The seven tests built here have shallow call depth and
 * use at most 8 thread slots in the worst case (MutexBargingTest), so
 * sizing the static port arrays to those actual needs:
 *
 *   - lets the same numerical-result set be produced on every supported MCU,
 *     including small-RAM Cortex-M0 targets like the STM32F030R8 (8 KB SRAM);
 *   - keeps every cross-RTOS comparison like-for-like (FreeRTOS / ThreadX /
 *     TaktOS all link the same byte budget);
 *   - keeps every cross-MCU comparison like-for-like (every MCU runs the
 *     same byte budget, regardless of how much RAM it has spare).
 *
 * Each value is wrapped in #ifndef/#endif so a future target with even
 * tighter constraints can still override individual sizes via -D flags
 * in its .cproject. Any such override MUST be applied identically to every
 * RTOS variant on that target so the cross-RTOS benchmark stays valid;
 * the resulting cross-MCU comparison against targets using the global
 * defaults must be noted in the report.
 *
 * Per-MCU parameters that are not benchmark sizes (UART pins, core clock,
 * SW IRQ number, IRQ vector handler name) live in the target's board.h.
 */
#ifndef TM_PORT_COMMON_H_
#define TM_PORT_COMMON_H_

/* Worst case across the seven tests: MutexBargingTest creates 9 threads
 * with slot ids 0..8: four workers, four interlopers, and one reporter.
 * CooperativeScheduling / PreemptiveScheduling use 6, MutexProcessing uses
 * 5, the others use 2. Sizing for the worst case lets every project link
 * unmodified and prevents the reporter thread descriptor from corrupting
 * adjacent port globals. */
#ifndef TM_PORT_MAX_THREADS
#define TM_PORT_MAX_THREADS          9
#endif

/* TM reporter threads call tm_printf()/UARTvprintf() from thread context.
 * 384 bytes is not enough on the C++/IOsonata UART path and can overflow
 * into the following TCB/run-queue metadata. 1024 bytes is the shared fair
 * benchmark stack budget used across the RTOS ports. */
#ifndef TM_PORT_STACK_BYTES
#define TM_PORT_STACK_BYTES          1024u
#endif

#ifndef TM_PORT_TICK_HZ
#define TM_PORT_TICK_HZ              1000u    /* 1 ms  required for TM4 */
#endif

#ifndef TM_PORT_MAX_QUEUES
#define TM_PORT_MAX_QUEUES           1
#endif
#ifndef TM_PORT_QUEUE_DEPTH
#define TM_PORT_QUEUE_DEPTH          10u
#endif
#ifndef TM_PORT_QUEUE_MSG_BYTES
#define TM_PORT_QUEUE_MSG_BYTES      16u
#endif

#ifndef TM_PORT_MAX_SEMAPHORES
#define TM_PORT_MAX_SEMAPHORES       1
#endif
#ifndef TM_PORT_MAX_MUTEXES
#define TM_PORT_MAX_MUTEXES          1
#endif

/* TM5 (memory allocation) is excluded from the suite by every RTOS port
 * built here (TaktOS has no heap; the FreeRTOS / ThreadX projects don't
 * link memory_processing.c either). The pool array is kept minimal so
 * the type compiles. Raise this if a TM5 project is ever added. */
#ifndef TM_PORT_MAX_POOLS
#define TM_PORT_MAX_POOLS            1
#endif
#ifndef TM_PORT_POOL_BYTES
#define TM_PORT_POOL_BYTES           256u
#endif
#ifndef TM_PORT_BLOCK_BYTES
#define TM_PORT_BLOCK_BYTES          128u
#endif
#ifndef TM_PORT_BLOCK_COUNT
#define TM_PORT_BLOCK_COUNT          (TM_PORT_POOL_BYTES / TM_PORT_BLOCK_BYTES)
#endif

#endif /* TM_PORT_COMMON_H_ */
