/*
 * limit_switch_service.h
 *
 *  Created on: May 29, 2025
 *      Author: Admin
 */

#ifndef INC_SERVICES_LIMIT_SWITCH_SERVICE_H_
#define INC_SERVICES_LIMIT_SWITCH_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include "gpio_driver.h"

typedef enum {
    LIMIT_SWITCH_ID_DOOR_OPEN = 0,
    LIMIT_SWITCH_ID_DOOR_CLOSED,
    LIMIT_SWITCH_COUNT // Tổng số công tắc
} LimitSwitchID_t;

// Thời gian debounce (ms)
#define DEBOUNCE_TIME_MS 50

void LimitSwitchService_Init(GPIO_TypeDef* open_ls_port, uint8_t open_ls_pin_number,
                             GPIO_TypeDef* closed_ls_port, uint8_t closed_ls_pin_number,
                             uint8_t nvic_priority);

bool LimitSwitchService_IsDoorFullyOpen(void);

bool LimitSwitchService_IsDoorFullyClosed(void);

void LimitSwitchService_ProcessDebounce(void);

#endif /* INC_SERVICES_LIMIT_SWITCH_SERVICE_H_ */
