/*
 * door_fsm.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include <stdio.h>
#include "door_fsm.h"
#include "system_manager.h"
#include "people_counter.h"
#include "pir_sensor_service.h"
#include "limit_switch_service.h"
#include "motor_control_service.h"
#include "systick_driver.h"

static float motor_speed = 100.0f;
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
        g_current_door_state = new_state; // Cập nhật trạng thái mới
        g_state_timer_start_tick = GetTick(); // Reset timer khi chuyển trạng thái
    }
}

void DoorFSM_Init(void) {
    // Xác định trạng thái ban đầu dựa trên công tắc hành trình
    if (LimitSwitchService_IsDoorFullyClosed()) {
        g_current_door_state = DOOR_STATE_CLOSED;
    } else if (LimitSwitchService_IsDoorFullyOpen()) {
        g_current_door_state = DOOR_STATE_OPEN;
    } else {
        g_current_door_state = DOOR_STATE_ERROR;
    }
    g_previous_door_state = g_current_door_state; 

    g_state_timer_start_tick = GetTick(); // Bắt đầu timer
}

void DoorFSM_Process(void) {
    bool just_enter_state = (g_current_door_state != g_previous_door_state);

    // Xử lý FORCE mode - chỉ khi không ở ERROR state
    if (g_current_door_state != DOOR_STATE_ERROR) {
        if (g_current_system_mode == SYSTEM_MODE_FORCE_OPEN) {
            if (g_current_door_state != DOOR_STATE_OPEN && g_current_door_state != DOOR_STATE_OPENING) {
                change_door_state(DOOR_STATE_OPENING);
                just_enter_state = true; // Update flag vì đã thay đổi state
            }
        } else if (g_current_system_mode == SYSTEM_MODE_FORCE_CLOSE) {
            if (g_current_door_state != DOOR_STATE_CLOSED && g_current_door_state != DOOR_STATE_CLOSING) {
                change_door_state(DOOR_STATE_CLOSING);
                just_enter_state = true; // Update flag vì đã thay đổi state
            }
        }
    }

    // Logic xử lý trạng thái hiện tại
    switch (g_current_door_state) {
        case DOOR_STATE_INIT:
            // Trạng thái INIT chỉ để khởi tạo, không xử lý gì thêm
            if (LimitSwitchService_IsDoorFullyClosed()) {
                 change_door_state(DOOR_STATE_CLOSED);
            } else {
                 change_door_state(DOOR_STATE_CLOSING); // Thử đóng lại
            }
            break;

        case DOOR_STATE_CLOSED:
            process_state_closed(just_enter_state);
            break;

        case DOOR_STATE_OPENING:
            process_state_opening(just_enter_state);
            break;

        case DOOR_STATE_OPEN:
            process_state_open(just_enter_state);
            break;

        case DOOR_STATE_CLOSING:
            process_state_closing(just_enter_state);
            break;

        case DOOR_STATE_ERROR:
            process_state_error(just_enter_state);
            break;
    }
    
    g_previous_door_state = g_current_door_state;
}

DoorState_t DoorFSM_GetState(void) { 
    return g_current_door_state; 
}

void DoorFSM_NotifySystemModeChange(SystemOpMode_t new_mode) {
    if (g_current_system_mode != new_mode) {
        g_current_system_mode = new_mode;
    }
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
}

static void process_state_opening(bool just_enter_state) {
    if (just_enter_state) {
        Motor_SetDirection(MOTOR_FORWARD); // Đặt hướng mở cửa
        Motor_SetSpeed(motor_speed); // Bắt đầu mở cửa
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

    // Chỉ xử lý logic NORMAL mode 
    if (g_current_system_mode == SYSTEM_MODE_NORMAL) {
        bool is_person_detected = PIRService_IsMotionDetected(PIR_SENSOR_IN) || 
                                 PIRService_IsMotionDetected(PIR_SENSOR_OUT);
        if (is_person_detected) {
            g_state_timer_start_tick = GetTick();
        } else if (is_timeout(g_state_timer_start_tick, DOOR_AUTO_CLOSE_TIMEOUT_MS)) {
            change_door_state(DOOR_STATE_CLOSING);
        }
    }
}

static void process_state_closing(bool just_enter_state) {
    if (just_enter_state) {
        Motor_SetDirection(MOTOR_REVERSE); // Đặt hướng đóng cửa
        Motor_SetSpeed(motor_speed); // Bắt đầu đóng cửa
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
}
