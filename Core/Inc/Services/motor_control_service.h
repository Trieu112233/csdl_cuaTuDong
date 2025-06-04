#ifndef MOTOR_CONTROL_SERVICE_H
#define MOTOR_CONTROL_SERVICE_H

#include <stdint.h>
#include "pwm_driver.h"
#include "gpio_driver.h"

// Định nghĩa các hướng hoạt động của động cơ
typedef enum {
    MOTOR_FORWARD,
    MOTOR_REVERSE,
    MOTOR_COAST
} MotorDirection_t;

// Khởi tạo động cơ (chỉ gọi 1 lần ở main)
void Motor_Init(GPIO_TypeDef* dir_1_port, uint8_t dir_1_pin_number,
        		GPIO_TypeDef* dir_2_port, uint8_t dir_2_pin_number);

// Điều khiển tốc độ động cơ (0.0 - 100.0 %)
void Motor_SetSpeed(float percent);

// Cài đặt hướng quay cho động cơ
void Motor_SetDirection(MotorDirection_t direction);

// Dừng động cơ (tốc độ về 0 và đưa về trạng thái coast)
void Motor_Stop(void);

#endif // MOTOR_CONTROL_SERVICE_H
