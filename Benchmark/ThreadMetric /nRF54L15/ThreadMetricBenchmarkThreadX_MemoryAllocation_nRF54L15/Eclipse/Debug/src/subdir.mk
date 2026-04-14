################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /nRF54L15/ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15/src/main.cpp \
/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /nRF54L15/ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15/src/tm_port.cpp 

S_UPPER_SRCS += \
/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /nRF54L15/ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15/src/tx_initialize_low_level.S 

C_SRCS += \
/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /src/memory_allocation.c \
/Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /src/tm_report.c 

C_DEPS += \
./src/memory_allocation.d \
./src/tm_report.d 

OBJS += \
./src/main.o \
./src/memory_allocation.o \
./src/tm_port.o \
./src/tm_report.o \
./src/tx_initialize_low_level.o 

S_UPPER_DEPS += \
./src/tx_initialize_low_level.d 

CPP_DEPS += \
./src/main.d \
./src/tm_port.d 


# Each subdirectory must supply rules for building sources it contributes
src/main.o: /Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /nRF54L15/ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15/src/main.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"/../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu++23 -fabi-version=0 -fno-exceptions -fno-rtti -MMD -MP -MF"src/main.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/memory_allocation.o: /Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /src/memory_allocation.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"src/memory_allocation.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/tm_port.o: /Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /nRF54L15/ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15/src/tm_port.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"/../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu++23 -fabi-version=0 -fno-exceptions -fno-rtti -MMD -MP -MF"src/tm_port.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/tm_report.o: /Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /src/tm_report.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -DNRF54L15_XXAA -DNRF_APPLICATION -D__PROGRAM_START -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -I"../../src" -I"../../../../src" -I"../../../../../../ARM/include" -I"../../../../../../include" -I"/Users/hoan/IOcomposer/IOsonata/ARM/CMSIS/Core/Include" -I"/Users/hoan/IOcomposer/external/nrfx/bsp/stable/mdk" -I"/Users/hoan/IOcomposer/external/threadx/common/inc" -I"/Users/hoan/IOcomposer/external/threadx/ports/cortex_m33/gnu/inc" -std=gnu11 -MMD -MP -MF"src/tm_report.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/tx_initialize_low_level.o: /Users/hoan/swdev/private/I-SYST/TaktOS_Dev/Benchmark/Thread-Metric\ /nRF54L15/ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15/src/tx_initialize_low_level.S src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -x assembler-with-cpp -I"../../src" -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_SECURE -MMD -MP -MF"src/tx_initialize_low_level.d" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


