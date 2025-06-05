/*
 * people_counter.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include "people_counter.h"
#include "pir_sensor_service.h"
#include "systick_driver.h"
#include "uart_protocol_service.h"

static volatile int16_t g_person_count = 0;

// Máy trạng thái cho việc phát hiện hướng
typedef enum {
    DETECTION_STATE_IDLE,
    DETECTION_STATE_EXPECTING_PIR_INSIDE, // PIR_OUTSIDE đã active, chờ PIR_INSIDE
    DETECTION_STATE_EXPECTING_PIR_OUTSIDE // PIR_INSIDE đã active, chờ PIR_OUTSIDE
} DetectionState_t;

static DetectionState_t g_detection_state = DETECTION_STATE_IDLE;
static uint32_t g_first_pir_activation_tick = 0;
static uint32_t g_last_detection_time = 0; // Thời điểm lần đếm cuối cùng

static bool g_pir_outside_last_state = false;
static bool g_pir_inside_last_state = false;

static person_passed_callback_t g_person_passed_cb = NULL;

void PeopleCounter_Init(person_passed_callback_t callback) {
    g_person_count = 0;
    g_detection_state = DETECTION_STATE_IDLE;
    g_first_pir_activation_tick = 0;
    g_last_detection_time = 0; // Để cooldown hoạt động ngay từ đầu nếu cần
    g_person_passed_cb = callback;

    // Khởi tạo trạng thái PIR ban đầu
    g_pir_outside_last_state = PIRService_IsMotionDetected(PIR_SENSOR_OUT);
    g_pir_inside_last_state = PIRService_IsMotionDetected(PIR_SENSOR_IN);

    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_PERSON_COUNT, (uint8_t*)&g_person_count, 1);
}

void PeopleCounter_Process(void) {
    bool pir_outside_current_state = PIRService_IsMotionDetected(PIR_SENSOR_OUT);
    bool pir_inside_current_state = PIRService_IsMotionDetected(PIR_SENSOR_IN);
    uint32_t current_tick = GetTick();

    // Phát hiện sườn lên (rising edge)
    bool pir_outside_triggered = (pir_outside_current_state && !g_pir_outside_last_state);
    bool pir_inside_triggered = (pir_inside_current_state && !g_pir_inside_last_state);

    // Cooldown: Nếu vừa mới đếm xong, bỏ qua các trigger mới trong một khoảng thời gian
    if ((current_tick - g_last_detection_time) < PERSON_COUNTER_COOLDOWN_MS) {
        // Cập nhật trạng thái cuối cùng để không bỏ lỡ sườn lên sau cooldown
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

        case DETECTION_STATE_EXPECTING_PIR_INSIDE: // PIR_OUTSIDE đã active, chờ PIR_INSIDE
            if (pir_inside_triggered) { // PIR_INSIDE cũng active -> Người vào
                g_person_count++;
                UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_PERSON_COUNT, (uint8_t*)&g_person_count, 1);
                g_detection_state = DETECTION_STATE_IDLE;
                g_last_detection_time = current_tick;
                if (g_person_passed_cb) g_person_passed_cb(PERSON_PASSED_ENTERED);
            } else if ((current_tick - g_first_pir_activation_tick) > PERSON_COUNTER_MAX_TRANSITION_TIME_MS) {
                // Hết thời gian chờ mà PIR_INSIDE không active -> Reset
                g_detection_state = DETECTION_STATE_IDLE;
            } else if (!pir_outside_current_state && !pir_inside_current_state) {
                // Nếu cả 2 PIR đều tắt trước khi PIR_INSIDE kịp active (hiếm, nhưng có thể)
                // Hoặc nếu người dùng quay lại sau khi kích hoạt PIR_OUTSIDE
                g_detection_state = DETECTION_STATE_IDLE;
            }
            break;

        case DETECTION_STATE_EXPECTING_PIR_OUTSIDE: // PIR_INSIDE đã active, chờ PIR_OUTSIDE
            if (pir_outside_triggered) { // PIR_OUTSIDE cũng active -> Người ra
                if (g_person_count > 0) {
                    g_person_count--;
                    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_PERSON_COUNT, (uint8_t*)&g_person_count, 1);
                }
                g_detection_state = DETECTION_STATE_IDLE;
                g_last_detection_time = current_tick;
                if (g_person_passed_cb) g_person_passed_cb(PERSON_PASSED_EXITED);
            } else if ((current_tick - g_first_pir_activation_tick) > PERSON_COUNTER_MAX_TRANSITION_TIME_MS) {
                // Hết thời gian chờ mà PIR_OUTSIDE không active -> Reset
                g_detection_state = DETECTION_STATE_IDLE;
            } else if (!pir_outside_current_state && !pir_inside_current_state) {
                // Nếu cả 2 PIR đều tắt trước khi PIR_OUTSIDE kịp active
                g_detection_state = DETECTION_STATE_IDLE;
            }
            break;
    }

    // Cập nhật trạng thái cuối cùng của PIR cho lần gọi Process tiếp theo
    g_pir_outside_last_state = pir_outside_current_state;
    g_pir_inside_last_state = pir_inside_current_state;
}

int16_t PeopleCounter_GetCount(void) {
    return g_person_count;
}

void PeopleCounter_Reset(void) {
    // Critical section
    __disable_irq();
    g_person_count = 0;
    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_PERSON_COUNT, (uint8_t*)&g_person_count, 1);
    g_detection_state = DETECTION_STATE_IDLE; // Reset cả máy trạng thái phát hiện
    g_last_detection_time = GetTick(); // Reset cooldown để lệnh reset có hiệu lực ngay
    __enable_irq();
}




