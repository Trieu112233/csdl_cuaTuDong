/*
 * system_manager.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#ifndef INC_APPLICATIONS_SYSTEM_MANAGER_H_
#define INC_APPLICATIONS_SYSTEM_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "uart_protocol_service.h"
#include "door_fsm.h"

// Thời gian gửi status update định kỳ lên LabVIEW (ms)
#define STATUS_UPDATE_INTERVAL_MS 2000 // Gửi mỗi 2 giây

/**
 * @brief Initializes the System Manager module.
 *        This function should initialize all other relevant application modules and services.
 */
void SystemManager_Init(void);

/**
 * @brief Main processing loop for the System Manager.
 *        Should be called periodically in the main application loop.
 *        Handles tasks like sending periodic status updates.
 */
void SystemManager_Process(void);

/**
 * @brief Callback function to handle commands received from LabVIEW via UART.
 *        This function is registered with UARTProto_Init.
 * @param frame: Pointer to the parsed frame containing the command.
 * @return true if the command was understood and accepted for processing, false otherwise.
 *         (uart_protocol_service sẽ không gửi NACK dựa trên giá trị này nữa).
 */
bool SystemManager_HandleLabVIEWCommand(const ParsedFrame_t* frame);

/**
 * @brief Gets the current operating mode of the system.
 * @return Current SystemOpMode_t.
 */
SystemOpMode_t SystemManager_GetCurrentMode(void);

/**
 * @brief Notifies the System Manager about a door error event.
 *        (Ví dụ, được gọi bởi DoorFSM khi vào trạng thái ERROR)
 */
 void SystemManager_NotifyDoorError(void);

#endif /* INC_APPLICATIONS_SYSTEM_MANAGER_H_ */
