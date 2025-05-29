/*
 * gpio_driver.c
 *
 *  Created on: May 26, 2025
 *      Author: Admin
 */

#include "gpio_driver.h"

// Helper function to enable/disable clock for GPIO port
void GPIO_ClockCmd(GPIO_TypeDef* GPIOx, uint8_t NewState) {
    if (NewState == CLOCK_CMD_ENABLE) {
        if (GPIOx == GPIOA) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        else if (GPIOx == GPIOB) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
        else if (GPIOx == GPIOC) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
        else if (GPIOx == GPIOD) RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
        else if (GPIOx == GPIOE) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
        // STM32F401RE has up to GPIOE and GPIOH
        else if (GPIOx == GPIOH) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    } else {
        if (GPIOx == GPIOA) RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOAEN;
        else if (GPIOx == GPIOB) RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOBEN;
        else if (GPIOx == GPIOC) RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOCEN;
        else if (GPIOx == GPIOD) RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIODEN;
        else if (GPIOx == GPIOE) RCC->AHB1ENR &= ~RCC_AHB1ENR_GPIOEEN;
        // ...
    }
}


void GPIO_InitPin(GPIO_TypeDef* GPIOx, uint16_t pin, uint32_t mode, uint32_t pull,
                  uint32_t speed, uint32_t otype, uint32_t alt_func) {
    uint32_t position;
    uint32_t iocurrent;
    uint32_t temp;

    // Enable clock for the GPIO port
    GPIO_ClockCmd(GPIOx, CLOCK_CMD_ENABLE);

    for (position = 0; position < 16; position++) {
        iocurrent = (0x1U << position); // Mask for current pin
        if (pin & iocurrent) {          // If this pin is selected
            // 1. Configure Mode (Input, Output, AF, Analog)
            temp = GPIOx->MODER;
            temp &= ~(0x03U << (position * 2)); // Clear mode bits for this pin
            temp |= (mode << (position * 2));
            GPIOx->MODER = temp;

            if ((mode == GPIO_MODE_OUTPUT) || (mode == GPIO_MODE_AF)) {
                // 2. Configure Speed
                temp = GPIOx->OSPEEDR;
                temp &= ~(0x03U << (position * 2));
                temp |= (speed << (position * 2));
                GPIOx->OSPEEDR = temp;

                // 3. Configure Output Type (Push-Pull or Open-Drain)
                temp = GPIOx->OTYPER;
                temp &= ~(0x01U << position);
                temp |= (otype << position);
                GPIOx->OTYPER = temp;
            }

            // 4. Configure Pull-up/Pull-down
            temp = GPIOx->PUPDR;
            temp &= ~(0x03U << (position * 2));
            temp |= (pull << (position * 2));
            GPIOx->PUPDR = temp;

            // 5. Configure Alternate Function (if AF mode)
            if (mode == GPIO_MODE_AF) {
                if (position < 8) { // AFR[0] for pins 0-7
                    temp = GPIOx->AFR[0];
                    temp &= ~(0x0FU << (position * 4));
                    temp |= (alt_func << (position * 4));
                    GPIOx->AFR[0] = temp;
                } else { // AFR[1] for pins 8-15
                    temp = GPIOx->AFR[1];
                    temp &= ~(0x0FU << ((position - 8) * 4));
                    temp |= (alt_func << ((position - 8) * 4));
                    GPIOx->AFR[1] = temp;
                }
            }
        }
    }
}

uint8_t GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t pin) {
    if ((GPIOx->IDR & pin) != 0x00U) {
        return GPIO_PIN_SET;
    } else {
        return GPIO_PIN_RESET;
    }
}

void GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t pin, uint8_t state) {
    if (state == GPIO_PIN_SET) {
        GPIOx->BSRR = (uint32_t)pin; // Set bit
    } else {
        GPIOx->BSRR = (uint32_t)pin << 16U; // Reset bit
    }
}

void GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t pin) {
    GPIOx->ODR ^= pin;
}
