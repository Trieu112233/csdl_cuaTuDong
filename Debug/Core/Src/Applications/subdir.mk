################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Applications/door_fsm.c \
../Core/Src/Applications/lighting_logic.c \
../Core/Src/Applications/people_counter.c \
../Core/Src/Applications/system_manager.c 

OBJS += \
./Core/Src/Applications/door_fsm.o \
./Core/Src/Applications/lighting_logic.o \
./Core/Src/Applications/people_counter.o \
./Core/Src/Applications/system_manager.o 

C_DEPS += \
./Core/Src/Applications/door_fsm.d \
./Core/Src/Applications/lighting_logic.d \
./Core/Src/Applications/people_counter.d \
./Core/Src/Applications/system_manager.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Applications/%.o Core/Src/Applications/%.su Core/Src/Applications/%.cyclo: ../Core/Src/Applications/%.c Core/Src/Applications/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Drivers" -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Services" -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Applications" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Applications

clean-Core-2f-Src-2f-Applications:
	-$(RM) ./Core/Src/Applications/door_fsm.cyclo ./Core/Src/Applications/door_fsm.d ./Core/Src/Applications/door_fsm.o ./Core/Src/Applications/door_fsm.su ./Core/Src/Applications/lighting_logic.cyclo ./Core/Src/Applications/lighting_logic.d ./Core/Src/Applications/lighting_logic.o ./Core/Src/Applications/lighting_logic.su ./Core/Src/Applications/people_counter.cyclo ./Core/Src/Applications/people_counter.d ./Core/Src/Applications/people_counter.o ./Core/Src/Applications/people_counter.su ./Core/Src/Applications/system_manager.cyclo ./Core/Src/Applications/system_manager.d ./Core/Src/Applications/system_manager.o ./Core/Src/Applications/system_manager.su

.PHONY: clean-Core-2f-Src-2f-Applications

