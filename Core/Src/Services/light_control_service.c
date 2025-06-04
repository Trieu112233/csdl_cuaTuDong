/*
 * light_control_service.c
 *
 *  Created on: Jun 4, 2025
 *      Author: Admin
 */

#include "light_control_service.h"
#include <stdio.h>

// --- Biến nội bộ lưu trữ thông tin chân GPIO và trạng thái đèn ---
static GPIO_TypeDef* g_light_relay_port = NULL;
static uint16_t      g_light_relay_pin_mask = 0; // Sẽ là (1U << pin_number)
static bool          g_is_light_on = false;     // Trạng thái hiện tại của đèn

void LightService_Init(GPIO_TypeDef* light_relay_port, uint8_t light_relay_pin_number) {
    if (light_relay_port == NULL || light_relay_pin_number > 15) {
        // Xử lý lỗi đầu vào không hợp lệ (ví dụ: return hoặc assert)
        return;
    }

    g_light_relay_port = light_relay_port;
    g_light_relay_pin_mask = (1U << light_relay_pin_number);

    // Cấu hình chân GPIO làm output, push-pull, không pull, tốc độ thấp
    // GPIO_ClockCmd sẽ được gọi bên trong GPIO_InitPin
    GPIO_InitPin(g_light_relay_port, g_light_relay_pin_mask,
                 GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW,
                 GPIO_OTYPE_PUSHPULL, 0); // AF không dùng

    // Đặt trạng thái ban đầu cho relay (ví dụ: tắt đèn)
    GPIO_WritePin(g_light_relay_port, g_light_relay_pin_mask, RELAY_INACTIVE_LEVEL);
    g_is_light_on = false;
}

void LightService_TurnOn(void) {
    if (g_light_relay_port != NULL) {
        GPIO_WritePin(g_light_relay_port, g_light_relay_pin_mask, RELAY_ACTIVE_LEVEL);
        g_is_light_on = true;
    }
}

void LightService_TurnOff(void) {
    if (g_light_relay_port != NULL) {
        GPIO_WritePin(g_light_relay_port, g_light_relay_pin_mask, RELAY_INACTIVE_LEVEL);
        g_is_light_on = false;
    }
}

bool LightService_GetState(void) {
    return g_is_light_on;
}
