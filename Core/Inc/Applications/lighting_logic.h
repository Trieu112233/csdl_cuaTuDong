/*
 * lighting_logic.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#ifndef INC_APPLICATIONS_LIGHTING_LOGIC_H_
#define INC_APPLICATIONS_LIGHTING_LOGIC_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes the Lighting Logic module.
 */
void LightingLogic_Init(void);

/**
 * @brief Processes the lighting logic based on the number of people in the room.
 *        This function should be called periodically in the main application loop.
 */
void LightingLogic_Process(void);

/**
 * @brief Gets the current intended state of the light (ON or OFF) based on the logic.
 * @return true if the logic dictates the light should be ON, false otherwise.
 * @note This might not be the actual hardware state if light_control_service call failed.
 *       It reflects the desired state by this module.
 */
bool LightingLogic_IsLightIntendedToBeOn(void); // Tùy chọn, nếu muốn biết trạng thái logic

#endif /* INC_APPLICATIONS_LIGHTING_LOGIC_H_ */
