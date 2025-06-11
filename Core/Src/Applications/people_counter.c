/*
 * people_counter.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include "people_counter.h"
#include "pir_sensor_service.h"
#include "systick_driver.h"

static volatile uint8_t g_person_count = 0;

// Máy trạng thái cho việc phát hiện hướng
typedef enum {
    DETECTION_STATE_IDLE,
    DETECTION_STATE_EXPECTING_PIR_INSIDE, // PIR_OUTSIDE đã active, chờ PIR_INSIDE
    DETECTION_STATE_EXPECTING_PIR_OUTSIDE // PIR_INSIDE đã active, chờ PIR_OUTSIDE
} DetectionState_t;

static DetectionState_t g_detection_state = DETECTION_STATE_IDLE;
static uint32_t g_first_pir_activation_tick = 0; // Thời điểm PIR đầu tiên được kích hoạt
static uint32_t g_last_detection_time = 0;       // Thời điểm lần đếm cuối cùng (cho cooldown)

static bool g_pir_outside_last_state = false;
static bool g_pir_inside_last_state = false;

// Hàm kiểm tra timeout an toàn (chống tràn GetTick)
static bool is_timeout_pc(uint32_t start_tick, uint32_t timeout_ms) {
    uint32_t current_tick = GetTick();
    if (start_tick == 0) return false; // Nếu timer chưa bắt đầu, không timeout

    if (current_tick >= start_tick) {
        return (current_tick - start_tick) >= timeout_ms;
    } else { // Xử lý trường hợp GetTick() bị tràn số
        return (UINT32_MAX - start_tick + current_tick + 1) >= timeout_ms;
    }
}

void PeopleCounter_Init(void) { // Không còn tham số callback
    g_person_count = 0;
    g_detection_state = DETECTION_STATE_IDLE;
    g_first_pir_activation_tick = 0;
    g_last_detection_time = 0;

    // Đọc trạng thái PIR ban đầu
    g_pir_outside_last_state = PIRService_IsMotionDetected(PIR_SENSOR_OUT);
    g_pir_inside_last_state = PIRService_IsMotionDetected(PIR_SENSOR_IN);
}

void PeopleCounter_Process(void) {
    bool pir_outside_current_state = PIRService_IsMotionDetected(PIR_SENSOR_OUT);
    bool pir_inside_current_state = PIRService_IsMotionDetected(PIR_SENSOR_IN);
    uint32_t current_tick = GetTick();

    bool pir_outside_triggered = (pir_outside_current_state && !g_pir_outside_last_state);
    bool pir_inside_triggered = (pir_inside_current_state && !g_pir_inside_last_state);

    // Cooldown
    if (g_last_detection_time != 0 && !is_timeout_pc(g_last_detection_time, PERSON_COUNTER_COOLDOWN_MS)) {
        g_pir_outside_last_state = pir_outside_current_state;
        g_pir_inside_last_state = pir_inside_current_state;
        return;
    }

    switch (g_detection_state) {
        case DETECTION_STATE_IDLE:
            if (pir_outside_triggered) {
                g_detection_state = DETECTION_STATE_EXPECTING_PIR_INSIDE;
                g_first_pir_activation_tick = current_tick;
            } else if (pir_inside_triggered) {
                g_detection_state = DETECTION_STATE_EXPECTING_PIR_OUTSIDE;
                g_first_pir_activation_tick = current_tick;
            }
            break;

        case DETECTION_STATE_EXPECTING_PIR_INSIDE: // Chờ PIR_INSIDE sau khi PIR_OUTSIDE active
            if (pir_inside_triggered) {
                g_person_count++;
                // Callback đã bị loại bỏ
                g_detection_state = DETECTION_STATE_IDLE;
                g_last_detection_time = current_tick;
                g_first_pir_activation_tick = 0;
            } else if (is_timeout_pc(g_first_pir_activation_tick, PERSON_COUNTER_MAX_TRANSITION_TIME_MS)) {
                g_detection_state = DETECTION_STATE_IDLE;
                g_first_pir_activation_tick = 0;
            }
            break;

        case DETECTION_STATE_EXPECTING_PIR_OUTSIDE: // Chờ PIR_OUTSIDE sau khi PIR_INSIDE active
            if (pir_outside_triggered) {
                if (g_person_count > 0) {
                    g_person_count--;
                }
                // Callback đã bị loại bỏ
                g_detection_state = DETECTION_STATE_IDLE;
                g_last_detection_time = current_tick;
                g_first_pir_activation_tick = 0;
            } else if (is_timeout_pc(g_first_pir_activation_tick, PERSON_COUNTER_MAX_TRANSITION_TIME_MS)) {
                g_detection_state = DETECTION_STATE_IDLE;
                g_first_pir_activation_tick = 0;
            }
            break;
    }

    g_pir_outside_last_state = pir_outside_current_state;
    g_pir_inside_last_state = pir_inside_current_state;
}

uint8_t PeopleCounter_GetCount(void) {
    return g_person_count;
}

void PeopleCounter_Reset(void) {
    __disable_irq();
    g_person_count = 0;
    g_detection_state = DETECTION_STATE_IDLE;
    g_first_pir_activation_tick = 0;
    g_last_detection_time = 0; // Reset cooldown, cho phép đếm ngay
    __enable_irq();
}




