/*
 * Copyright (c) 2024 Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#ifndef TM_API_H
#define TM_API_H

#ifdef __cplusplus
extern "C" {
#endif

#define TM_SUCCESS 0
#define TM_ERROR 1

#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif

#ifndef TM_TEST_CYCLES
#define TM_TEST_CYCLES 0
#endif

extern int tm_test_duration;
extern int tm_test_cycles;

void tm_report_init(void);
void tm_report_init_argv(int argc, char **argv);
void tm_report_finish(void);
void tm_check_fail(const char *msg);
void tm_putchar(int c);
void tm_printf(const char *fmt, ...);

#define TM_REPORT_LOOP                                                     \
    {                                                                      \
        int _tm_cycle;                                                     \
        for (_tm_cycle = 0; !tm_test_cycles || _tm_cycle < tm_test_cycles; \
             tm_test_cycles ? _tm_cycle++ : 0)

#define TM_REPORT_FINISH \
    }                    \
    tm_report_finish()

#define TM_CHECK(call)                                  \
    do {                                                \
        if ((call) != TM_SUCCESS)                       \
            tm_check_fail("FATAL: " #call " failed\n"); \
    } while (0)

void tm_initialize(void (*test_initialization_function)(void));
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void));
int tm_thread_resume(int thread_id);
int tm_thread_suspend(int thread_id);
void tm_thread_relinquish(void);
void tm_thread_sleep(int seconds);
int tm_queue_create(int queue_id);
int tm_queue_send(int queue_id, unsigned long *message_ptr);
int tm_queue_receive(int queue_id, unsigned long *message_ptr);
int tm_semaphore_create(int semaphore_id);
int tm_semaphore_get(int semaphore_id);
int tm_semaphore_put(int semaphore_id);
int tm_mutex_create(int mutex_id);
int tm_mutex_lock(int mutex_id);
int tm_mutex_unlock(int mutex_id);
int tm_memory_pool_create(int pool_id);
int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr);
int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr);
void tm_cause_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif
