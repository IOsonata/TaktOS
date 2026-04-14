################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_allocate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_cleanup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_block_release.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_allocate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_cleanup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_search.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_release.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_cleanup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_set.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_set_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_initialize_high_level.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_initialize_kernel_enter.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_initialize_kernel_setup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_misra.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_cleanup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_priority_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_put.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_cleanup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_flush.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_front_send.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_receive.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_send.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_send_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_ceiling_put.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_cleanup.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_put.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_put_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_entry_exit_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_identify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_preemption_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_priority_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_relinquish.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_reset.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_resume.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_shell_entry.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_sleep.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_stack_analyze.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_stack_error_handler.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_stack_error_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_suspend.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_system_preempt_check.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_system_resume.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_system_suspend.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_terminate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_time_slice.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_time_slice_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_timeout.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_wait_abort.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_time_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_time_set.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_activate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_deactivate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_expiration_process.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_performance_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_performance_system_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_system_activate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_system_deactivate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_thread_entry.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_buffer_full_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_disable.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_enable.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_event_filter.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_event_unfilter.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_initialize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_interrupt_control.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_isr_enter_insert.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_isr_exit_insert.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_object_register.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_object_unregister.c \
/Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_user_event_insert.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_block_allocate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_block_release.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_allocate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_release.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_set.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_set_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_put.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_flush.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_front_send.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_receive.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_send.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_send_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_ceiling_put.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_prioritize.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_put.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_put_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_entry_exit_notify.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_info_get.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_preemption_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_priority_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_relinquish.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_reset.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_resume.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_suspend.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_terminate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_time_slice_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_wait_abort.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_activate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_change.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_create.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_deactivate.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_delete.c \
/Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_info_get.c 

