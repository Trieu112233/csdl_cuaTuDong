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

typedef enum{
	FRAME_TYPE_RESERVED         = 0x00,
	    // Lệnh từ LabVIEW -> STM32
	FRAME_TYPE_CMD_SET_MODE     = 0x10, // Payload: new_mode (AUTO, FORCE_OPEN, FORCE_CLOSE)
	FRAME_TYPE_CMD_RESET_COUNT  = 0x13, // Payload: (không có hoặc 0)
	// Trạng thái từ STM32 -> LabVIEW
	FRAME_TYPE_FULL_SNAPSHOT    = 0x05, // Payload: mode, door, person_count, light
	FRAME_TYPE_DOOR_STATE		= 0x01,
	FRAME_TYPE_LIGHT_STATE		= 0x02,
	FRAME_TYPE_PERSON_COUNT		= 0x03,
	FRAME_TYPE_SYSTEM_MODE		= 0x04,
	// ACK/NACK
	FRAME_TYPE_ACK              = 0x30, // Payload: ID của frame được ACK
	FRAME_TYPE_NACK             = 0x31  // Payload: ID của frame bị NACK
} FrameType_t;

typedef struct{
	FrameType_t type;
	uint8_t id;
	uint8_t payload[MAX_PAYLOAD_LENGTH];
	uint8_t length;
} ParsedFrame_t;

// --- Các hằng số cho cơ chế ACK/NACK ---
#define ACK_TIMEOUT_MS      500  // Thời gian chờ ACK (ms)
#define MAX_SEND_RETRIES    3    // Số lần thử gửi lại tối đa

// Callback function pointer khi nhận được frame DATA hợp lệ từ LabVIEW ---
// Callback chỉ trả về bool. Service sẽ gửi ACK/NACK dựa trên đó.
typedef bool (*uart_command_handler_callback_t)(const ParsedFrame_t* command_frame);

/**
 * @brief Initializes the UART Protocol Service.
 * @param command_callback: Function pointer to be called when a valid command frame is received.
 *                          The callback should return true if command processed successfully,
 *                          false otherwise (service will send NACK).
 */
void UARTProto_Init(uart_command_handler_callback_t command_callback);

/**
 * @brief Sends a data frame with ACK mechanism.
 * @param type: Type of the frame (e.g., FRAME_TYPE_STATUS_UPDATE).
 * @param id: ID for this frame (sẽ được dùng trong ACK/NACK).
 * @param payload: Pointer to the payload data.
 * @param length: Length of the payload.
 * @return true if the frame was accepted for transmission, false otherwise (e.g., busy).
 * @note This function will manage retries and ACK timeout.
 */
bool UARTProto_SendFrameWithAck(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length);

/**
 * @brief Processes incoming UART data from the driver's buffer and handles protocol logic.
 *        Should be called periodically in the main loop.
 */
void UARTProto_Process(void);

/**
 * @brief Gets the next available Frame ID for STM32 to send.
 */
uint8_t UARTProto_GetNextTxFrameID(void);

/**
 * @brief Kiểm tra các lỗi UART và xử lý nếu cần thiết
 * @return true nếu phát hiện có lỗi, false nếu không có lỗi nào
 * @note Hàm này sẽ tự động xóa các cờ lỗi sau khi xử lý
 */
bool UARTProto_CheckErrors(void);

#endif /* INC_SERVICES_UART_PROTOCOL_SERVICE_H_ */
