/*
 * door_fsm.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include "door_fsm.h"
#include "system_manager.h"
#include "people_counter.h"
#include "pir_sensor_service.h"
#include "limit_switch_service.h"
#include "motor_control_service.h"
#include "systick_driver.h"

static DoorState_t    g_current_door_state = DOOR_STATE_INIT;
static DoorState_t    g_previous_door_state = DOOR_STATE_INIT;
static SystemOpMode_t g_current_system_mode = SYSTEM_MODE_NORMAL;
static uint32_t g_state_timer_start_tick = 0;

// --- Forward declarations cho các hàm enter_state và process_state ---
static void process_state_closed(bool just_enter_state);
static void process_state_opening(bool just_enter_state);
static void process_state_open(bool just_enter_state);
static void process_state_closing(bool just_enter_state);
static void process_state_error(bool just_enter_state);

static void change_door_state(DoorState_t new_state) {
    if(g_current_door_state != new_state) {
        g_previous_door_state = g_current_door_state; // Lưu trạng thái cũ
        g_current_door_state = new_state; // Cập nhật trạng thái mới
        g_state_timer_start_tick = GetTick(); // Reset timer khi chuyển trạng thái
    }

}

void DoorFSM_Init(void) {
    if (LimitSwitchService_IsDoorFullyClosed()) {
        g_current_door_state = DOOR_STATE_CLOSED;
    } else if (LimitSwitchService_IsDoorFullyOpen()) {
        g_current_door_state = DOOR_STATE_OPEN;
    } else {
        g_current_door_state = DOOR_STATE_ERROR; // Nếu không rõ trạng thái, chuyển sang ERROR
    }
    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, (uint8_t*)&g_current_door_state, 1);
    g_previous_door_state = DOOR_STATE_INIT; // Khởi tạo trạng thái trước 
    g_state_timer_start_tick = GetTick(); // Bắt đầu timer
}

void DoorFSM_Process(void) {
    bool just_enter_state = (g_current_door_state != g_previous_door_state);
    if(just_enter_state){
        g_previous_door_state = g_current_door_state; 
    }

    // Xử lý ưu tiên: nếu đang ở chế độ FORCE_OPEN hoặc FORCE_CLOSE, cần xử lý trước
    if (g_current_system_mode == SYSTEM_MODE_FORCE_OPEN) {
        if (g_current_door_state != DOOR_STATE_OPEN && g_current_door_state != DOOR_STATE_OPENING) {
            if (g_current_door_state != DOOR_STATE_ERROR) { 
                 change_door_state(DOOR_STATE_OPENING);
            }
        }
    } else if (g_current_system_mode == SYSTEM_MODE_FORCE_CLOSE) {
        if (g_current_door_state != DOOR_STATE_CLOSED && g_current_door_state != DOOR_STATE_CLOSING) {
             if (g_current_door_state != DOOR_STATE_ERROR) {
                change_door_state(DOOR_STATE_CLOSING);
             }
        }
    }

    // Logic xử lý trạng thái hiện tại
    switch (g_current_door_state) {
        case DOOR_STATE_INIT:
            DoorFSM_Init();
            break;

        case DOOR_STATE_CLOSED:
            process_state_closed(just_enter_state);
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, (uint8_t*)&g_current_door_state, 1);
            break;

        case DOOR_STATE_OPENING:
            process_state_opening(just_enter_state);
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, (uint8_t*)&g_current_door_state, 1);
            break;

        case DOOR_STATE_OPEN:           
            process_state_open(just_enter_state);
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, (uint8_t*)&g_current_door_state, 1);
            break;

        case DOOR_STATE_CLOSING:
            process_state_closing(just_enter_state);
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, (uint8_t*)&g_current_door_state, 1);    
            break;

        case DOOR_STATE_ERROR:
            process_state_error(just_enter_state);
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, (uint8_t*)&g_current_door_state, 1);
            break;

        default:
            break;
    }
}

DoorState_t DoorFSM_GetState(void) { 
    return g_current_door_state; 
}

void DoorFSM_NotifySystemModeChange(SystemOpMode_t new_mode) { 
    SystemOpMode_t old_mode = g_current_system_mode;
    g_current_system_mode = new_mode;

    if ((old_mode == SYSTEM_MODE_FORCE_OPEN || old_mode == SYSTEM_MODE_FORCE_CLOSE) && new_mode == SYSTEM_MODE_NORMAL) {
        if (g_current_door_state == DOOR_STATE_OPEN) {
            g_state_timer_start_tick = GetTick(); // Reset auto-close timer
        }
    }

    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_SYSTEM_MODE, (uint8_t*)&g_current_system_mode, 1);
}

void DoorFSM_NotifyPersonDetectedPassing(void) { 
    if (g_current_system_mode == SYSTEM_MODE_NORMAL) {
        if (g_current_door_state == DOOR_STATE_CLOSED) {
            change_door_state(DOOR_STATE_OPENING);
        } else if (g_current_door_state == DOOR_STATE_CLOSING) {
            change_door_state(DOOR_STATE_OPENING); // Mở lại ngay nếu đang đóng mà có người đi qua
        } else if (g_current_door_state == DOOR_STATE_OPEN) {
            g_state_timer_start_tick = GetTick(); // Reset auto-close timer
        }
    }
}

static void process_state_closed(bool just_enter_state) {
    if (just_enter_state) {
        Motor_Stop();
    }

    if (g_current_system_mode == SYSTEM_MODE_FORCE_OPEN) {
        change_door_state(DOOR_STATE_OPENING);
    } 
}

static void process_state_opening(bool just_enter_state) {
    if (just_enter_state) {
        Motor_SetDirection(MOTOR_FORWARD); // Đặt hướng mở cửa
        Motor_SetSpeed(100.0f); // Bắt đầu mở cửa
    }

    if (LimitSwitchService_IsDoorFullyOpen()) {
        change_door_state(DOOR_STATE_OPEN);
    } else if (GetTick() - g_state_timer_start_tick > DOOR_MAX_TRAVEL_TIME_MS) {
        change_door_state(DOOR_STATE_ERROR); // Quá thời gian mở cửa
    }
}

static void process_state_open(bool just_enter_state) {
    if (just_enter_state) {
        Motor_Stop(); // Dừng động cơ khi đã mở
        g_state_timer_start_tick = GetTick(); // Reset auto-close timer
    }

    if (g_current_system_mode == SYSTEM_MODE_FORCE_CLOSE) {
        change_door_state(DOOR_STATE_CLOSING);
    } else if (g_current_system_mode == SYSTEM_MODE_NORMAL) {
        bool is_person_detected = PIRService_IsMotionDetected(PIR_SENSOR_IN) || PIRService_IsMotionDetected(PIR_SENSOR_OUT);
        if (is_person_detected) {
            g_state_timer_start_tick = GetTick(); // Reset auto-close timer nếu có người
        } else {
            if (GetTick() - g_state_timer_start_tick > DOOR_AUTO_CLOSE_TIMEOUT_MS) {
            change_door_state(DOOR_STATE_CLOSING); // Tự động đóng cửa sau thời gian quy định
           }
        }
    }
}

static void process_state_closing(bool just_enter_state) {
    if (just_enter_state) {
        Motor_SetDirection(MOTOR_REVERSE); // Đặt hướng đóng cửa
        Motor_SetSpeed(100.0f); // Bắt đầu đóng cửa
    }

    if (g_current_system_mode == SYSTEM_MODE_NORMAL) {
        bool is_person_detected = PIRService_IsMotionDetected(PIR_SENSOR_IN) || PIRService_IsMotionDetected(PIR_SENSOR_OUT);
        if (is_person_detected) {
            change_door_state(DOOR_STATE_OPENING); // Nếu có người, mở cửa lại
            return;
        }
    }

    if (LimitSwitchService_IsDoorFullyClosed()) {
        change_door_state(DOOR_STATE_CLOSED);
    } else if (GetTick() - g_state_timer_start_tick > DOOR_MAX_TRAVEL_TIME_MS) {
        change_door_state(DOOR_STATE_ERROR); // Quá thời gian đóng cửa
    }
}

static void process_state_error(bool just_enter_state) {
    if (just_enter_state) {
        Motor_Stop(); // Dừng động cơ khi ở trạng thái lỗi
    }

    if (g_current_system_mode == SYSTEM_MODE_FORCE_OPEN) {
        change_door_state(DOOR_STATE_OPENING); // Chuyển sang mở cửa nếu ở chế độ FORCE_OPEN
    } else if (g_current_system_mode == SYSTEM_MODE_FORCE_CLOSE) {
        change_door_state(DOOR_STATE_CLOSING); // Chuyển sang đóng cửa nếu ở chế độ FORCE_CLOSE
    }
    // Ở trạng thái lỗi, không làm gì cả, chờ người dùng can thiệp
    // Có thể thêm logic reset nếu cần
}
