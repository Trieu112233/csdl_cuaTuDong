/*
 * uart_protocol_service.c
 *
 *  Created on: May 28, 2025
 *      Author: Admin
 */

#include "uart_protocol_service.h"
#include "uart_driver.h"
#include "systick_driver.h" // For GetTick()
#include <string.h> // For memcpy

typedef enum {
    STATE_WAIT_START,
    STATE_WAIT_TYPE,
    STATE_WAIT_ID,
    STATE_WAIT_LENGTH,
    STATE_WAIT_PAYLOAD,
    STATE_WAIT_END
} RxState_t;

static RxState_t 		g_rx_state = STATE_WAIT_START;
static uint8_t 			g_rx_buffer[MAX_FRAME_LENGTH]; // Buffer for assembling raw incoming frame bytes
static uint8_t 			g_rx_buffer_idx = 0;
static ParsedFrame_t 	g_current_rx_frame;    // Holds the currently parsed frame data
static uint8_t 			g_expected_payload_len = 0;

static uart_command_handler_callback_t g_app_command_callback = NULL;

// Forward declarations of static helper functions
static void reset_rx_parser(void);
static void process_received_frame_logic(void);
static bool actually_send_frame(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length);

void UARTProto_Init(uart_command_handler_callback_t command_callback) {
	g_app_command_callback = command_callback;
    reset_rx_parser();
    UART2_Init(115200, UART_WORDLENGTH_8B, UART_PARITY_NONE, UART_STOPBITS_1);
}

static bool actually_send_frame(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length) {
    if (length > MAX_PAYLOAD_LENGTH) {
        return false;
    }

    uint8_t frame_buffer[MAX_FRAME_LENGTH];
    uint8_t frame_idx = 0;

    frame_buffer[frame_idx++] = FRAME_START_BYTE;
    frame_buffer[frame_idx++] = (uint8_t)type;
    frame_buffer[frame_idx++] = id;
    frame_buffer[frame_idx++] = length;

    if (length > 0 && payload != NULL) {
        memcpy(&frame_buffer[frame_idx], payload, length);
        frame_idx += length;
    }
    frame_buffer[frame_idx++] = FRAME_END_BYTE;

    return UART2_SendBuffer_IT(frame_buffer, frame_idx);
}

bool UARTProto_SendFrame(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length) {
    if (length > MAX_PAYLOAD_LENGTH) {
        return false; // Payload too long
    }
    return actually_send_frame(type, id, payload, length);
}

static void reset_rx_parser(void) {
    g_rx_state = STATE_WAIT_START;
    g_rx_buffer_idx = 0;
    g_expected_payload_len = 0;
}

static void process_received_frame_logic(void) {
    // This function is called when a complete, structurally valid frame
    // of type FRAME_TYPE_LABVIEW_TO_STM has been received.
    // Frame data is in g_current_rx_frame.

    // Only expect to process commands from LabVIEW.
    if (g_current_rx_frame.type != FRAME_TYPE_LABVIEW_TO_STM) {
        return; 
    }

    switch (g_current_rx_frame.id) {
        case FRAME_ID_LABVIEW_SET_MODE:
            if (g_current_rx_frame.length == 1) {
                if (g_app_command_callback != NULL) {
                    // Let the application handler process the command.
                    // The callback's return value is for the app's use.
                    g_app_command_callback(&g_current_rx_frame);
                }
            } else {
            }
            break;

        case FRAME_ID_LABVIEW_RESET_COUNT:
            // Validate payload length
            if (g_current_rx_frame.length == 0) {
                if (g_app_command_callback != NULL) {
                    g_app_command_callback(&g_current_rx_frame);
                }
            } else {
                // Invalid payload length for RESET_COUNT. Silently ignore or log.
            }
            break;

        default:
            // Unknown command ID received from LabVIEW. Silently ignore or log.
            break;
    }
}

