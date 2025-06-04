/*
 * light_control_service.h
 *
 *  Created on: Jun 4, 2025
 *      Author: Admin
 */

#ifndef INC_SERVICES_LIGHT_CONTROL_SERVICE_H_
#define INC_SERVICES_LIGHT_CONTROL_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include "gpio_driver.h"
#define RELAY_ACTIVE_LEVEL   GPIO_PIN_SET // 1
#define RELAY_INACTIVE_LEVEL GPIO_PIN_RESET


/**
 * @brief Initializes the Light Control Service.
 * @param light_relay_port: GPIO port connected to the light relay control pin.
 * @param light_relay_pin_number: Pin number connected to the light relay control pin.
 */
void LightService_Init(GPIO_TypeDef* light_relay_port, uint8_t light_relay_pin_number);

/**
 * @brief Turns the light ON by activating the relay.
 */
void LightService_TurnOn(void);

/**
 * @brief Turns the light OFF by deactivating the relay.
 */
void LightService_TurnOff(void);

/**
 * @brief Gets the current state of the light.
 * @return true if the light is ON, false if the light is OFF.
 */
bool LightService_GetState(void);

#endif /* INC_SERVICES_LIGHT_CONTROL_SERVICE_H_ */
