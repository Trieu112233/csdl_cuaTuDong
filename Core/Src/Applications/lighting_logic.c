/*
 * lighting_logic.c
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#include "lighting_logic.h"
#include "people_counter.h"
#include "light_control_service.h"

void LightingLogic_Init(void) {
    // Trạng thái ban đầu của đèn sẽ được quyết định bởi PeopleCounter_GetCount()
    // và được LightService_TurnOn/Off xử lý.
    // Gọi Process một lần để đảm bảo trạng thái đèn đúng sau khi các module khác đã Init.
    LightingLogic_Process();
}

void LightingLogic_Process(void) {
    uint8_t current_person_count = PeopleCounter_GetCount();
    bool is_light_currently_on_hw = LightService_GetState(); // Lấy trạng thái thực tế của đèn từ service

    if (current_person_count > 0) {
        if (!is_light_currently_on_hw) {
            LightService_TurnOn();
        }
    } else {
        if (is_light_currently_on_hw) {
            LightService_TurnOff();
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
