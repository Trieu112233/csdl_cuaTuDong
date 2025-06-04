#include "motor_control_service.h"
#include "stm32f401xe.h"
#include <stdio.h>

// Cấu hình phần cứng động cơ (ví dụ: TIM3 CH1, PA6, AF2)
#define MOTOR_PWM_TIMER      PWM_TIMER3
#define MOTOR_PWM_CHANNEL    PWM_CHANNEL_1
#define MOTOR_PWM_GPIO_PORT  GPIOA
#define MOTOR_PWM_GPIO_PIN   6
#define MOTOR_PWM_AF         2
#define MOTOR_PWM_FREQ_HZ    20000 // 20kHz

// Biến lưu trữ thông tin chân điều khiển hướng
static GPIO_TypeDef* g_motor_dir1_port = NULL;
static uint16_t      g_motor_dir1_pin_mask = 0;
static GPIO_TypeDef* g_motor_dir2_port = NULL;
static uint16_t      g_motor_dir2_pin_mask = 0;

void Motor_Init(GPIO_TypeDef* dir_1_port, uint8_t dir_1_pin_number,
                GPIO_TypeDef* dir_2_port, uint8_t dir_2_pin_number) {
    // Lưu thông tin chân điều khiển hướng
    g_motor_dir1_port = dir_1_port;
    g_motor_dir1_pin_mask = (1U << dir_1_pin_number);
    g_motor_dir2_port = dir_2_port;
    g_motor_dir2_pin_mask = (1U << dir_2_pin_number);

    // Khởi tạo chân PWM
    PWM_InitChannel(MOTOR_PWM_TIMER, MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ_HZ,
                    MOTOR_PWM_GPIO_PORT, MOTOR_PWM_GPIO_PIN, MOTOR_PWM_AF);
    
    // Khởi tạo các chân điều khiển hướng là output
    GPIO_InitPin(g_motor_dir1_port, g_motor_dir1_pin_mask,
                 GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW,
                 GPIO_OTYPE_PUSHPULL, 0);
    GPIO_InitPin(g_motor_dir2_port, g_motor_dir2_pin_mask,
                 GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW,
                 GPIO_OTYPE_PUSHPULL, 0);

    Motor_Stop(); // Đặt trạng thái ban đầu là dừng và coast
    PWM_Start(MOTOR_PWM_TIMER);
}

void Motor_SetSpeed(float percent) {
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    PWM_SetDutyCycle(MOTOR_PWM_TIMER, MOTOR_PWM_CHANNEL, percent);
}

void Motor_SetDirection(MotorDirection_t direction) {
    if (g_motor_dir1_port == NULL || g_motor_dir2_port == NULL) return;

    switch (direction) {
        case MOTOR_FORWARD:
            GPIO_WritePin(g_motor_dir1_port, g_motor_dir1_pin_mask, GPIO_PIN_SET);
            GPIO_WritePin(g_motor_dir2_port, g_motor_dir2_pin_mask, GPIO_PIN_RESET);
            break;
        case MOTOR_REVERSE:
            GPIO_WritePin(g_motor_dir1_port, g_motor_dir1_pin_mask, GPIO_PIN_RESET);
            GPIO_WritePin(g_motor_dir2_port, g_motor_dir2_pin_mask, GPIO_PIN_SET);
            break;
        case MOTOR_COAST:
        default:
            GPIO_WritePin(g_motor_dir1_port, g_motor_dir1_pin_mask, GPIO_PIN_RESET);
            GPIO_WritePin(g_motor_dir2_port, g_motor_dir2_pin_mask, GPIO_PIN_RESET);
            break;
    }
}

void Motor_Stop(void) {
    Motor_SetSpeed(0.0f);
    Motor_SetDirection(MOTOR_COAST); // Đưa về trạng thái coast khi dừng
}
