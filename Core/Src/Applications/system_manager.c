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
#define PIR_IN_PORT       		GPIOC
#define PIR_IN_PIN        		6
#define PIR_OUT_PORT       		GPIOC
#define PIR_OUT_PIN        		5
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

// Light Control
#define LIGHT_RELAY_PORT        GPIOA
#define LIGHT_RELAY_PIN         9 // Ví dụ


static SystemOpMode_t g_system_op_mode = SYSTEM_MODE_NORMAL;
static uint32_t g_last_status_update_tick = 0;


// Callback được people_counter gọi khi có người đi qua
void app_person_passed_handler(PersonPassedDirection_t direction) {
    if (g_system_op_mode == SYSTEM_MODE_NORMAL) {
        // Chỉ thông báo cho FSM nếu đang ở chế độ NORMAL
        DoorFSM_NotifyPersonDetectedPassing();
    }
    // Có thể gửi ngay một status update nếu cần
}


void SystemManager_Init(void) {
    UARTProto_Init(SystemManager_HandleLabVIEWCommand);

    PIRService_Init(PIR_IN_PORT, PIR_IN_PIN, PIR_OUT_PORT, PIR_OUT_PIN, PIR_IRQ_PRIO);
    LimitSwitchService_Init(LS_OPEN_PORT, LS_OPEN_PIN, LS_CLOSED_PORT, LS_CLOSED_PIN, LS_IRQ_PRIO);
    Motor_Init(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, MOTOR_DIR2_PORT, MOTOR_DIR2_PIN);
    LightService_Init(LIGHT_RELAY_PORT, LIGHT_RELAY_PIN);

    PeopleCounter_Init(app_person_passed_handler);
    DoorFSM_Init();
    LightingLogic_Init();

    g_system_op_mode = SYSTEM_MODE_NORMAL; // Đặt chế độ mặc định
    DoorFSM_NotifySystemModeChange(g_system_op_mode); // Thông báo cho FSM
    g_last_status_update_tick = GetTick();
}

void SystemManager_Process(void) {
    UARTProto_Process();
    LimitSwitchService_ProcessDebounce();
    PeopleCounter_Process();
    DoorFSM_Process();
    LightingLogic_Process();

    // Gửi status update định kỳ
    if ((GetTick() - g_last_status_update_tick) >= STATUS_UPDATE_INTERVAL_MS) {
        g_last_status_update_tick = GetTick();

        // --- Gửi FRAME_ID_STM_FULL_SNAPSHOT ---
        uint8_t snapshot_payload[4];
        snapshot_payload[0] = (uint8_t)g_system_op_mode;
        snapshot_payload[1] = (uint8_t)DoorFSM_GetState();
        snapshot_payload[2] = (uint8_t)PeopleCounter_GetCount();
        snapshot_payload[3] = LightService_GetState() ? PAYLOAD_LIGHT_ON : PAYLOAD_LIGHT_OFF;

        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW,
                              FRAME_ID_STM_FULL_SNAPSHOT,
                              snapshot_payload,
                              4);
    }
}

bool SystemManager_HandleLabVIEWCommand(const ParsedFrame_t* frame) {
    if (frame->type != FRAME_TYPE_LABVIEW_TO_STM) {
        return false; // Không phải lệnh, bỏ qua
    }

    bool cmd_processed_ok = false;

    switch (frame->id) {
        case FRAME_ID_LABVIEW_SET_MODE:
            if (frame->length == 1) {
                SystemOpMode_t requested_mode = (SystemOpMode_t)frame->payload[0];
                // Kiểm tra xem mode có hợp lệ không
                if (requested_mode == SYSTEM_MODE_NORMAL ||
                    requested_mode == SYSTEM_MODE_FORCE_OPEN ||
                    requested_mode == SYSTEM_MODE_FORCE_CLOSE) {

                    g_system_op_mode = requested_mode;
                    DoorFSM_NotifySystemModeChange(g_system_op_mode);
                    cmd_processed_ok = true;
                } else {
                    cmd_processed_ok = false; // Mode không hợp lệ
                }
            } else {
                cmd_processed_ok = false; // Payload length sai
            }
            break;

        case FRAME_ID_LABVIEW_RESET_COUNT:
            if (frame->length == 0) { // Lệnh này không cần payload
                PeopleCounter_Reset();
                cmd_processed_ok = true;
            } else {
                cmd_processed_ok = false; // Có payload không mong muốn
            }
            break;

        default:
            cmd_processed_ok = false; // ID lệnh không xác định
            break;
    }
    return cmd_processed_ok;
}

SystemOpMode_t SystemManager_GetCurrentMode(void) {
    return g_system_op_mode;
}

