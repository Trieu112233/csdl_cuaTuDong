/*
 * door_fsm.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include <stdio.h>
#include "door_fsm.h"
#include "pir_sensor_service.h"
#include "limit_switch_service.h"
#include "motor_control_service.h"
#include "systick_driver.h"


static float motor_speed = 100.0f;
static DoorState_t    g_current_door_state;
static SystemOpMode_t g_current_system_mode = SYSTEM_MODE_NORMAL;
static uint32_t       g_state_timer_start_tick = 0;

// --- Khai báo trước các hàm on_entry và process cho mỗi trạng thái ---
static void on_entry_closed(void);
static void process_closed(void);

static void on_entry_opening(void);
static void process_opening(void);

static void on_entry_open(void);
static void process_open(void);

static void on_entry_closing(void);
static void process_closing(void);

static void on_entry_error(void);
static void process_error(void);

// --- Hàm tiện ích kiểm tra Timeout (chống tràn GetTick) ---
static bool is_timeout(uint32_t start_tick, uint32_t timeout_ms) {
    uint32_t current_tick = GetTick();
    if (current_tick >= start_tick) {
        return (current_tick - start_tick) >= timeout_ms;
    } else { // Xử lý trường hợp GetTick() bị tràn số
        return (UINT32_MAX - start_tick + current_tick + 1) >= timeout_ms;
    }
}

// --- Hàm thay đổi trạng thái ---
static void change_door_state(DoorState_t new_state) {
    if (g_current_door_state == new_state) {
        return;
    }

    g_current_door_state = new_state;

    // Gọi on_entry cho trạng thái mới
    switch (g_current_door_state) {
        case DOOR_STATE_CLOSED:  on_entry_closed();  break;
        case DOOR_STATE_OPENING: on_entry_opening(); break;
        case DOOR_STATE_OPEN:    on_entry_open();    break;
        case DOOR_STATE_CLOSING: on_entry_closing(); break;
        case DOOR_STATE_ERROR:   on_entry_error();   break;
        default:
            // Trường hợp không mong muốn, vào ERROR
            on_entry_error();
            break;
    }
}

// --- Định nghĩa các hàm on_entry và process ---

// CLOSED State
static void on_entry_closed(void) {
    Motor_Stop();
}
static void process_closed(void) {
}

// OPENING State
static void on_entry_opening(void) {
    Motor_SetDirection(MOTOR_FORWARD);
    Motor_SetSpeed(motor_speed);
    g_state_timer_start_tick = GetTick(); // Bắt đầu timer cho thời gian di chuyển tối đa
}
static void process_opening(void) {
    if (LimitSwitchService_IsDoorFullyOpen()) {
        change_door_state(DOOR_STATE_OPEN);
        return; // Đã chuyển trạng thái
    }
    if (is_timeout(g_state_timer_start_tick, DOOR_MAX_TRAVEL_TIME_MS)) {
        change_door_state(DOOR_STATE_ERROR);
        return;
    }
}

// OPEN State
static void on_entry_open(void) {
    Motor_Stop();
    g_state_timer_start_tick = GetTick(); // Bắt đầu timer cho tự động đóng cửa
}
static void process_open(void) {
    if (g_current_system_mode == SYSTEM_MODE_NORMAL) {
        bool is_motion_detected = PIRService_IsMotionDetected(PIR_SENSOR_IN) ||
                                  PIRService_IsMotionDetected(PIR_SENSOR_OUT);
        if (is_motion_detected) {
            g_state_timer_start_tick = GetTick(); // Reset timer tự động đóng nếu có người
        } else {
            if (is_timeout(g_state_timer_start_tick, DOOR_AUTO_CLOSE_TIMEOUT_MS)) {
                change_door_state(DOOR_STATE_CLOSING);
                return;
            }
        }
    }
}

// CLOSING State
static void on_entry_closing(void) {
    Motor_SetDirection(MOTOR_REVERSE);
    Motor_SetSpeed(motor_speed);
    g_state_timer_start_tick = GetTick(); // Bắt đầu timer cho thời gian di chuyển tối đa
}
static void process_closing(void) {
    if (LimitSwitchService_IsDoorFullyClosed()) {
        change_door_state(DOOR_STATE_CLOSED);
        return;
    }
    if (is_timeout(g_state_timer_start_tick, DOOR_MAX_TRAVEL_TIME_MS)) {
        change_door_state(DOOR_STATE_ERROR);
        return;
    }
}

// ERROR State
static void on_entry_error(void) {
    Motor_Stop();
}
static void process_error(void) {
}


// --- Các hàm public của FSM ---
void DoorFSM_Init(void) {
    // Xác định trạng thái ban đầu dựa trên công tắc hành trình
    // và gọi on_entry cho trạng thái đó.
    if (LimitSwitchService_IsDoorFullyClosed()) {
        g_current_door_state = DOOR_STATE_CLOSED; // Gán trước khi gọi on_entry
        on_entry_closed();
    } else if (LimitSwitchService_IsDoorFullyOpen()) {
        g_current_door_state = DOOR_STATE_OPEN;
        on_entry_open();
    } else {
        // Nếu không rõ ràng, mặc định là ERROR
        g_current_door_state = DOOR_STATE_ERROR;
        on_entry_error();
    }
}

void DoorFSM_Process(void) {
    // Xử lý System Mode (ưu tiên cao nhất, trừ khi đang ở ERROR và mode không phải FORCE)
    if (g_current_door_state != DOOR_STATE_ERROR) { // Nếu không lỗi, các FORCE mode có hiệu lực
        if (g_current_system_mode == SYSTEM_MODE_FORCE_OPEN) {
            if (g_current_door_state != DOOR_STATE_OPEN && g_current_door_state != DOOR_STATE_OPENING) {
                change_door_state(DOOR_STATE_OPENING);
            }
        } else if (g_current_system_mode == SYSTEM_MODE_FORCE_CLOSE) {
            if (g_current_door_state != DOOR_STATE_CLOSED && g_current_door_state != DOOR_STATE_CLOSING) {
                change_door_state(DOOR_STATE_CLOSING);
            }
        }
    } else { // Nếu đang ở DOOR_STATE_ERROR
        if (g_current_system_mode == SYSTEM_MODE_FORCE_OPEN) {
            change_door_state(DOOR_STATE_OPENING);
        } else if (g_current_system_mode == SYSTEM_MODE_FORCE_CLOSE) {
            change_door_state(DOOR_STATE_CLOSING);
        }
    }

    // Xử lý PIR ở NORMAL mode (nếu chưa bị FORCE mode override)
    // Logic này có thể làm cửa đang ĐÓNG chuyển sang MỞ.
    if (g_current_system_mode == SYSTEM_MODE_NORMAL && g_current_door_state != DOOR_STATE_ERROR) {
        bool is_motion_detected = PIRService_IsMotionDetected(PIR_SENSOR_IN) ||
                                  PIRService_IsMotionDetected(PIR_SENSOR_OUT);
        if (is_motion_detected) {
            if (g_current_door_state == DOOR_STATE_CLOSED) {
                change_door_state(DOOR_STATE_OPENING);
            } else if (g_current_door_state == DOOR_STATE_CLOSING) {
                // Nếu đang đóng mà phát hiện người, ưu tiên mở lại
                change_door_state(DOOR_STATE_OPENING);
            }
        }
    }

    // --- Thực thi logic của trạng thái hiện tại ---
    switch (g_current_door_state) {
        case DOOR_STATE_CLOSED:  process_closed();  break;
        case DOOR_STATE_OPENING: process_opening(); break;
        case DOOR_STATE_OPEN:    process_open();    break;
        case DOOR_STATE_CLOSING: process_closing(); break;
        case DOOR_STATE_ERROR:   process_error();   break;
        default:
            change_door_state(DOOR_STATE_ERROR);
            process_error(); // Xử lý trạng thái lỗi ngay
            break;
    }
}

DoorState_t DoorFSM_GetState(void) {
    return g_current_door_state;
}

void DoorFSM_NotifySystemModeChange(SystemOpMode_t new_mode) {
    g_current_system_mode = new_mode;
}


