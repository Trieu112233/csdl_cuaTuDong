#ifndef MOTOR_CONTROL_SERVICE_H
#define MOTOR_CONTROL_SERVICE_H

#include <stdint.h>
#include "pwm_driver.h"

// Khởi tạo động cơ (chỉ gọi 1 lần ở main)
void Motor_Init(void);

// Điều khiển tốc độ động cơ (0.0 - 100.0 %)
void Motor_SetSpeed(float percent);

#endif // MOTOR_CONTROL_SERVICE_H
