/**
 * @file    tm_port_common.h
 * @brief   Thread-Metric port constants shared by every RTOS port.
 *
 * Every port file (tm_port_taktos.cpp, tm_port_freertos.c, tm_port_threadx.c)
 * uses THESE values. Discrepancies here are the biggest source of
 * apples-vs-oranges benchmark results.
 *
 * Per-MCU parameters (UART pins, core clock, SW IRQ number, IRQ vector
 * handler name) live in the target's board.h.
 */
#ifndef TM_PORT_COMMON_H_
#define TM_PORT_COMMON_H_

#define TM_PORT_MAX_THREADS          10
#define TM_PORT_STACK_BYTES          1024u

#define TM_PORT_TICK_HZ              1000u    /* 1 ms  required for TM4 */

#define TM_PORT_MAX_QUEUES           1
#define TM_PORT_QUEUE_DEPTH          10u
#define TM_PORT_QUEUE_MSG_BYTES      16u

#define TM_PORT_MAX_SEMAPHORES       1
#define TM_PORT_MAX_MUTEXES          1

#define TM_PORT_MAX_POOLS            1
#define TM_PORT_POOL_BYTES           2048u
#define TM_PORT_BLOCK_BYTES          128u
#define TM_PORT_BLOCK_COUNT          (TM_PORT_POOL_BYTES / TM_PORT_BLOCK_BYTES)

#endif /* TM_PORT_COMMON_H_ */
