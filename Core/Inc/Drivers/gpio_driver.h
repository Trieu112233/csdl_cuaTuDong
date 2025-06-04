/*
 * gpio_driver.h
 *
 *  Created on: May 26, 2025
 *      Author: Admin
 */

#ifndef INC_DRIVERS_GPIO_DRIVER_H_
#define INC_DRIVERS_GPIO_DRIVER_H_

#include "stm32f401xe.h"
#include <stdint.h>

// GPIO Modes
#define GPIO_MODE_INPUT     0x00U
#define GPIO_MODE_OUTPUT    0x01U
#define GPIO_MODE_AF        0x02U
#define GPIO_MODE_ANALOG    0x03U

// GPIO Output Types
#define GPIO_OTYPE_PUSHPULL   0x00U
#define GPIO_OTYPE_OPENDRAIN  0x01U

// GPIO Output Speeds
#define GPIO_SPEED_LOW      0x00U
#define GPIO_SPEED_MEDIUM   0x01U
#define GPIO_SPEED_HIGH     0x02U
#define GPIO_SPEED_VERYHIGH 0x03U

// GPIO Pull-up/Pull-down
#define GPIO_PULL_NO        0x00U
#define GPIO_PULL_UP        0x01U
#define GPIO_PULL_DOWN      0x02U

// Pin State
#define GPIO_PIN_RESET      0
#define GPIO_PIN_SET        1

// Clock Command States
#define CLOCK_CMD_DISABLE   0
#define CLOCK_CMD_ENABLE    1

void GPIO_InitPin(GPIO_TypeDef* GPIOx, uint16_t pin, uint32_t mode, uint32_t pull,
                  uint32_t speed, uint32_t otype, uint32_t alt_func);
uint8_t GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t pin);
void GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t pin, uint8_t state);
void GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t pin);
// Helper function to enable clock for GPIO port
void GPIO_ClockCmd(GPIO_TypeDef* GPIOx, uint8_t NewState);


#endif /* INC_DRIVERS_GPIO_DRIVER_H_ */
