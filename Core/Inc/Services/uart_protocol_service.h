/*
 * uart_protocol_service.h
 *
 *  Created on: May 28, 2025
 *      Author: Admin
 */

#ifndef INC_SERVICES_UART_PROTOCOL_SERVICE_H_
#define INC_SERVICES_UART_PROTOCOL_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>

#define FRAME_START_BYTE	0x7E
#define FRAME_END_BYTE		0x7F

#define MAX_PAYLOAD_LENGTH	4
#define FRAME_OVERHEAD		5	// START(1) + TYPE(1) + ID(1) + LENGTH(1) + END(1)
#define MAX_FRAME_LENGTH	(MAX_PAYLOAD_LENGTH + FRAME_OVERHEAD)

typedef enum {
    FRAME_TYPE_STM_TO_LABVIEW = 0x01, // Messages sent from STM32 to LabVIEW
    FRAME_TYPE_LABVIEW_TO_STM = 0x02  // Messages sent from LabVIEW to STM32
} FrameType_t;

// IDs for messages (used in the 'id' field of the frame)

// IDs for Type: FRAME_TYPE_STM_TO_LABVIEW (STM32 -> LabVIEW)
#define FRAME_ID_STM_DOOR_STATE       0x01
#define FRAME_ID_STM_LIGHT_STATE      0x02
#define FRAME_ID_STM_PERSON_COUNT     0x03
#define FRAME_ID_STM_SYSTEM_MODE      0x04
#define FRAME_ID_STM_FULL_SNAPSHOT    0x05
#define FRAME_ID_STM_LOG_DEBUG        0x00 // Payload: ASCII string

// Common Payload Values for Door State (for FRAME_ID_STM_DOOR_STATE)
#define PAYLOAD_DOOR_CLOSED         0x00
#define PAYLOAD_DOOR_OPENING        0x01
#define PAYLOAD_DOOR_OPEN           0x02
#define PAYLOAD_DOOR_CLOSING        0x03
#define PAYLOAD_DOOR_BLOCKED        0x04

// Common Payload Values for Light State (for FRAME_ID_STM_LIGHT_STATE)
#define PAYLOAD_LIGHT_OFF           0x00
#define PAYLOAD_LIGHT_ON            0x01

// Common Payload Values for System Mode
#define PAYLOAD_MODE_NORMAL         0x00
#define PAYLOAD_MODE_FORCE_OPEN     0x01
#define PAYLOAD_MODE_FORCE_CLOSE    0x02

// IDs for Type: FRAME_TYPE_LABVIEW_TO_STM (LabVIEW -> STM32)
#define FRAME_ID_LABVIEW_SET_MODE     0x10 // Payload: 1 byte (new_mode)
#define FRAME_ID_LABVIEW_RESET_COUNT  0x13 // Payload: 0 bytes

typedef struct{
    FrameType_t type; 
    uint8_t id;       
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    uint8_t length;
} ParsedFrame_t;

// Callback function pointer when a command frame is received from LabVIEW.
// The callback should return true if command processed successfully, false otherwise.
// This return value is for application's internal use; no NACK will be sent.
typedef bool (*uart_command_handler_callback_t)(const ParsedFrame_t* command_frame);

/**
 * @brief Initializes the UART Protocol Service.
 * @param command_callback: Function pointer to be called when a valid command frame from LabVIEW is received.
 */
void UARTProto_Init(uart_command_handler_callback_t command_callback);

/**
 * @brief Sends a data frame.
 *        The frame is passed to the UART driver to be sent when UART is available.
 * @param type: Type of the frame (e.g., FRAME_TYPE_STM_TO_LABVIEW).
 * @param id: Specific ID of the message (e.g., FRAME_ID_STM_DOOR_STATE).
 * @param payload: Pointer to the payload data.
 * @param length: Length of the payload (must be <= MAX_PAYLOAD_LENGTH).
 * @return true if the frame was accepted by the UART driver for transmission,
 *         false otherwise (e.g., driver busy, invalid parameters, or payload too long).
 */
bool UARTProto_SendFrame(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length);

/**
 * @brief Processes incoming UART data from the driver's buffer and handles protocol logic.
 *        Should be called periodically in the main loop.
 */
void UARTProto_Process(void);

/**
 * @brief Kiểm tra các lỗi UART và xử lý nếu cần thiết
 * @return true nếu phát hiện có lỗi, false nếu không có lỗi nào
 * @note Hàm này sẽ tự động xóa các cờ lỗi sau khi xử lý
 */
bool UARTProto_CheckErrors(void);

#endif /* INC_SERVICES_UART_PROTOCOL_SERVICE_H_ */
