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
    PIR_SENSOR_1 = 0, // Ví dụ: PIR ở lối vào/ngoài
    PIR_SENSOR_2 = 1, // Ví dụ: PIR ở lối ra/trong
    PIR_SENSOR_COUNT // Số lượng cảm biến PIR
} PIR_SensorID_t;

void PIRService_Init(GPIO_TypeDef* pir1_port, uint8_t pir1_pin_number,
                     GPIO_TypeDef* pir2_port, uint8_t pir2_pin_number,
                     uint8_t nvic_priority);

bool PIRService_IsMotionDetected(PIR_SensorID_t sensor_id);



#endif /* INC_SERVICES_PIR_SENSOR_SERVICE_H_ */
