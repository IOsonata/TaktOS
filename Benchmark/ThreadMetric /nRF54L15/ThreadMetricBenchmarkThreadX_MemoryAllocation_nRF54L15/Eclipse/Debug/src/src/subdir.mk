################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_misra.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_context_restore.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_context_save.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_interrupt_control.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_interrupt_disable.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_interrupt_restore.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_schedule.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_stack_build.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_system_return.S \
/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_timer_interrupt.S 

OBJS += \
./src/src/tx_misra.o \
./src/src/tx_thread_context_restore.o \
./src/src/tx_thread_context_save.o \
./src/src/tx_thread_interrupt_control.o \
./src/src/tx_thread_interrupt_disable.o \
./src/src/tx_thread_interrupt_restore.o \
./src/src/tx_thread_schedule.o \
./src/src/tx_thread_stack_build.o \
./src/src/tx_thread_system_return.o \
./src/src/tx_timer_interrupt.o 

S_UPPER_DEPS += \
./src/src/tx_misra.d \
./src/src/tx_thread_context_restore.d \
./src/src/tx_thread_context_save.d \
./src/src/tx_thread_interrupt_control.d \
./src/src/tx_thread_interrupt_disable.d \
./src/src/tx_thread_interrupt_restore.d \
./src/src/tx_thread_schedule.d \
./src/src/tx_thread_stack_build.d \
./src/src/tx_thread_system_return.d \
./src/src/tx_timer_interrupt.d 


# Each subdirectory must supply rules for building sources it contributes
src/src/tx_misra.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_misra.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_misra.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_context_restore.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_context_restore.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_context_restore.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_context_save.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_context_save.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_context_save.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_interrupt_control.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_interrupt_control.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_interrupt_control.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_interrupt_disable.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_interrupt_disable.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_interrupt_disable.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_interrupt_restore.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_interrupt_restore.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_interrupt_restore.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_schedule.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_schedule.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_schedule.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_stack_build.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_stack_build.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_stack_build.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_thread_system_return.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_thread_system_return.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_thread_system_return.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/src/tx_timer_interrupt.o: /Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/src/tx_timer_interrupt.S src/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/src/tx_timer_interrupt.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


