/*
 * exti_driver.h
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#ifndef INC_DRIVERS_EXTI_DRIVER_H_
#define INC_DRIVERS_EXTI_DRIVER_H_

#include <stdint.h>
#include <stdio.h>
#include "stm32f401xe.h"
#include "gpio_driver.h"

// Định nghĩa các loại trigger
#define EXTI_TRIGGER_NONE     0x00U
#define EXTI_TRIGGER_RISING   0x01U
#define EXTI_TRIGGER_FALLING  0x02U
#define EXTI_TRIGGER_BOTH     0x03U // Cả sườn lên và xuống

typedef void (*exti_callback_t)(uint8_t exti_line);

/**
 * @brief Initializes a GPIO pin for EXTI interrupt.
 * @param GPIOx: Pointer to GPIO peripheral (e.g., GPIOA, GPIOB).
 * @param pin_number: The pin number (0-15).
 * @param trigger_type: EXTI_TRIGGER_RISING, EXTI_TRIGGER_FALLING, or EXTI_TRIGGER_BOTH.
 * @param priority: NVIC interrupt priority (0-15 for STM32F4).
 * @param callback: Function to be called when the interrupt occurs for this line.
 *                  Pass NULL if no callback is needed at driver level.
 * @note  This function configures the GPIO pin as input (usually with pull-up/pull-down).
 *        It also enables the corresponding EXTI line and NVIC interrupt.
 */
void EXTI_InitPin(GPIO_TypeDef* GPIOx, uint8_t pin_number, uint8_t trigger_type,
                  uint8_t priority, exti_callback_t callback);

/**
 * @brief Clears the EXTI line pending bit.
 * @param exti_line: The EXTI line number (0-15).
 * @note This function should be called inside the ISR after handling the interrupt.
 *       It's exposed in case user wants to manage ISRs outside this driver.
 */
void EXTI_ClearPendingBit(uint8_t exti_line);

 void EXTI0_IRQHandler(void);
 void EXTI1_IRQHandler(void);
 void EXTI2_IRQHandler(void);
 void EXTI3_IRQHandler(void);
 void EXTI4_IRQHandler(void);
 void EXTI9_5_IRQHandler(void);
 void EXTI15_10_IRQHandler(void);

#endif /* INC_DRIVERS_EXTI_DRIVER_H_ */
