#include "motor_control_service.h"
#include "stm32f401xe.h"

// Cấu hình phần cứng động cơ (ví dụ: TIM3 CH1, PA6, AF2)
#define MOTOR_PWM_TIMER      PWM_TIMER3
#define MOTOR_PWM_CHANNEL    PWM_CHANNEL_1
#define MOTOR_PWM_GPIO_PORT  GPIOA
#define MOTOR_PWM_GPIO_PIN   6
#define MOTOR_PWM_AF         2
#define MOTOR_PWM_FREQ_HZ    20000 // 20kHz

void Motor_Init(void) {
    PWM_InitChannel(MOTOR_PWM_TIMER, MOTOR_PWM_CHANNEL, MOTOR_PWM_FREQ_HZ,
                    MOTOR_PWM_GPIO_PORT, MOTOR_PWM_GPIO_PIN, MOTOR_PWM_AF);
    PWM_SetDutyCycle(MOTOR_PWM_TIMER, MOTOR_PWM_CHANNEL, 0.0f);
    PWM_Start(MOTOR_PWM_TIMER);
}

void Motor_SetSpeed(float percent) {
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    PWM_SetDutyCycle(MOTOR_PWM_TIMER, MOTOR_PWM_CHANNEL, percent);
}
