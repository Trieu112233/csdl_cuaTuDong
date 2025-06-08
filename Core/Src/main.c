//#include <stdio.h>
//#include <string.h>
//
//#include "rcc_config.h"
//#include "systick_driver.h"
//// Application Logic
//#include "system_manager.h"       // Module điều phối chính
//#include "people_counter.h"
//#include "door_fsm.h"
//#include "lighting_logic.h"
//
//int main(void) {
//    // Khởi tạo hệ thống
//    SystemClock_Config(); // Cấu hình clock hệ thống (từ rcc_config.c)
//    SysTick_Init();       // Khởi tạo SysTick cho tick 1ms và Delay_ms()
//    SystemManager_Init();
//
//    // Vòng lặp chính
//    while (1) {
//        // Xử lý các module
//        SystemManager_Process();
//    }
//
//    return 0;
//}

#include <stdint.h>
#include <stdbool.h>
#include <string.h> // Cho strlen, memcpy (nếu dùng trong main.c)
// #include <stdio.h> // Cho sprintf (nếu dùng trong main.c)

#include "stm32f401xe.h"
#include "rcc_config.h"
#include "gpio_driver.h"
#include "systick_driver.h"
#include "uart_driver.h"
#include "uart_protocol_service.h"

#define LED_USER_PORT   GPIOA
#define LED_USER_PIN    (1U << 5)

// --- Biến toàn cục mô phỏng trạng thái hệ thống ---
static uint8_t g_app_door_state = PAYLOAD_DOOR_CLOSED;
static uint8_t g_app_light_state = PAYLOAD_LIGHT_OFF;
static uint8_t g_app_person_count = 0;
static uint8_t g_app_system_mode = PAYLOAD_MODE_NORMAL;


// --- Hàm callback được gọi bởi UARTProto_Service khi nhận được lệnh từ LabVIEW ---
bool main_handle_labview_command(const ParsedFrame_t* frame) {
    // Nháy LED để báo hiệu nhận được frame
	GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_SET); Delay_ms(50);
	    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_RESET); Delay_ms(50);
	    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_SET); Delay_ms(50);
	    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_RESET);

    if (frame->type != FRAME_TYPE_LABVIEW_TO_STM) {
        return false; // Không phải loại frame mong đợi
    }

    switch (frame->id) {
        case FRAME_ID_LABVIEW_SET_MODE:
            if (frame->length == 1) {
                uint8_t new_mode = frame->payload[0];
                if (new_mode == PAYLOAD_MODE_NORMAL ||
                    new_mode == PAYLOAD_MODE_FORCE_OPEN ||
                    new_mode == PAYLOAD_MODE_FORCE_CLOSE) {
                    g_app_system_mode = new_mode;
                    // Gửi lại trạng thái mode mới để LabVIEW biết đã cập nhật
                    UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_SYSTEM_MODE, &g_app_system_mode, 1);
                    return true;
                } else {
                    return false; // Giá trị mode không hợp lệ
                }
            } else {
                return false; // Độ dài payload sai
            }
            // break;

        case FRAME_ID_LABVIEW_RESET_COUNT:
            if (frame->length == 0) {
                g_app_person_count = 0;
                // Gửi lại số người mới (0) để LabVIEW biết đã cập nhật
                UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_PERSON_COUNT, &g_app_person_count, 1);
                return true;
            } else {
                return false; // Độ dài payload sai
            }
            // break;

        default:
            return false; // ID lệnh không xác định
    }
    // return false; // Mặc định
}


int main(void) {
    SystemClock_Config();
    SysTick_Init();

    GPIO_InitPin(LED_USER_PORT, LED_USER_PIN, GPIO_MODE_OUTPUT, GPIO_PULL_NO,
                 GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_RESET);

    for (int i = 0; i < 3; ++i) {
            GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_SET); Delay_ms(150);
            GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_RESET); Delay_ms(150);
        }

    UART2_Init(115200, UART_WORDLENGTH_8B, UART_PARITY_NONE, UART_STOPBITS_1);

    UARTProto_Init(main_handle_labview_command);

    char startup_msg[] = "STM32 No-ACK Test Ready!";
    if (!UART2_IsTxBusy()) { // Kiểm tra xem driver có bận không
         UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_LOG_DEBUG, (uint8_t*)startup_msg, strlen(startup_msg));
    }
    Delay_ms(100); // Chờ chút cho message đi

    uint32_t last_status_send_tick = GetTick();

    while (1) {
        UARTProto_Process();

        if (UARTProto_CheckErrors()) {
            // Có lỗi UART, ví dụ: bật LED báo lỗi
            GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_SET); // Bật LED đỏ nếu lỗi
        }


        // Gửi frame FULL_SNAPSHOT lên LabVIEW mỗi 2 giây
        if ((GetTick() - last_status_send_tick) >= 2000) {
            if (!UART2_IsTxBusy()) { // Chỉ gửi nếu driver không bận
                uint8_t snapshot_payload[4]; // door_state, light_state, person_count, system_mode
                snapshot_payload[0] = g_app_system_mode;
                snapshot_payload[1] = g_app_door_state;
                snapshot_payload[2] = g_app_person_count;
                snapshot_payload[3] = g_app_light_state;

                if (UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_FULL_SNAPSHOT, snapshot_payload, 4)) {
                    last_status_send_tick = GetTick();
                    // Nháy LED nhẹ báo đã cố gửi
                    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_RESET); // Tắt (nếu đang bật do lỗi)
                    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_SET);
                    Delay_ms(10);
                    GPIO_WritePin(LED_USER_PORT, LED_USER_PIN, GPIO_PIN_RESET);
                }
            }
        }

        // Giả lập thay đổi trạng thái để gửi đi
        static uint32_t last_state_change_tick = 0;
        if((GetTick() - last_state_change_tick) >= 3000) { // Mỗi 3 giây
            g_app_person_count = (g_app_person_count + 1) % 10;
            g_app_door_state = (g_app_door_state + 1) % 4; // Cycle qua 0,1,2,3
            g_app_light_state = !g_app_light_state;
            last_state_change_tick = GetTick();
        }
    }
}