void UARTProto_Process(void) {
    uint8_t byte;
    while (UART2_ReadByte_FromBuffer(&byte)) {
        // Prevent buffer overflow if a malformed frame is too long
        if (g_rx_state != STATE_WAIT_START && g_rx_buffer_idx >= MAX_FRAME_LENGTH) {
            reset_rx_parser();
            // After reset, the current 'byte' might be a START_BYTE
        }

        switch (g_rx_state) {
            case STATE_WAIT_START:
                if (byte == FRAME_START_BYTE) {
                    // g_rx_buffer_idx is 0 due to reset_rx_parser()
                    g_rx_buffer[g_rx_buffer_idx++] = byte;
                    g_rx_state = STATE_WAIT_TYPE;
                }
                break;

            case STATE_WAIT_TYPE:
                g_current_rx_frame.type = (FrameType_t)byte;
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                if (g_current_rx_frame.type == FRAME_TYPE_LABVIEW_TO_STM) {
                    g_rx_state = STATE_WAIT_ID;
                } else {
                    // Received an unexpected frame type (e.g., STM_TO_LABVIEW type, or invalid)
                    reset_rx_parser();
                }
                break;

            case STATE_WAIT_ID:
                g_current_rx_frame.id = byte;
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                g_rx_state = STATE_WAIT_LENGTH;
                break;

            case STATE_WAIT_LENGTH:
                if (byte > MAX_PAYLOAD_LENGTH) {
                    reset_rx_parser(); // Error: payload length field is too large
                } else {
                    g_current_rx_frame.length = byte;
                    g_expected_payload_len = byte;
                    g_rx_buffer[g_rx_buffer_idx++] = byte;
                    if (g_expected_payload_len == 0) {
                        g_rx_state = STATE_WAIT_END; // Skip payload if length is 0
                    } else {
                        g_rx_state = STATE_WAIT_PAYLOAD;
                    }
                }
                break;

            case STATE_WAIT_PAYLOAD:
                // Current payload byte's 0-based index in the payload array.
                // Header (START,TYPE,ID,LENGTH) is 4 bytes.
                // g_rx_buffer_idx is count of bytes in g_rx_buffer *before* adding current 'byte'.
                uint8_t payload_byte_index = g_rx_buffer_idx - 4;

                if (payload_byte_index < MAX_PAYLOAD_LENGTH) {
                     g_current_rx_frame.payload[payload_byte_index] = byte;
                }
                g_rx_buffer[g_rx_buffer_idx++] = byte; // Store current payload byte in raw buffer

                if ((payload_byte_index + 1) >= g_expected_payload_len) {
                    g_rx_state = STATE_WAIT_END; // All expected payload bytes received
                }
                break;

            case STATE_WAIT_END:
                if (byte == FRAME_END_BYTE) {
                    // Frame is structurally complete.
                    // g_rx_buffer_idx is count of (START,TYPE,ID,LENGTH,PAYLOAD bytes).
                    // Expected count = 4 + g_current_rx_frame.length.
                    if (g_rx_buffer_idx == (4 + g_current_rx_frame.length)) {
                        // Only process if it's a command from LabVIEW (already checked at TYPE stage)
                        if (g_current_rx_frame.type == FRAME_TYPE_LABVIEW_TO_STM) {
                            process_received_frame_logic();
                        }
                    } else {
                        // Error: Frame length mismatch. Reset parser.
                        reset_rx_parser();
                    }
                } else {
                    // Error: Expected FRAME_END_BYTE but received something else.
                }
                reset_rx_parser(); // Always reset for the next frame, regardless of END byte correctness.
                break;

            default: 
                reset_rx_parser();
                break;
        }
    }
}

bool UARTProto_CheckErrors(void) {
    uint8_t errors = UART2_GetErrorFlags();
    bool has_errors = (errors != 0);

    if (has_errors) {
        if (errors & UART_ERROR_BUFFER_FULL) {
            reset_rx_parser();
        }
        UART2_ClearErrorFlags(errors); // Clear the reported errors in the driver.
    }
    return has_errors;
}