C_DEPS += \
./threadx_common_src/src/tx_block_allocate.d \
./threadx_common_src/src/tx_block_pool_cleanup.d \
./threadx_common_src/src/tx_block_pool_create.d \
./threadx_common_src/src/tx_block_pool_delete.d \
./threadx_common_src/src/tx_block_pool_info_get.d \
./threadx_common_src/src/tx_block_pool_initialize.d \
./threadx_common_src/src/tx_block_pool_performance_info_get.d \
./threadx_common_src/src/tx_block_pool_performance_system_info_get.d \
./threadx_common_src/src/tx_block_pool_prioritize.d \
./threadx_common_src/src/tx_block_release.d \
./threadx_common_src/src/tx_byte_allocate.d \
./threadx_common_src/src/tx_byte_pool_cleanup.d \
./threadx_common_src/src/tx_byte_pool_create.d \
./threadx_common_src/src/tx_byte_pool_delete.d \
./threadx_common_src/src/tx_byte_pool_info_get.d \
./threadx_common_src/src/tx_byte_pool_initialize.d \
./threadx_common_src/src/tx_byte_pool_performance_info_get.d \
./threadx_common_src/src/tx_byte_pool_performance_system_info_get.d \
./threadx_common_src/src/tx_byte_pool_prioritize.d \
./threadx_common_src/src/tx_byte_pool_search.d \
./threadx_common_src/src/tx_byte_release.d \
./threadx_common_src/src/tx_event_flags_cleanup.d \
./threadx_common_src/src/tx_event_flags_create.d \
./threadx_common_src/src/tx_event_flags_delete.d \
./threadx_common_src/src/tx_event_flags_get.d \
./threadx_common_src/src/tx_event_flags_info_get.d \
./threadx_common_src/src/tx_event_flags_initialize.d \
./threadx_common_src/src/tx_event_flags_performance_info_get.d \
./threadx_common_src/src/tx_event_flags_performance_system_info_get.d \
./threadx_common_src/src/tx_event_flags_set.d \
./threadx_common_src/src/tx_event_flags_set_notify.d \
./threadx_common_src/src/tx_initialize_high_level.d \
./threadx_common_src/src/tx_initialize_kernel_enter.d \
./threadx_common_src/src/tx_initialize_kernel_setup.d \
./threadx_common_src/src/tx_misra.d \
./threadx_common_src/src/tx_mutex_cleanup.d \
./threadx_common_src/src/tx_mutex_create.d \
./threadx_common_src/src/tx_mutex_delete.d \
./threadx_common_src/src/tx_mutex_get.d \
./threadx_common_src/src/tx_mutex_info_get.d \
./threadx_common_src/src/tx_mutex_initialize.d \
./threadx_common_src/src/tx_mutex_performance_info_get.d \
./threadx_common_src/src/tx_mutex_performance_system_info_get.d \
./threadx_common_src/src/tx_mutex_prioritize.d \
./threadx_common_src/src/tx_mutex_priority_change.d \
./threadx_common_src/src/tx_mutex_put.d \
./threadx_common_src/src/tx_queue_cleanup.d \
./threadx_common_src/src/tx_queue_create.d \
./threadx_common_src/src/tx_queue_delete.d \
./threadx_common_src/src/tx_queue_flush.d \
./threadx_common_src/src/tx_queue_front_send.d \
./threadx_common_src/src/tx_queue_info_get.d \
./threadx_common_src/src/tx_queue_initialize.d \
./threadx_common_src/src/tx_queue_performance_info_get.d \
./threadx_common_src/src/tx_queue_performance_system_info_get.d \
./threadx_common_src/src/tx_queue_prioritize.d \
./threadx_common_src/src/tx_queue_receive.d \
./threadx_common_src/src/tx_queue_send.d \
./threadx_common_src/src/tx_queue_send_notify.d \
./threadx_common_src/src/tx_semaphore_ceiling_put.d \
./threadx_common_src/src/tx_semaphore_cleanup.d \
./threadx_common_src/src/tx_semaphore_create.d \
./threadx_common_src/src/tx_semaphore_delete.d \
./threadx_common_src/src/tx_semaphore_get.d \
./threadx_common_src/src/tx_semaphore_info_get.d \
./threadx_common_src/src/tx_semaphore_initialize.d \
./threadx_common_src/src/tx_semaphore_performance_info_get.d \
./threadx_common_src/src/tx_semaphore_performance_system_info_get.d \
./threadx_common_src/src/tx_semaphore_prioritize.d \
./threadx_common_src/src/tx_semaphore_put.d \
./threadx_common_src/src/tx_semaphore_put_notify.d \
./threadx_common_src/src/tx_thread_create.d \
./threadx_common_src/src/tx_thread_delete.d \
./threadx_common_src/src/tx_thread_entry_exit_notify.d \
./threadx_common_src/src/tx_thread_identify.d \
./threadx_common_src/src/tx_thread_info_get.d \
./threadx_common_src/src/tx_thread_initialize.d \
./threadx_common_src/src/tx_thread_performance_info_get.d \
./threadx_common_src/src/tx_thread_performance_system_info_get.d \
./threadx_common_src/src/tx_thread_preemption_change.d \
./threadx_common_src/src/tx_thread_priority_change.d \
./threadx_common_src/src/tx_thread_relinquish.d \
./threadx_common_src/src/tx_thread_reset.d \
./threadx_common_src/src/tx_thread_resume.d \
./threadx_common_src/src/tx_thread_shell_entry.d \
./threadx_common_src/src/tx_thread_sleep.d \
./threadx_common_src/src/tx_thread_stack_analyze.d \
./threadx_common_src/src/tx_thread_stack_error_handler.d \
./threadx_common_src/src/tx_thread_stack_error_notify.d \
./threadx_common_src/src/tx_thread_suspend.d \
./threadx_common_src/src/tx_thread_system_preempt_check.d \
./threadx_common_src/src/tx_thread_system_resume.d \
./threadx_common_src/src/tx_thread_system_suspend.d \
./threadx_common_src/src/tx_thread_terminate.d \
./threadx_common_src/src/tx_thread_time_slice.d \
./threadx_common_src/src/tx_thread_time_slice_change.d \
./threadx_common_src/src/tx_thread_timeout.d \
./threadx_common_src/src/tx_thread_wait_abort.d \
./threadx_common_src/src/tx_time_get.d \
./threadx_common_src/src/tx_time_set.d \
./threadx_common_src/src/tx_timer_activate.d \
./threadx_common_src/src/tx_timer_change.d \
./threadx_common_src/src/tx_timer_create.d \
./threadx_common_src/src/tx_timer_deactivate.d \
./threadx_common_src/src/tx_timer_delete.d \
./threadx_common_src/src/tx_timer_expiration_process.d \
./threadx_common_src/src/tx_timer_info_get.d \
./threadx_common_src/src/tx_timer_initialize.d \
./threadx_common_src/src/tx_timer_performance_info_get.d \
./threadx_common_src/src/tx_timer_performance_system_info_get.d \
./threadx_common_src/src/tx_timer_system_activate.d \
./threadx_common_src/src/tx_timer_system_deactivate.d \
./threadx_common_src/src/tx_timer_thread_entry.d \
./threadx_common_src/src/tx_trace_buffer_full_notify.d \
./threadx_common_src/src/tx_trace_disable.d \
./threadx_common_src/src/tx_trace_enable.d \
./threadx_common_src/src/tx_trace_event_filter.d \
./threadx_common_src/src/tx_trace_event_unfilter.d \
./threadx_common_src/src/tx_trace_initialize.d \
./threadx_common_src/src/tx_trace_interrupt_control.d \
./threadx_common_src/src/tx_trace_isr_enter_insert.d \
./threadx_common_src/src/tx_trace_isr_exit_insert.d \
./threadx_common_src/src/tx_trace_object_register.d \
./threadx_common_src/src/tx_trace_object_unregister.d \
./threadx_common_src/src/tx_trace_user_event_insert.d \
./threadx_common_src/src/txe_block_allocate.d \
./threadx_common_src/src/txe_block_pool_create.d \
./threadx_common_src/src/txe_block_pool_delete.d \
./threadx_common_src/src/txe_block_pool_info_get.d \
./threadx_common_src/src/txe_block_pool_prioritize.d \
./threadx_common_src/src/txe_block_release.d \
./threadx_common_src/src/txe_byte_allocate.d \
./threadx_common_src/src/txe_byte_pool_create.d \
./threadx_common_src/src/txe_byte_pool_delete.d \
./threadx_common_src/src/txe_byte_pool_info_get.d \
./threadx_common_src/src/txe_byte_pool_prioritize.d \
./threadx_common_src/src/txe_byte_release.d \
./threadx_common_src/src/txe_event_flags_create.d \
./threadx_common_src/src/txe_event_flags_delete.d \
./threadx_common_src/src/txe_event_flags_get.d \
./threadx_common_src/src/txe_event_flags_info_get.d \
./threadx_common_src/src/txe_event_flags_set.d \
./threadx_common_src/src/txe_event_flags_set_notify.d \
./threadx_common_src/src/txe_mutex_create.d \
./threadx_common_src/src/txe_mutex_delete.d \
./threadx_common_src/src/txe_mutex_get.d \
./threadx_common_src/src/txe_mutex_info_get.d \
./threadx_common_src/src/txe_mutex_prioritize.d \
./threadx_common_src/src/txe_mutex_put.d \
./threadx_common_src/src/txe_queue_create.d \
./threadx_common_src/src/txe_queue_delete.d \
./threadx_common_src/src/txe_queue_flush.d \
./threadx_common_src/src/txe_queue_front_send.d \
./threadx_common_src/src/txe_queue_info_get.d \
./threadx_common_src/src/txe_queue_prioritize.d \
./threadx_common_src/src/txe_queue_receive.d \
./threadx_common_src/src/txe_queue_send.d \
./threadx_common_src/src/txe_queue_send_notify.d \
./threadx_common_src/src/txe_semaphore_ceiling_put.d \
./threadx_common_src/src/txe_semaphore_create.d \
./threadx_common_src/src/txe_semaphore_delete.d \
./threadx_common_src/src/txe_semaphore_get.d \
./threadx_common_src/src/txe_semaphore_info_get.d \
./threadx_common_src/src/txe_semaphore_prioritize.d \
./threadx_common_src/src/txe_semaphore_put.d \
./threadx_common_src/src/txe_semaphore_put_notify.d \
./threadx_common_src/src/txe_thread_create.d \
./threadx_common_src/src/txe_thread_delete.d \
./threadx_common_src/src/txe_thread_entry_exit_notify.d \
./threadx_common_src/src/txe_thread_info_get.d \
./threadx_common_src/src/txe_thread_preemption_change.d \
./threadx_common_src/src/txe_thread_priority_change.d \
./threadx_common_src/src/txe_thread_relinquish.d \
./threadx_common_src/src/txe_thread_reset.d \
./threadx_common_src/src/txe_thread_resume.d \
./threadx_common_src/src/txe_thread_suspend.d \
./threadx_common_src/src/txe_thread_terminate.d \
./threadx_common_src/src/txe_thread_time_slice_change.d \
./threadx_common_src/src/txe_thread_wait_abort.d \
./threadx_common_src/src/txe_timer_activate.d \
./threadx_common_src/src/txe_timer_change.d \
./threadx_common_src/src/txe_timer_create.d \
./threadx_common_src/src/txe_timer_deactivate.d \
./threadx_common_src/src/txe_timer_delete.d \
./threadx_common_src/src/txe_timer_info_get.d 

