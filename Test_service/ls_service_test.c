// #include <stdio.h>

// #include "rcc_config.h"
// #include "systick_driver.h"
// #include "gpio_driver.h"
// #include "limit_switch_service.h" // Service cần kiểm tra

// // Chân cho limit switch (công tắc hành trình)
// #define LS_OPEN_PORT        GPIOB
// #define LS_OPEN_PIN_NUM     8

// #define LS_CLOSED_PORT      GPIOB
// #define LS_CLOSED_PIN_NUM   9

// // Chân cho LED debug
// #define LED_OPEN_PORT       GPIOC
// #define LED_OPEN_PIN_NUM    8
// #define LED_OPEN_PIN_MASK   (1U << LED_OPEN_PIN_NUM)

// #define LED_CLOSE_PORT       GPIOC
// #define LED_CLOSE_PIN_NUM    6
// #define LED_CLOSE_PIN_MASK   (1U << LED_CLOSE_PIN_NUM)

// // Ưu tiên ngắt cho EXTI (công tắc hành trình)
// #define LIMIT_SWITCH_NVIC_PRIORITY  10 

// int main(void) {
//     SystemClock_Config();
//     SysTick_Init();

//     GPIO_InitPin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
//     GPIO_WritePin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_PIN_RESET);
//     GPIO_InitPin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
//     GPIO_WritePin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_PIN_RESET); 

//     LimitSwitchService_Init(LS_OPEN_PORT, LS_OPEN_PIN_NUM,
//                             LS_CLOSED_PORT, LS_CLOSED_PIN_NUM,
//                             LIMIT_SWITCH_NVIC_PRIORITY);

//     bool prev_door_open_state = LimitSwitchService_IsDoorFullyOpen();
//     bool prev_door_closed_state = LimitSwitchService_IsDoorFullyClosed();
//     bool current_door_open_state, current_door_closed_state;

//     while (1) {
//         LimitSwitchService_ProcessDebounce();

//         // Lấy trạng thái ổn định hiện tại của các công tắc
//         current_door_open_state = LimitSwitchService_IsDoorFullyOpen();
//         current_door_closed_state = LimitSwitchService_IsDoorFullyClosed();

//         // Kiểm tra trạng thái công tắc cửa mở thay đổi
//         if (current_door_open_state != prev_door_open_state) {
//             prev_door_open_state = current_door_open_state;
//         }

//         // Kiểm tra trạng thái công tắc cửa đóng thay đổi
//         if (current_door_closed_state != prev_door_closed_state) {
//             prev_door_closed_state = current_door_closed_state;
//         }

//         if (current_door_open_state) {
//             GPIO_WritePin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_PIN_SET); // Bật LED
//         } else {
//             GPIO_WritePin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_PIN_RESET); // Tắt LED
//         }
        
//         if (current_door_closed_state) {
//             GPIO_WritePin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_PIN_SET); // Bật LED
//         } else {
//             GPIO_WritePin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_PIN_RESET); // Tắt LED
//         }

//         Delay_ms(50); 
//     }

// }