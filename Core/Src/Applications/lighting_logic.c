/*
 * lighting_logic.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include "lighting_logic.h"
#include "people_counter.h"
#include "light_control_service.h"
#include "uart_protocol_service.h"

void LightingLogic_Init(void) {
    int16_t current_person_count = PeopleCounter_GetCount();

    if (current_person_count > 0) {
        LightService_TurnOn();
    } else {
        LightService_TurnOff();
    }
}

void LightingLogic_Process(void) {
    int16_t current_person_count = PeopleCounter_GetCount();
    bool current_light_hw_state = LightService_GetState(); // Lấy trạng thái thực tế của đèn từ service

    if (current_person_count > 0) {
        if (!current_light_hw_state) {
            LightService_TurnOn();
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_LIGHT_STATE, PAYLOAD_LIGHT_ON, 0);
        }
    } else {
        if (current_light_hw_state) {
            LightService_TurnOff();
            UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_LIGHT_STATE, PAYLOAD_LIGHT_OFF, 0);
        }
    }
}

// Hàm trả về trạng thái logic mà module này muốn đèn có.
bool LightingLogic_IsLightIntendedToBeOn(void) {
    if (PeopleCounter_GetCount() > 0) {
        return true;
    } else {
        return false;
    }
}
