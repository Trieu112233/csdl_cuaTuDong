#include <stdio.h>

#include "rcc_config.h"
#include "systick_driver.h"
#include "gpio_driver.h"
#include "pir_sensor_service.h" // Service cần kiểm tra

// Chân cho pir in và pir out
#define PIR_IN_PORT        GPIOB
#define PIR_IN_PIN_NUM     8

#define PIR_OUT_PORT      GPIOB
#define PIR_OUT_PIN_NUM   9

// Chân cho LED debug
#define LED_PIR_IN_PORT       GPIOC
#define LED_PIR_IN_PIN_NUM    8
#define LED_PIR_IN_PIN_MASK   (1U << LED_PIR_IN_PIN_NUM)

#define LED_PIR_OUT_PORT       GPIOC
#define LED_PIR_OUT_PIN_NUM    6
#define LED_PIR_OUT_PIN_MASK   (1U << LED_PIR_OUT_PIN_NUM)

// Ưu tiên ngắt cho EXTI (pir)
#define PIR_NVIC_PRIORITY  10 

int main(void) {
    SystemClock_Config();
    SysTick_Init();

    GPIO_InitPin(LED_PIR_IN_PORT, LED_PIR_IN_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_WritePin(LED_PIR_IN_PORT, LED_PIR_IN_PIN_MASK, GPIO_PIN_RESET); // Tắt LED ban đầu

    GPIO_InitPin(LED_PIR_OUT_PORT, LED_PIR_OUT_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_WritePin(LED_PIR_OUT_PORT, LED_PIR_OUT_PIN_MASK, GPIO_PIN_RESET); // Tắt LED ban đầu


    PIRService_Init(PIR_IN_PORT, PIR_IN_PIN_NUM,
                    PIR_OUT_PORT, PIR_OUT_PIN_NUM,
                    PIR_NVIC_PRIORITY);

    bool prev_pir_in_motion = PIRService_IsMotionDetected(PIR_SENSOR_IN);
    bool prev_pir_out_motion = PIRService_IsMotionDetected(PIR_SENSOR_OUT);
    bool current_pir_in_motion, current_pir_out_motion;

    while (1) {
        current_pir_in_motion = PIRService_IsMotionDetected(PIR_SENSOR_IN);
        current_pir_out_motion = PIRService_IsMotionDetected(PIR_SENSOR_OUT);


        // Kiểm tra trạng thái pir in thay đổi
        if (current_pir_in_motion != prev_pir_in_motion) {
            prev_pir_in_motion = current_pir_in_motion;
        }

        // Kiểm tra trạng thái pir out thay đổi
        if (current_pir_out_motion != prev_pir_out_motion) {
            prev_pir_out_motion = current_pir_out_motion;
        }

        if (current_pir_in_motion) {
            GPIO_WritePin(LED_PIR_IN_PORT, LED_PIR_IN_PIN_MASK, GPIO_PIN_SET); // Bật LED
        } else {
            GPIO_WritePin(LED_PIR_IN_PORT, LED_PIR_IN_PIN_MASK, GPIO_PIN_RESET); // Tắt LED
        }
        
        if (current_pir_out_motion) {
            GPIO_WritePin(LED_PIR_OUT_PORT, LED_PIR_OUT_PIN_MASK, GPIO_PIN_SET); // Bật LED
        } else {
            GPIO_WritePin(LED_PIR_OUT_PORT, LED_PIR_OUT_PIN_MASK, GPIO_PIN_RESET); // Tắt LED
        }

        Delay_ms(100); 
    }

}