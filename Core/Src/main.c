// main.c
#include "main.h" // Header chính của bạn, nên include các file .h của driver và service
#include "rcc_config.h"
#include "gpio_driver.h"
#include "systick_driver.h"
#include "uart_driver.h"
#include "uart_protocol_service.h" // Service cần test
#include <stdio.h>  // Cho sprintf (nếu dùng)
#include <string.h> // Cho strlen

// --- Cấu hình LED (ví dụ) ---
#define LED_PORT   GPIOA
#define LED_PIN    (1U << 5) // PA5 (LED xanh trên Nucleo F401RE)

// --- Biến toàn cục để test ---
volatile bool g_command_received_flag = false;
ParsedFrame_t g_last_received_command;

// --- Hàm Callback cho UART Protocol Service ---
// Hàm này sẽ được gọi bởi UARTProto_Process khi nhận được frame từ LabVIEW
bool handle_labview_frame(const ParsedFrame_t* command_frame) {
    g_command_received_flag = true;
    memcpy((void*)&g_last_received_command, command_frame, sizeof(ParsedFrame_t));

    // Trong một ứng dụng thực tế, bạn sẽ xử lý lệnh ở đây
    // Ví dụ:
    // if (command_frame->type == FRAME_TYPE_LABVIEW_TO_STM) {
    //     if (command_frame->id == FRAME_ID_LABVIEW_SET_MODE) {
    //         // uint8_t mode = command_frame->payload[0];
    //         // ProcessSetMode(mode);
    //     } else if (command_frame->id == FRAME_ID_LABVIEW_RESET_COUNT) {
    //         // ProcessResetCount();
    //     }
    // }
    return true; // Giả sử lệnh luôn được xử lý "thành công" về mặt logic của callback này
                 // vì service không gửi NACK dựa trên giá trị trả về này nữa.
}


int main(void) {
    // 1. Khởi tạo hệ thống cơ bản
    SystemClock_Config(); // Từ rcc_config.c
    SysTick_Init();       // Từ systick_driver.c

    // Khởi tạo LED để test
    GPIO_InitPin(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); // Tắt LED

    // 2. Khởi tạo UART Driver (USART2)
    // Giả định PCLK1_FREQUENCY_HZ đã được định nghĩa trong rcc_config.h
    UART2_Init(115200, UART_WORDLENGTH_8B, UART_PARITY_NONE, UART_STOPBITS_1);

    // 3. Khởi tạo UART Protocol Service
    UARTProto_Init(handle_labview_frame);

    uint32_t last_blink_time = GetTick();
    uint32_t last_status_send_time = GetTick();
    uint8_t person_count_test = 0;

    // Gửi thông báo khởi động
    char startup_msg[] = "STM32 UART Proto Test Ready!\r\n";
    // Gửi trực tiếp qua driver UART để không bị ảnh hưởng bởi logic frame của service
    UART2_SendBuffer_IT((uint8_t*)startup_msg, strlen(startup_msg));


    while (1) {
        // 4. Gọi UARTProto_Process() định kỳ để xử lý dữ liệu nhận
        UARTProto_Process();

        // 5. Kiểm tra lỗi UART (nếu có)
        if (UARTProto_CheckErrors()) {
            // Có lỗi UART (ví dụ: Overrun). Bạn có thể log hoặc xử lý.
            // Ví dụ: Gửi một thông điệp debug đặc biệt qua UART driver
            char error_msg[] = "UART HW Error Detected!\r\n";
            UART2_SendBuffer_IT((uint8_t*)error_msg, strlen(error_msg));
        }

        // 6. Test nhận lệnh từ LabVIEW
        if (g_command_received_flag) {
            g_command_received_flag = false;
            GPIO_TogglePin(LED_PORT, LED_PIN); // Nháy LED khi nhận được lệnh

            // In thông tin frame nhận được ra UART (dùng UART driver trực tiếp để debug)
            char debug_rx_msg[100];
            sprintf(debug_rx_msg, "RX Frame: T=0x%02X, ID=0x%02X, L=%d, P=",
                    g_last_received_command.type,
                    g_last_received_command.id,
                    g_last_received_command.length);
            UART2_SendBuffer_IT((uint8_t*)debug_rx_msg, strlen(debug_rx_msg));

            for (int i = 0; i < g_last_received_command.length; i++) {
                sprintf(debug_rx_msg, "0x%02X ", g_last_received_command.payload[i]);
                UART2_SendBuffer_IT((uint8_t*)debug_rx_msg, strlen(debug_rx_msg));
            }
            UART2_SendBuffer_IT((uint8_t*)"\r\n", 2);

            // Phản hồi một frame đơn giản (ví dụ, gửi lại person_count)
            // để kiểm tra LabVIEW có nhận được không
            // (Giả sử LabVIEW mong đợi một phản hồi sau khi gửi lệnh)
            // Đây không phải là ACK/NACK của protocol service mà là dữ liệu ứng dụng.
             uint8_t ack_payload[1];
             ack_payload[0] = g_last_received_command.id; // Gửi lại ID của lệnh vừa nhận
             UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_LOG_DEBUG, ack_payload, 1);

        }

        // 7. Test gửi trạng thái định kỳ lên LabVIEW
        if ((GetTick() - last_status_send_time) >= 2000) { // Gửi mỗi 2 giây
            last_status_send_time = GetTick();

            // Ví dụ gửi trạng thái số người
//            uint8_t payload_count[1];
//            payload_count[0] = person_count_test++;
//            if(person_count_test > 10) person_count_test = 0;
//
//            bool sent_ok = UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW,
//                                               FRAME_ID_STM_PERSON_COUNT,
//                                               payload_count,
//                                               1);
//            if (!sent_ok) {
//                // UARTProto_SendFrame có thể trả về false nếu UART driver bận
//                // Hoặc nếu các tham số không hợp lệ (đã được kiểm tra trong service)
//                char send_fail_msg[] = "ProtoSend Failed (Busy?)\r\n";
//                UART2_SendBuffer_IT((uint8_t*)send_fail_msg, strlen(send_fail_msg));
//            }

            // Ví dụ gửi trạng thái cửa (giả lập)
            uint8_t payload_door[1];
            payload_door[0] = PAYLOAD_DOOR_OPENING; // Trạng thái cửa đang mở
            bool sent_door = UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW,
                                               FRAME_ID_STM_DOOR_STATE,
                                               payload_door,
                                               1);
        }

        // Nháy LED để biết chương trình đang chạy (không liên quan UART)
        if ((GetTick() - last_blink_time) >= 500) {
            last_blink_time = GetTick();
            // GPIO_TogglePin(LED_PORT, LED_PIN); // Tạm thời không nháy ở đây để LED dùng cho RX
        }

        // Thêm một delay nhỏ để giảm tải CPU nếu không có gì khác để làm
        // Tuy nhiên, với các hàm non-blocking và ngắt, main loop có thể rảnh rỗi
        // Delay_ms(1); // Nếu cần
    }
}
