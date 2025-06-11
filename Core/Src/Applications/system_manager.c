/*
 * system_manager.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include "system_manager.h"
#include "uart_protocol_service.h"
#include "pir_sensor_service.h"
#include "limit_switch_service.h"
#include "motor_control_service.h"
#include "light_control_service.h"
#include "people_counter.h"
#include "door_fsm.h"
#include "lighting_logic.h"
#include "systick_driver.h"
#include "rcc_config.h"

// PIR Sensors
#define PIR_IN_PORT       		GPIOB
#define PIR_IN_PIN        		2
#define PIR_OUT_PORT       		GPIOB
#define PIR_OUT_PIN        		1
#define PIR_IRQ_PRIO       		10

// Limit Switches
#define LS_OPEN_PORT        	GPIOB
#define LS_OPEN_PIN         	4
#define LS_CLOSED_PORT      	GPIOB
#define LS_CLOSED_PIN       	5
#define LS_IRQ_PRIO         	11

#define MOTOR_DIR1_PORT         GPIOB
#define MOTOR_DIR1_PIN          8
#define MOTOR_DIR2_PORT         GPIOB
#define MOTOR_DIR2_PIN          9
// PWM: PA6 (đã được cấu hình trong Motor_Init dùng PWM)

// Light Control
#define LIGHT_RELAY_PORT        GPIOA
#define LIGHT_RELAY_PIN         9

static SystemOpMode_t g_prev_system_op_mode;
static SystemOpMode_t g_cur_system_op_mode;

static uint8_t g_prev_perCnt;
static uint8_t g_cur_perCnt;

static uint8_t g_prev_light_state_payload; // Lưu trữ payload (PAYLOAD_LIGHT_ON/OFF)
static uint8_t g_cur_light_state_payload;

static DoorState_t g_prev_door_state;
static DoorState_t g_current_door_state;

static uint32_t g_periodic_snapshot_timer_start_tick;

// Hàm kiểm tra timeout an toàn (chống tràn GetTick)
static bool is_timeout_sm(uint32_t start_tick, uint32_t timeout_ms) {
    uint32_t current_tick = GetTick();
    if (current_tick >= start_tick) {
        return (current_tick - start_tick) >= timeout_ms;
    } else { // Xử lý trường hợp GetTick() bị tràn số
        return (UINT32_MAX - start_tick + current_tick + 1) >= timeout_ms;
    }
}

void SendFrameToLabVIEWProcess(void) {
    // Gửi trạng thái cửa nếu thay đổi
    if (g_current_door_state != g_prev_door_state)  {
        uint8_t door_state_payload = (uint8_t)g_current_door_state;
        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_DOOR_STATE, // Sửa FRAME ID
                                    &door_state_payload, 1);
        g_prev_door_state = g_current_door_state;
    } 

    // Gửi số người nếu thay đổi
    if (g_cur_perCnt != g_prev_perCnt) {
        // g_cur_perCnt đã là uint8_t
        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_PERSON_COUNT,
                            &g_cur_perCnt, 1);
        g_prev_perCnt = g_cur_perCnt;
    }

    // Gửi trạng thái đèn nếu thay đổi
    // g_cur_light_state_payload đã là payload (PAYLOAD_LIGHT_ON/OFF)
    if (g_cur_light_state_payload != g_prev_light_state_payload) {
        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_LIGHT_STATE, 
                            &g_cur_light_state_payload, 1);
        g_prev_light_state_payload = g_cur_light_state_payload;
    }

    // Gửi chế độ hệ thống nếu thay đổi
    if (g_cur_system_op_mode != g_prev_system_op_mode) {
        uint8_t mode_payload = (uint8_t)g_cur_system_op_mode;
        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_SYSTEM_MODE, 
                            &mode_payload, 1);
        g_prev_system_op_mode = g_cur_system_op_mode;
    }
}

void SystemManager_Init(void) {
    // Khởi tạo các services và drivers
    UARTProto_Init(SystemManager_HandleLabVIEWCommand);
    PIRService_Init(PIR_IN_PORT, PIR_IN_PIN, PIR_OUT_PORT, PIR_OUT_PIN, PIR_IRQ_PRIO);
    LimitSwitchService_Init(LS_OPEN_PORT, LS_OPEN_PIN, LS_CLOSED_PORT, LS_CLOSED_PIN, LS_IRQ_PRIO);
    Motor_Init(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, MOTOR_DIR2_PORT, MOTOR_DIR2_PIN);
    LightService_Init(LIGHT_RELAY_PORT, LIGHT_RELAY_PIN);

    // Khởi tạo các modules ứng dụng
    PeopleCounter_Init();
    DoorFSM_Init();
    LightingLogic_Init();

    // Thiết lập trạng thái ban đầu cho System Manager
    g_cur_system_op_mode = SYSTEM_MODE_NORMAL;
    g_prev_system_op_mode = g_cur_system_op_mode;
    DoorFSM_NotifySystemModeChange(g_cur_system_op_mode);

    g_cur_perCnt = PeopleCounter_GetCount();
    g_prev_perCnt = g_cur_perCnt;

    g_cur_light_state_payload = LightingLogic_IsLightIntendedToBeOn() ? PAYLOAD_LIGHT_ON : PAYLOAD_LIGHT_OFF;
    g_prev_light_state_payload = g_cur_light_state_payload;

    g_current_door_state = DoorFSM_GetState();
    g_prev_door_state = g_current_door_state;

    // Gửi trạng thái ban đầu của hệ thống
    uint8_t initial_status_payload[4];
    initial_status_payload[0] = (uint8_t) g_cur_system_op_mode;
    initial_status_payload[1] = (uint8_t) g_current_door_state;
    initial_status_payload[2] = g_cur_perCnt;
    initial_status_payload[3] = g_cur_light_state_payload;
    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_FULL_SNAPSHOT, initial_status_payload, 4);

    g_periodic_snapshot_timer_start_tick = GetTick();
}

void SystemManager_Process(void) {
    // Xử lý input và các tác vụ nền của services
    UARTProto_Process(); // Xử lý byte nhận được, gọi SystemManager_HandleLabVIEWCommand nếu có frame hoàn chỉnh
    LimitSwitchService_ProcessDebounce(); // Xử lý debounce cho công tắc hành trình

    // Xử lý logic của các modules ứng dụng
    PeopleCounter_Process();
    DoorFSM_Process();       // FSM cửa dựa trên mode, PIR, limit switch, timers
    LightingLogic_Process(); // Logic đèn dựa trên số người

    // Cập nhật trạng thái hiện tại từ các modules
    g_cur_perCnt = PeopleCounter_GetCount();
    g_cur_light_state_payload = LightingLogic_IsLightIntendedToBeOn() ? PAYLOAD_LIGHT_ON : PAYLOAD_LIGHT_OFF;
    g_current_door_state = DoorFSM_GetState();

    // Gửi các thay đổi trạng thái lên LabVIEW
    SendFrameToLabVIEWProcess();

    // Gửi full snapshot định kỳ
    if (is_timeout_sm(g_periodic_snapshot_timer_start_tick, STATUS_UPDATE_INTERVAL_MS)) {
        uint8_t snapshot_payload[4];
        snapshot_payload[0] = (uint8_t) g_cur_system_op_mode;
        snapshot_payload[1] = (uint8_t) g_current_door_state;
        snapshot_payload[2] = g_cur_perCnt;
        snapshot_payload[3] = g_cur_light_state_payload;

        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_FULL_SNAPSHOT, snapshot_payload, 4);
        g_periodic_snapshot_timer_start_tick = GetTick(); // Reset timer
    }
}

bool SystemManager_HandleLabVIEWCommand(const ParsedFrame_t* frame) {
    if (frame->type != FRAME_TYPE_LABVIEW_TO_STM) {
        return false; // Không phải frame lệnh cho STM
    }

    uint8_t command_id_for_ack_nack = frame->id; // Dùng để gửi lại trong payload của ACK/NACK

    switch (frame->id) {
        case FRAME_ID_LABVIEW_SET_MODE:
            if (frame->length == 1) {
                SystemOpMode_t requested_mode = (SystemOpMode_t)frame->payload[0];
                if (requested_mode == SYSTEM_MODE_NORMAL ||
                    requested_mode == SYSTEM_MODE_FORCE_OPEN ||
                    requested_mode == SYSTEM_MODE_FORCE_CLOSE) {
                    
                    g_cur_system_op_mode = requested_mode; // Cập nhật mode trước
                    DoorFSM_NotifySystemModeChange(g_cur_system_op_mode); // Thông báo cho FSM
                    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_COMMAND_ACK, &command_id_for_ack_nack, 1);
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }

        case FRAME_ID_LABVIEW_RESET_COUNT:
            if (frame->length == 0) {
                PeopleCounter_Reset();
                UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_COMMAND_ACK, &command_id_for_ack_nack, 1);
                return true;
            } else {
                return false;
            }

        default:
            return false;
    }
}

SystemOpMode_t SystemManager_GetCurrentMode(void) {
    return g_cur_system_op_mode;
}


