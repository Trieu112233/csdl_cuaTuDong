/*
 * uart_protocol_service.c
 *
 *  Created on: May 28, 2025
 *      Author: Admin
 */

#include "uart_protocol_service.h"
#include "uart_driver.h"
#include "systick_driver.h"
#include <string.h>

typedef enum {
    STATE_WAIT_START,
    STATE_WAIT_TYPE,
    STATE_WAIT_ID,
    STATE_WAIT_LENGTH,
    STATE_WAIT_PAYLOAD,
    STATE_WAIT_END
} RxState_t;

static RxState_t 		g_rx_state = STATE_WAIT_START;
static uint8_t 			g_rx_buffer[MAX_FRAME_LENGTH]; // Buffer tạm để xây dựng frame nhận được
static uint8_t 			g_rx_buffer_idx = 0;
static ParsedFrame_t 	g_current_rx_frame; // Frame đang được xây dựng
static uint8_t 			g_expected_payload_len = 0;

// --- Trạng thái cho việc gửi frame và chờ ACK ---
static bool 		g_waiting_for_ack = false;
static uint8_t 		g_last_sent_frame_id_ack;//id cuar frame đang chờ ack
static FrameType_t 	g_last_sent_frame_type_ack;
static uint8_t 		g_last_sent_length_ack;
static uint8_t		g_last_sent_payload_ack[MAX_PAYLOAD_LENGTH];
static uint8_t 		g_send_retry_count = 0;
static uint32_t 	g_ack_timeout_start_tick = 0;

// --- Callback cho lớp Application ---
static uart_command_handler_callback_t g_app_command_callback = NULL;

// --- ID cho frame gửi đi tiếp theo (STM32 -> LabVIEW) ---
static uint8_t g_next_stm_frame_id = 0x80; // Bắt đầu từ 0x80 để phân biệt với ID từ LabVIEW (nếu cần)

// --- Forward declarations of static helper functions ---
static void reset_rx_parser(void);
static void process_received_frame_logic(void);
static void send_ack(uint8_t for_frame_id);
static void send_nack(uint8_t for_frame_id);
static bool actually_send_frame(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length);
static void UARTProto_TxDoneCallback(void);

void UARTProto_Init(uart_command_handler_callback_t command_callback) { // Kiểu callback đã đổi
    g_app_command_callback = command_callback;
    reset_rx_parser();
    g_waiting_for_ack = false;
    UART2_RegisterTxCompleteCallback(UARTProto_TxDoneCallback);
}

static void UARTProto_TxDoneCallback(void) {
    // Hàm này được gọi bởi UART Driver khi buffer TX đã được gửi hoàn toàn.
    // Nếu chúng ta đang chờ ACK cho frame vừa gửi, đây là lúc bắt đầu timer timeout.
    if (g_waiting_for_ack && g_send_retry_count == 0) { // Chỉ bắt đầu timeout cho lần gửi đầu tiên
        g_ack_timeout_start_tick = GetTick();
    } else if (g_waiting_for_ack && g_send_retry_count > 0) { // Đang gửi lại
        g_ack_timeout_start_tick = GetTick(); // Reset timeout cho lần gửi lại
    }
}

