################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Services/limit_switch_service.c \
../Core/Src/Services/motor_control_service.c \
../Core/Src/Services/pir__sensor_service.c \
../Core/Src/Services/uart_protocol_service.c 

OBJS += \
./Core/Src/Services/limit_switch_service.o \
./Core/Src/Services/motor_control_service.o \
./Core/Src/Services/pir__sensor_service.o \
./Core/Src/Services/uart_protocol_service.o 

C_DEPS += \
./Core/Src/Services/limit_switch_service.d \
./Core/Src/Services/motor_control_service.d \
./Core/Src/Services/pir__sensor_service.d \
./Core/Src/Services/uart_protocol_service.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Services/%.o Core/Src/Services/%.su Core/Src/Services/%.cyclo: ../Core/Src/Services/%.c Core/Src/Services/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Drivers" -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Services" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Services

clean-Core-2f-Src-2f-Services:
	-$(RM) ./Core/Src/Services/limit_switch_service.cyclo ./Core/Src/Services/limit_switch_service.d ./Core/Src/Services/limit_switch_service.o ./Core/Src/Services/limit_switch_service.su ./Core/Src/Services/motor_control_service.cyclo ./Core/Src/Services/motor_control_service.d ./Core/Src/Services/motor_control_service.o ./Core/Src/Services/motor_control_service.su ./Core/Src/Services/pir__sensor_service.cyclo ./Core/Src/Services/pir__sensor_service.d ./Core/Src/Services/pir__sensor_service.o ./Core/Src/Services/pir__sensor_service.su ./Core/Src/Services/uart_protocol_service.cyclo ./Core/Src/Services/uart_protocol_service.d ./Core/Src/Services/uart_protocol_service.o ./Core/Src/Services/uart_protocol_service.su

.PHONY: clean-Core-2f-Src-2f-Services

