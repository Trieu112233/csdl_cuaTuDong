/*
 * pir__sensor_service.c
 *
 *  Created on: May 29, 2025
 *      Author: Admin
 */

#include "pir_sensor_service.h"
#include "gpio_driver.h"
#include "systick_driver.h"

static volatile bool g_pir_motion_detected_state[PIR_SENSOR_COUNT] = {false};

typedef struct {
    GPIO_TypeDef* port;
    uint8_t       pin_number; // 0-15
    uint8_t       exti_line;  // EXTI line tương ứng với pin_number
} PIR_GpioConfig_t;

static PIR_GpioConfig_t g_pir_configs[PIR_SENSOR_COUNT];

// Callback cho PIR 1
static void pir1_exti_event_handler(uint8_t exti_line) {
    // Giả định HC-SR501: Output HIGH khi phát hiện, LOW khi không.
    // Cấu hình EXTI_TRIGGER_RISING để báo hiệu bắt đầu phát hiện.
    // Và EXTI_TRIGGER_FALLING để báo hiệu kết thúc phát hiện.
    // GPIO_ReadPin trả về 1 (GPIO_PIN_SET) nếu pin đang HIGH.

    if (GPIO_ReadPin(g_pir_configs[PIR_SENSOR_1].port, (1U << g_pir_configs[PIR_SENSOR_1].pin_number)) == GPIO_PIN_SET) {
        g_pir_motion_detected_state[PIR_SENSOR_1] = true;
    } else {
        g_pir_motion_detected_state[PIR_SENSOR_1] = false;
    }
}

// Callback cho PIR 2
static void pir2_exti_event_handler(uint8_t exti_line) {
    if (GPIO_ReadPin(g_pir_configs[PIR_SENSOR_2].port, (1U << g_pir_configs[PIR_SENSOR_2].pin_number)) == GPIO_PIN_SET) {
        g_pir_motion_detected_state[PIR_SENSOR_2] = true;
    } else {
        g_pir_motion_detected_state[PIR_SENSOR_2] = false;
    }
}

void PIRService_Init(GPIO_TypeDef* pir1_port, uint8_t pir1_pin_number,
                     GPIO_TypeDef* pir2_port, uint8_t pir2_pin_number,
                     uint8_t nvic_priority) {
    // Lưu cấu hình chân
    g_pir_configs[PIR_SENSOR_1].port = pir1_port;
    g_pir_configs[PIR_SENSOR_1].pin_number = pir1_pin_number;
    g_pir_configs[PIR_SENSOR_1].exti_line = pir1_pin_number; // EXTI line thường trùng pin_number

    g_pir_configs[PIR_SENSOR_2].port = pir2_port;
    g_pir_configs[PIR_SENSOR_2].pin_number = pir2_pin_number;
    g_pir_configs[PIR_SENSOR_2].exti_line = pir2_pin_number;

    // Reset trạng thái ban đầu
    g_pir_motion_detected_state[PIR_SENSOR_1] = false;
    g_pir_motion_detected_state[PIR_SENSOR_2] = false;

    // Chân GPIO sẽ được EXTI_InitPin cấu hình là input (pull-up là lựa chọn an toàn).
    EXTI_InitPin(g_pir_configs[PIR_SENSOR_1].port,
                 g_pir_configs[PIR_SENSOR_1].pin_number,
                 EXTI_TRIGGER_BOTH, // <<<< Sườn lên và xuống
                 nvic_priority,
                 pir1_exti_event_handler);

    // Khởi tạo EXTI cho PIR2
    EXTI_InitPin(g_pir_configs[PIR_SENSOR_2].port,
                 g_pir_configs[PIR_SENSOR_2].pin_number,
                 EXTI_TRIGGER_BOTH, // <<<< Sườn lên và xuống
                 nvic_priority,     // Có thể dùng cùng priority hoặc khác
                 pir2_exti_event_handler);
}

bool PIRService_IsMotionDetected(PIR_SensorID_t sensor_id) {
    if (sensor_id < PIR_SENSOR_COUNT) {
        return g_pir_motion_detected_state[sensor_id];
    }
    return false; // ID không hợp lệ
}