static bool actually_send_frame(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length) {
    if (length > MAX_PAYLOAD_LENGTH) return false; // Payload quá dài
    if (UART2_IsTxBusy() && type != FRAME_TYPE_ACK && type != FRAME_TYPE_NACK) {
        // Nếu đang bận gửi frame DATA, không cho gửi frame DATA mới,
        // nhưng vẫn cho phép gửi ACK/NACK (ACK/NACK không cần cơ chế ACK ngược lại)
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

bool UARTProto_SendFrameWithAck(FrameType_t type, uint8_t id, const uint8_t* payload, uint8_t length) {
    if (g_waiting_for_ack) { // Nếu đang chờ ACK cho frame trước, không gửi frame mới (cần ACK)
        return false;
    }

    if (length > MAX_PAYLOAD_LENGTH) { // Thêm kiểm tra ở đây nữa cho chắc
    	return false;
    }

    g_last_sent_frame_id_ack = id;
    g_last_sent_frame_type_ack = type;
    g_last_sent_length_ack = length;
    if(length > 0 && payload != NULL){
    	memcpy(g_last_sent_payload_ack, payload, length);
    } else if (length == 0){
        memset(g_last_sent_payload_ack, 0, MAX_PAYLOAD_LENGTH);
    }
    g_send_retry_count = 0;
    g_waiting_for_ack = true;
    if (actually_send_frame(type, id, payload, length)) {
    	return true;
    } else {
    	g_waiting_for_ack = false;
    	return false;
    }
}

static void reset_rx_parser(void) {
    g_rx_state = STATE_WAIT_START;
    g_rx_buffer_idx = 0;
    g_expected_payload_len = 0;
}

static void process_received_frame_logic(void) {
    bool command_processed_ok = false;

    switch (g_current_rx_frame.type) {
        case FRAME_TYPE_CMD_SET_MODE:
        case FRAME_TYPE_CMD_RESET_COUNT:
            if (g_app_command_callback != NULL) {
                command_processed_ok = g_app_command_callback(&g_current_rx_frame);
            } else {
                command_processed_ok = false; // Không có handler
            }

            if (command_processed_ok) {
                send_ack(g_current_rx_frame.id);
            } else {
                send_nack(g_current_rx_frame.id); // <<<< KHÔNG CẦN MÃ LỖI
            }
            break;

        case FRAME_TYPE_ACK:
            if (g_waiting_for_ack && g_current_rx_frame.length == 1 &&
                g_current_rx_frame.payload[0] == g_last_sent_frame_id_ack) {
                g_waiting_for_ack = false;
                g_send_retry_count = 0;
            }
            break;

        case FRAME_TYPE_NACK:
            if (g_waiting_for_ack && g_current_rx_frame.length == 1 && // NACK giờ chỉ có ID
                g_current_rx_frame.payload[0] == g_last_sent_frame_id_ack) {
                g_send_retry_count++;
                if (g_send_retry_count <= MAX_SEND_RETRIES) {
                    if (!UART2_IsTxBusy()) {
                         actually_send_frame(g_last_sent_frame_type_ack, g_last_sent_frame_id_ack,
                                        g_last_sent_payload_ack, g_last_sent_length_ack);
                    } else {
                         g_send_retry_count--;
                    }
                } else {
                    g_waiting_for_ack = false;
                    // TODO: Báo lỗi Application
                }
            }
            break;

        default: // Lỗi: Type không xác định từ LabVIEW
            send_nack(g_current_rx_frame.id); // <<<< KHÔNG CẦN MÃ LỖI
            break;
    }
}

void UARTProto_Process(void) {
    uint8_t byte;
    while (UART2_ReadByte_FromBuffer(&byte)) { // Đọc tất cả byte có trong buffer RX của driver
        switch (g_rx_state) {
            case STATE_WAIT_START:
                if (byte == FRAME_START_BYTE) {
                    g_rx_buffer_idx = 0;
                    g_rx_buffer[g_rx_buffer_idx++] = byte;
                    g_rx_state = STATE_WAIT_TYPE;
                }
                break;

            case STATE_WAIT_TYPE:
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                g_current_rx_frame.type = (FrameType_t)byte;
                g_rx_state = STATE_WAIT_ID;
                break;

            case STATE_WAIT_ID:
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                g_current_rx_frame.id = byte;
                g_rx_state = STATE_WAIT_LENGTH;
                break;

            case STATE_WAIT_LENGTH:
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                if (byte > MAX_PAYLOAD_LENGTH) { // Kiểm tra độ dài payload
                	send_nack(g_current_rx_frame.id);
                    reset_rx_parser(); // Lỗi, payload quá dài
                } else {
                    g_current_rx_frame.length = byte;
                    g_expected_payload_len = byte;
                    if (g_expected_payload_len == 0) {
                        g_rx_state = STATE_WAIT_END;
                    } else {
                        g_rx_state = STATE_WAIT_PAYLOAD;
                    }
                }
                break;

            case STATE_WAIT_PAYLOAD:
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                g_current_rx_frame.payload[g_rx_buffer_idx - (FRAME_OVERHEAD -1 /*END*/)] = byte; // -1 vì END chưa có
                if ((g_rx_buffer_idx - (FRAME_OVERHEAD -1)) >= g_expected_payload_len) {
                    g_rx_state = STATE_WAIT_END;
                }
                break;

            case STATE_WAIT_END:
                g_rx_buffer[g_rx_buffer_idx++] = byte;
                if (byte == FRAME_END_BYTE) {
                	process_received_frame_logic();
                } else {
                	send_nack(g_current_rx_frame.id);
                }
                reset_rx_parser();
                break;

            default:
                reset_rx_parser();
                break;
        }
        if (g_rx_buffer_idx >= MAX_FRAME_LENGTH && g_rx_state != STATE_WAIT_START) {
            // Tránh tràn buffer nếu frame lỗi không có END byte
            reset_rx_parser();
        }
    }

    // Xử lý timeout ACK
    if (g_waiting_for_ack && (GetTick() - g_ack_timeout_start_tick >= ACK_TIMEOUT_MS)) {
        if (UART2_IsTxBusy()) {
            // Nếu UART vẫn đang bận gửi lần cuối (ví dụ, do ngắt bị ưu tiên thấp),
            // cho thêm chút thời gian.
            g_ack_timeout_start_tick = GetTick(); // Reset timeout
            return;
        }

        g_send_retry_count++;
        if (g_send_retry_count <= MAX_SEND_RETRIES) {
            // Gửi lại frame
            if(!UART2_IsTxBusy()){
                 actually_send_frame(g_last_sent_frame_type_ack, g_last_sent_frame_id_ack,
                                g_last_sent_payload_ack, g_last_sent_length_ack);
                 // g_ack_timeout_start_tick sẽ được set trong callback TxDone
            } else {
                 // UART vẫn đang bận, có thể chờ hoặc xử lý khác
                 // Tạm thời coi như mất lượt retry này để tránh block
            }
        } else {
            g_waiting_for_ack = false; // Quá số lần thử
            // TODO: Báo lỗi "Không nhận được ACK sau nhiều lần thử" lên lớp Application
            // Ví dụ: if (g_app_ack_fail_callback) g_app_ack_fail_callback(g_last_sent_frame_id_ack);
        }
    }
}

static void send_ack(uint8_t for_frame_id) {
	uint8_t ack_id = UARTProto_GetNextTxFrameID();
	uint8_t ack_payload[1] = {for_frame_id};
	actually_send_frame(FRAME_TYPE_ACK, ack_id, ack_payload, 1);
}

static void send_nack(uint8_t for_frame_id) {
	uint8_t nack_id = UARTProto_GetNextTxFrameID();
    uint8_t nack_payload[1] = {for_frame_id};
    actually_send_frame(FRAME_TYPE_NACK, nack_id, nack_payload, 1); // Payload length là 1
}

// Hàm lấy ID cho frame gửi đi từ STM32 (ví dụ STATUS_UPDATE)
uint8_t UARTProto_GetNextTxFrameID(void) {
    uint8_t id = g_next_stm_frame_id++;
    if (g_next_stm_frame_id < 0x80) {
    	g_next_stm_frame_id = 0x80;
    }
    return id;
}



