/*
 * pir_sensor_service.h
 *
 *  Created on: May 29, 2025
 *      Author: Admin
 */

#ifndef INC_SERVICES_PIR_SENSOR_SERVICE_H_
#define INC_SERVICES_PIR_SENSOR_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include "stm32f401xe.h"
#include "exti_driver.h"

typedef enum {
    PIR_SENSOR_IN = 0, //PIR ở trong
    PIR_SENSOR_OUT = 1, //PIR ở ngoài
    PIR_SENSOR_COUNT = 2// Số lượng cảm biến PIR
} PIR_SensorID_t;

void PIRService_Init(GPIO_TypeDef* pir1_port, uint8_t pir1_pin_number,
                     GPIO_TypeDef* pir2_port, uint8_t pir2_pin_number,
                     uint8_t nvic_priority);

bool PIRService_IsMotionDetected(PIR_SensorID_t sensor_id);



#endif /* INC_SERVICES_PIR_SENSOR_SERVICE_H_ */
