################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include threadx_common_src/src/subdir.mk
-include src/src/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(SX_DEPS)),)
-include $(SX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(CCM_DEPS)),)
-include $(CCM_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CXXM_DEPS)),)
-include $(CXXM_DEPS)
endif
ifneq ($(strip $(C++M_DEPS)),)
-include $(C++M_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.hex \

SECONDARY_SIZE += \
ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.siz \


# All Target
all: main-build

# Main-build Target
main-build: ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf secondary-outputs

# Tool invocations
ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C++ Linker'
	arm-none-eabi-g++ -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -g3 -T "/Users/hoan/IOcomposer/IOsonata/ARM/Nordic/nRF54/ldscript/gcc_nrf54l15_xxaa.ld" -Xlinker --gc-sections -L"../../../../../../../../../IOsonata/ARM/Nordic/nRF54/nRF54L15/lib/Eclipse/Debug" -L"../../../../../../ARM/cm33/Eclipse/Debug" -L"/Users/hoan/IOcomposer/IOsonata/ARM/ldscript" -L"/Users/hoan/IOcomposer/IOsonata/ARM/Nordic/nRF54/nRF54L15/lib/Eclipse/Debug" -Wl,-Map,"ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.map" --specs=nano.specs --specs=rdimon.specs -Wl,--start-group -lgcc -lc -lm -lrdimon -Wl,--end-group -o "ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.hex: ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex "ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf"  "ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.hex"
	@echo 'Finished building: $@'
	@echo ' '

ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.siz: ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CCM_DEPS)$(C_UPPER_DEPS)$(SECONDARY_SIZE)$(CXXM_DEPS)$(C_DEPS)$(CC_DEPS)$(SX_DEPS)$(C++_DEPS)$(OBJS)$(CXX_DEPS)$(SECONDARY_FLASH)$(ASM_DEPS)$(S_UPPER_DEPS)$(C++M_DEPS)$(CPP_DEPS) ThreadMetricBenchmarkThreadX_MemoryAllocation_nRF54L15.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents main-build

-include ../makefile.targets