OBJS += \
./threadx_common_src/src/tx_block_allocate.o \
./threadx_common_src/src/tx_block_pool_cleanup.o \
./threadx_common_src/src/tx_block_pool_create.o \
./threadx_common_src/src/tx_block_pool_delete.o \
./threadx_common_src/src/tx_block_pool_info_get.o \
./threadx_common_src/src/tx_block_pool_initialize.o \
./threadx_common_src/src/tx_block_pool_performance_info_get.o \
./threadx_common_src/src/tx_block_pool_performance_system_info_get.o \
./threadx_common_src/src/tx_block_pool_prioritize.o \
./threadx_common_src/src/tx_block_release.o \
./threadx_common_src/src/tx_byte_allocate.o \
./threadx_common_src/src/tx_byte_pool_cleanup.o \
./threadx_common_src/src/tx_byte_pool_create.o \
./threadx_common_src/src/tx_byte_pool_delete.o \
./threadx_common_src/src/tx_byte_pool_info_get.o \
./threadx_common_src/src/tx_byte_pool_initialize.o \
./threadx_common_src/src/tx_byte_pool_performance_info_get.o \
./threadx_common_src/src/tx_byte_pool_performance_system_info_get.o \
./threadx_common_src/src/tx_byte_pool_prioritize.o \
./threadx_common_src/src/tx_byte_pool_search.o \
./threadx_common_src/src/tx_byte_release.o \
./threadx_common_src/src/tx_event_flags_cleanup.o \
./threadx_common_src/src/tx_event_flags_create.o \
./threadx_common_src/src/tx_event_flags_delete.o \
./threadx_common_src/src/tx_event_flags_get.o \
./threadx_common_src/src/tx_event_flags_info_get.o \
./threadx_common_src/src/tx_event_flags_initialize.o \
./threadx_common_src/src/tx_event_flags_performance_info_get.o \
./threadx_common_src/src/tx_event_flags_performance_system_info_get.o \
./threadx_common_src/src/tx_event_flags_set.o \
./threadx_common_src/src/tx_event_flags_set_notify.o \
./threadx_common_src/src/tx_initialize_high_level.o \
./threadx_common_src/src/tx_initialize_kernel_enter.o \
./threadx_common_src/src/tx_initialize_kernel_setup.o \
./threadx_common_src/src/tx_misra.o \
./threadx_common_src/src/tx_mutex_cleanup.o \
./threadx_common_src/src/tx_mutex_create.o \
./threadx_common_src/src/tx_mutex_delete.o \
./threadx_common_src/src/tx_mutex_get.o \
./threadx_common_src/src/tx_mutex_info_get.o \
./threadx_common_src/src/tx_mutex_initialize.o \
./threadx_common_src/src/tx_mutex_performance_info_get.o \
./threadx_common_src/src/tx_mutex_performance_system_info_get.o \
./threadx_common_src/src/tx_mutex_prioritize.o \
./threadx_common_src/src/tx_mutex_priority_change.o \
./threadx_common_src/src/tx_mutex_put.o \
./threadx_common_src/src/tx_queue_cleanup.o \
./threadx_common_src/src/tx_queue_create.o \
./threadx_common_src/src/tx_queue_delete.o \
./threadx_common_src/src/tx_queue_flush.o \
./threadx_common_src/src/tx_queue_front_send.o \
./threadx_common_src/src/tx_queue_info_get.o \
./threadx_common_src/src/tx_queue_initialize.o \
./threadx_common_src/src/tx_queue_performance_info_get.o \
./threadx_common_src/src/tx_queue_performance_system_info_get.o \
./threadx_common_src/src/tx_queue_prioritize.o \
./threadx_common_src/src/tx_queue_receive.o \
./threadx_common_src/src/tx_queue_send.o \
./threadx_common_src/src/tx_queue_send_notify.o \
./threadx_common_src/src/tx_semaphore_ceiling_put.o \
./threadx_common_src/src/tx_semaphore_cleanup.o \
./threadx_common_src/src/tx_semaphore_create.o \
./threadx_common_src/src/tx_semaphore_delete.o \
./threadx_common_src/src/tx_semaphore_get.o \
./threadx_common_src/src/tx_semaphore_info_get.o \
./threadx_common_src/src/tx_semaphore_initialize.o \
./threadx_common_src/src/tx_semaphore_performance_info_get.o \
./threadx_common_src/src/tx_semaphore_performance_system_info_get.o \
./threadx_common_src/src/tx_semaphore_prioritize.o \
./threadx_common_src/src/tx_semaphore_put.o \
./threadx_common_src/src/tx_semaphore_put_notify.o \
./threadx_common_src/src/tx_thread_create.o \
./threadx_common_src/src/tx_thread_delete.o \
./threadx_common_src/src/tx_thread_entry_exit_notify.o \
./threadx_common_src/src/tx_thread_identify.o \
./threadx_common_src/src/tx_thread_info_get.o \
./threadx_common_src/src/tx_thread_initialize.o \
./threadx_common_src/src/tx_thread_performance_info_get.o \
./threadx_common_src/src/tx_thread_performance_system_info_get.o \
./threadx_common_src/src/tx_thread_preemption_change.o \
./threadx_common_src/src/tx_thread_priority_change.o \
./threadx_common_src/src/tx_thread_relinquish.o \
./threadx_common_src/src/tx_thread_reset.o \
./threadx_common_src/src/tx_thread_resume.o \
./threadx_common_src/src/tx_thread_shell_entry.o \
./threadx_common_src/src/tx_thread_sleep.o \
./threadx_common_src/src/tx_thread_stack_analyze.o \
./threadx_common_src/src/tx_thread_stack_error_handler.o \
./threadx_common_src/src/tx_thread_stack_error_notify.o \
./threadx_common_src/src/tx_thread_suspend.o \
./threadx_common_src/src/tx_thread_system_preempt_check.o \
./threadx_common_src/src/tx_thread_system_resume.o \
./threadx_common_src/src/tx_thread_system_suspend.o \
./threadx_common_src/src/tx_thread_terminate.o \
./threadx_common_src/src/tx_thread_time_slice.o \
./threadx_common_src/src/tx_thread_time_slice_change.o \
./threadx_common_src/src/tx_thread_timeout.o \
./threadx_common_src/src/tx_thread_wait_abort.o \
./threadx_common_src/src/tx_time_get.o \
./threadx_common_src/src/tx_time_set.o \
./threadx_common_src/src/tx_timer_activate.o \
./threadx_common_src/src/tx_timer_change.o \
./threadx_common_src/src/tx_timer_create.o \
./threadx_common_src/src/tx_timer_deactivate.o \
./threadx_common_src/src/tx_timer_delete.o \
./threadx_common_src/src/tx_timer_expiration_process.o \
./threadx_common_src/src/tx_timer_info_get.o \
./threadx_common_src/src/tx_timer_initialize.o \
./threadx_common_src/src/tx_timer_performance_info_get.o \
./threadx_common_src/src/tx_timer_performance_system_info_get.o \
./threadx_common_src/src/tx_timer_system_activate.o \
./threadx_common_src/src/tx_timer_system_deactivate.o \
./threadx_common_src/src/tx_timer_thread_entry.o \
./threadx_common_src/src/tx_trace_buffer_full_notify.o \
./threadx_common_src/src/tx_trace_disable.o \
./threadx_common_src/src/tx_trace_enable.o \
./threadx_common_src/src/tx_trace_event_filter.o \
./threadx_common_src/src/tx_trace_event_unfilter.o \
./threadx_common_src/src/tx_trace_initialize.o \
./threadx_common_src/src/tx_trace_interrupt_control.o \
./threadx_common_src/src/tx_trace_isr_enter_insert.o \
./threadx_common_src/src/tx_trace_isr_exit_insert.o \
./threadx_common_src/src/tx_trace_object_register.o \
./threadx_common_src/src/tx_trace_object_unregister.o \
./threadx_common_src/src/tx_trace_user_event_insert.o \
./threadx_common_src/src/txe_block_allocate.o \
./threadx_common_src/src/txe_block_pool_create.o \
./threadx_common_src/src/txe_block_pool_delete.o \
./threadx_common_src/src/txe_block_pool_info_get.o \
./threadx_common_src/src/txe_block_pool_prioritize.o \
./threadx_common_src/src/txe_block_release.o \
./threadx_common_src/src/txe_byte_allocate.o \
./threadx_common_src/src/txe_byte_pool_create.o \
./threadx_common_src/src/txe_byte_pool_delete.o \
./threadx_common_src/src/txe_byte_pool_info_get.o \
./threadx_common_src/src/txe_byte_pool_prioritize.o \
./threadx_common_src/src/txe_byte_release.o \
./threadx_common_src/src/txe_event_flags_create.o \
./threadx_common_src/src/txe_event_flags_delete.o \
./threadx_common_src/src/txe_event_flags_get.o \
./threadx_common_src/src/txe_event_flags_info_get.o \
./threadx_common_src/src/txe_event_flags_set.o \
./threadx_common_src/src/txe_event_flags_set_notify.o \
./threadx_common_src/src/txe_mutex_create.o \
./threadx_common_src/src/txe_mutex_delete.o \
./threadx_common_src/src/txe_mutex_get.o \
./threadx_common_src/src/txe_mutex_info_get.o \
./threadx_common_src/src/txe_mutex_prioritize.o \
./threadx_common_src/src/txe_mutex_put.o \
./threadx_common_src/src/txe_queue_create.o \
./threadx_common_src/src/txe_queue_delete.o \
./threadx_common_src/src/txe_queue_flush.o \
./threadx_common_src/src/txe_queue_front_send.o \
./threadx_common_src/src/txe_queue_info_get.o \
./threadx_common_src/src/txe_queue_prioritize.o \
./threadx_common_src/src/txe_queue_receive.o \
./threadx_common_src/src/txe_queue_send.o \
./threadx_common_src/src/txe_queue_send_notify.o \
./threadx_common_src/src/txe_semaphore_ceiling_put.o \
./threadx_common_src/src/txe_semaphore_create.o \
./threadx_common_src/src/txe_semaphore_delete.o \
./threadx_common_src/src/txe_semaphore_get.o \
./threadx_common_src/src/txe_semaphore_info_get.o \
./threadx_common_src/src/txe_semaphore_prioritize.o \
./threadx_common_src/src/txe_semaphore_put.o \
./threadx_common_src/src/txe_semaphore_put_notify.o \
./threadx_common_src/src/txe_thread_create.o \
./threadx_common_src/src/txe_thread_delete.o \
./threadx_common_src/src/txe_thread_entry_exit_notify.o \
./threadx_common_src/src/txe_thread_info_get.o \
./threadx_common_src/src/txe_thread_preemption_change.o \
./threadx_common_src/src/txe_thread_priority_change.o \
./threadx_common_src/src/txe_thread_relinquish.o \
./threadx_common_src/src/txe_thread_reset.o \
./threadx_common_src/src/txe_thread_resume.o \
./threadx_common_src/src/txe_thread_suspend.o \
./threadx_common_src/src/txe_thread_terminate.o \
./threadx_common_src/src/txe_thread_time_slice_change.o \
./threadx_common_src/src/txe_thread_wait_abort.o \
./threadx_common_src/src/txe_timer_activate.o \
./threadx_common_src/src/txe_timer_change.o \
./threadx_common_src/src/txe_timer_create.o \
./threadx_common_src/src/txe_timer_deactivate.o \
./threadx_common_src/src/txe_timer_delete.o \
./threadx_common_src/src/txe_timer_info_get.o 


