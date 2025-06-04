################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Drivers/exti_driver.c \
../Core/Src/Drivers/gpio_driver.c \
../Core/Src/Drivers/pwm_driver.c \
../Core/Src/Drivers/rcc_config.c \
../Core/Src/Drivers/systick_driver.c \
../Core/Src/Drivers/uart_driver.c 

OBJS += \
./Core/Src/Drivers/exti_driver.o \
./Core/Src/Drivers/gpio_driver.o \
./Core/Src/Drivers/pwm_driver.o \
./Core/Src/Drivers/rcc_config.o \
./Core/Src/Drivers/systick_driver.o \
./Core/Src/Drivers/uart_driver.o 

C_DEPS += \
./Core/Src/Drivers/exti_driver.d \
./Core/Src/Drivers/gpio_driver.d \
./Core/Src/Drivers/pwm_driver.d \
./Core/Src/Drivers/rcc_config.d \
./Core/Src/Drivers/systick_driver.d \
./Core/Src/Drivers/uart_driver.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Drivers/%.o Core/Src/Drivers/%.su Core/Src/Drivers/%.cyclo: ../Core/Src/Drivers/%.c Core/Src/Drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Drivers" -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Services" -I"C:/Users/Admin/STM32CubeIDE/workspace_1.17.0/cuaTuDong/Core/Inc/Applications" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Drivers

clean-Core-2f-Src-2f-Drivers:
	-$(RM) ./Core/Src/Drivers/exti_driver.cyclo ./Core/Src/Drivers/exti_driver.d ./Core/Src/Drivers/exti_driver.o ./Core/Src/Drivers/exti_driver.su ./Core/Src/Drivers/gpio_driver.cyclo ./Core/Src/Drivers/gpio_driver.d ./Core/Src/Drivers/gpio_driver.o ./Core/Src/Drivers/gpio_driver.su ./Core/Src/Drivers/pwm_driver.cyclo ./Core/Src/Drivers/pwm_driver.d ./Core/Src/Drivers/pwm_driver.o ./Core/Src/Drivers/pwm_driver.su ./Core/Src/Drivers/rcc_config.cyclo ./Core/Src/Drivers/rcc_config.d ./Core/Src/Drivers/rcc_config.o ./Core/Src/Drivers/rcc_config.su ./Core/Src/Drivers/systick_driver.cyclo ./Core/Src/Drivers/systick_driver.d ./Core/Src/Drivers/systick_driver.o ./Core/Src/Drivers/systick_driver.su ./Core/Src/Drivers/uart_driver.cyclo ./Core/Src/Drivers/uart_driver.d ./Core/Src/Drivers/uart_driver.o ./Core/Src/Drivers/uart_driver.su

.PHONY: clean-Core-2f-Src-2f-Drivers

