#include "rcc_config.h"
#include "systick_driver.h"
#include "gpio_driver.h"
#include "motor_control_service.h" // Service cần kiểm tra

// Định nghĩa chân cho điều khiển hướng động cơ 
#define MOTOR_DIR1_PORT        GPIOB
#define MOTOR_DIR1_PIN_NUM     8
#define MOTOR_DIR2_PORT        GPIOB
#define MOTOR_DIR2_PIN_NUM     9

int main(void) {
    SystemClock_Config(); 
    SysTick_Init();       

    Motor_Init(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN_NUM,
               MOTOR_DIR2_PORT, MOTOR_DIR2_PIN_NUM);
    Motor_SetDirection(MOTOR_FORWARD);

    while (1) {
        // Chạy tiến với 90% tốc độ trong 2 giây
        Motor_SetDirection(MOTOR_FORWARD);
        Motor_SetSpeed(90.0f);
        Delay_ms(2000);
        // Dừng động cơ trong 2 giây
        Motor_Stop();
        Delay_ms(2000);
        // Chạy lùi với 100% tốc độ trong 2 giây
        Motor_SetDirection(MOTOR_REVERSE);
        Motor_SetSpeed(100.0f);
        Delay_ms(2000);
        // Dừng động cơ trong 2 giây
        Motor_Stop();
        Delay_ms(2000);
    }

}