# Each subdirectory must supply rules for building sources it contributes
threadx_common_src/src/tx_block_allocate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_allocate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_allocate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_cleanup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_cleanup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_cleanup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_pool_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_pool_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_pool_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_block_release.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_block_release.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_block_release.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_allocate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_allocate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_allocate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_cleanup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_cleanup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_cleanup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_pool_search.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_pool_search.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_pool_search.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_byte_release.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_byte_release.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_byte_release.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_cleanup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_cleanup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_cleanup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_set.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_set.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_set.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_event_flags_set_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_event_flags_set_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_event_flags_set_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_initialize_high_level.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_initialize_high_level.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_initialize_high_level.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_initialize_kernel_enter.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_initialize_kernel_enter.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_initialize_kernel_enter.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_initialize_kernel_setup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_initialize_kernel_setup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_initialize_kernel_setup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_misra.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_misra.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_misra.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_cleanup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_cleanup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_cleanup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_priority_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_priority_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_priority_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_mutex_put.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_mutex_put.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_mutex_put.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_cleanup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_cleanup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_cleanup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_flush.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_flush.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_flush.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_front_send.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_front_send.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_front_send.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_receive.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_receive.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_receive.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_send.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_send.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_send.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_queue_send_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_queue_send_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_queue_send_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_ceiling_put.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_ceiling_put.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_ceiling_put.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_cleanup.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_cleanup.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_cleanup.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_put.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_put.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_put.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_semaphore_put_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_semaphore_put_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_semaphore_put_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_entry_exit_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_entry_exit_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_entry_exit_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_identify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_identify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_identify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_preemption_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_preemption_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_preemption_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_priority_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_priority_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_priority_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_relinquish.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_relinquish.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_relinquish.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_reset.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_reset.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_reset.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_resume.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_resume.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_resume.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_shell_entry.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_shell_entry.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_shell_entry.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_sleep.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_sleep.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_sleep.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_stack_analyze.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_stack_analyze.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_stack_analyze.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_stack_error_handler.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_stack_error_handler.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_stack_error_handler.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_stack_error_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_stack_error_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_stack_error_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_suspend.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_suspend.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_suspend.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_system_preempt_check.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_system_preempt_check.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_system_preempt_check.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_system_resume.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_system_resume.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_system_resume.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_system_suspend.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_system_suspend.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_system_suspend.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_terminate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_terminate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_terminate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_time_slice.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_time_slice.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_time_slice.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_time_slice_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_time_slice_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_time_slice_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_timeout.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_timeout.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_timeout.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_thread_wait_abort.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_thread_wait_abort.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_thread_wait_abort.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_time_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_time_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_time_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_time_set.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_time_set.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_time_set.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_activate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_activate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_activate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_deactivate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_deactivate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_deactivate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_expiration_process.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_expiration_process.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_expiration_process.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_performance_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_performance_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_performance_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_performance_system_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_performance_system_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_performance_system_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_system_activate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_system_activate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_system_activate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_system_deactivate.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_system_deactivate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_system_deactivate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_timer_thread_entry.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_timer_thread_entry.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_timer_thread_entry.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_buffer_full_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_buffer_full_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_buffer_full_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_disable.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_disable.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_disable.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_enable.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_enable.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_enable.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_event_filter.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_event_filter.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_event_filter.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_event_unfilter.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_event_unfilter.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_event_unfilter.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_initialize.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_initialize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_initialize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_interrupt_control.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_interrupt_control.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_interrupt_control.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_isr_enter_insert.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_isr_enter_insert.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_isr_enter_insert.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_isr_exit_insert.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_isr_exit_insert.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_isr_exit_insert.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_object_register.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_object_register.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_object_register.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_object_unregister.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_object_unregister.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_object_unregister.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/tx_trace_user_event_insert.o: /Users/hoan/IOcomposer/external/threadx/common/src/tx_trace_user_event_insert.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/tx_trace_user_event_insert.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_block_allocate.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_block_allocate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_block_allocate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_block_pool_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_block_pool_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_block_pool_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_block_pool_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_block_pool_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_block_pool_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_block_pool_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_block_pool_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_block_pool_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_block_release.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_block_release.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_block_release.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_byte_allocate.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_allocate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_byte_allocate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_byte_pool_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_byte_pool_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_byte_pool_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_byte_pool_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_byte_pool_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_byte_pool_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_byte_pool_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_pool_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_byte_pool_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_byte_release.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_byte_release.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_byte_release.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_event_flags_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_event_flags_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_event_flags_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_event_flags_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_event_flags_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_event_flags_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_event_flags_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_event_flags_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_event_flags_set.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_set.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_event_flags_set.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_event_flags_set_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_event_flags_set_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_event_flags_set_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_mutex_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_mutex_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_mutex_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_mutex_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_mutex_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_mutex_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_mutex_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_mutex_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_mutex_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_mutex_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_mutex_put.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_mutex_put.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_mutex_put.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_flush.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_flush.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_flush.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_front_send.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_front_send.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_front_send.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_receive.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_receive.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_receive.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_send.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_send.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_send.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_queue_send_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_queue_send_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_queue_send_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_ceiling_put.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_ceiling_put.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_ceiling_put.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_prioritize.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_prioritize.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_prioritize.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_put.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_put.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_put.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_semaphore_put_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_semaphore_put_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_semaphore_put_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_entry_exit_notify.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_entry_exit_notify.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_entry_exit_notify.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_preemption_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_preemption_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_preemption_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_priority_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_priority_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_priority_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_relinquish.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_relinquish.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_relinquish.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_reset.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_reset.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_reset.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_resume.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_resume.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_resume.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_suspend.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_suspend.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_suspend.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_terminate.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_terminate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_terminate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_time_slice_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_time_slice_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_time_slice_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_thread_wait_abort.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_thread_wait_abort.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_thread_wait_abort.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_timer_activate.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_activate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_timer_activate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_timer_change.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_change.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_timer_change.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_timer_create.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_create.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_timer_create.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_timer_deactivate.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_deactivate.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_timer_deactivate.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_timer_delete.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_delete.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_timer_delete.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

threadx_common_src/src/txe_timer_info_get.o: /Users/hoan/IOcomposer/external/threadx/common/src/txe_timer_info_get.c threadx_common_src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"threadx_common_src/src/txe_timer_info_get.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


