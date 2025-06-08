/*
 * limit_switch_service.c
 *
 *  Created on: May 29, 2025
 *      Author: Admin
 */

#include "limit_switch_service.h"
#include "exti_driver.h"
#include "systick_driver.h"

typedef enum {
    DEBOUNCE_STATE_IDLE,      // Chờ ngắt đầu tiên
    DEBOUNCE_STATE_WAITING,   // Đã có ngắt, đang chờ hết thời gian debounce
    DEBOUNCE_STATE_VALIDATED  // Trạng thái đã ổn định (không dùng trực tiếp, chỉ là khái niệm)
} DebounceInternalState_t;

typedef struct {
    GPIO_TypeDef* port;
    uint8_t       pin_number;
    uint8_t       exti_line;
    bool          current_stable_state; // Trạng thái ổn định cuối cùng (true = active/pressed)
    DebounceInternalState_t debounce_state;
    uint32_t      debounce_start_tick;
    uint8_t       last_raw_pin_state;   // Trạng thái đọc được từ pin gần nhất trong ISR
} LimitSwitch_Internal_t;

static LimitSwitch_Internal_t g_limit_switches[LIMIT_SWITCH_COUNT];

// Giả định: Công tắc là ACTIVE LOW (khi nhấn thì nối với GND, dùng PULL_UP nội)
// Do đó, khi đọc pin, LOW (0) nghĩa là công tắc được nhấn (active).
#define LIMIT_SWITCH_ACTIVE_LEVEL GPIO_PIN_RESET // 0

// Callback cho công tắc "Cửa Mở Hoàn Toàn"
static void ls_open_exti_handler(uint8_t exti_line) {
    (void)exti_line; // Tránh warning
    LimitSwitch_Internal_t* ls = &g_limit_switches[LIMIT_SWITCH_ID_DOOR_OPEN];

    // Đọc trạng thái thô của pin ngay lập tức
    ls->last_raw_pin_state = GPIO_ReadPin(ls->port, (1U << ls->pin_number));

    // Nếu đang IDLE (chưa debounce), bắt đầu quá trình debounce
    if (ls->debounce_state == DEBOUNCE_STATE_IDLE) {
        ls->debounce_state = DEBOUNCE_STATE_WAITING;
        ls->debounce_start_tick = GetTick();
    }
}

// Callback cho công tắc "Cửa Đóng Hoàn Toàn"
static void ls_closed_exti_handler(uint8_t exti_line) {
    (void)exti_line;
    LimitSwitch_Internal_t* ls = &g_limit_switches[LIMIT_SWITCH_ID_DOOR_CLOSED];
    ls->last_raw_pin_state = GPIO_ReadPin(ls->port, (1U << ls->pin_number));
    if (ls->debounce_state == DEBOUNCE_STATE_IDLE) {
        ls->debounce_state = DEBOUNCE_STATE_WAITING;
        ls->debounce_start_tick = GetTick();
    }
}

void LimitSwitchService_Init(GPIO_TypeDef* open_ls_port, uint8_t open_ls_pin_number,
                             GPIO_TypeDef* closed_ls_port, uint8_t closed_ls_pin_number,
                             uint8_t nvic_priority) {
    // --- Cấu hình cho Công tắc Cửa Mở ---
    LimitSwitch_Internal_t* ls_open = &g_limit_switches[LIMIT_SWITCH_ID_DOOR_OPEN];
    ls_open->port = open_ls_port;
    ls_open->pin_number = open_ls_pin_number;
    ls_open->exti_line = open_ls_pin_number; // EXTI line thường trùng pin_number
    ls_open->current_stable_state = (GPIO_ReadPin(ls_open->port, (1U << ls_open->pin_number)) == LIMIT_SWITCH_ACTIVE_LEVEL); // Đọc trạng thái ban đầu
    ls_open->debounce_state = DEBOUNCE_STATE_IDLE;
    ls_open->debounce_start_tick = 0;
    ls_open->last_raw_pin_state = (ls_open->current_stable_state ? LIMIT_SWITCH_ACTIVE_LEVEL : !LIMIT_SWITCH_ACTIVE_LEVEL);

    // EXTI_InitPin sẽ cấu hình GPIO là input. Chúng ta muốn PULL_UP vì công tắc active LOW.
    // Trigger trên cả hai sườn để bắt đầu debounce khi có bất kỳ thay đổi nào.
    EXTI_InitPin(ls_open->port, ls_open->pin_number, EXTI_TRIGGER_BOTH,
                 nvic_priority, ls_open_exti_handler);


    // --- Cấu hình cho Công tắc Cửa Đóng ---
    LimitSwitch_Internal_t* ls_closed = &g_limit_switches[LIMIT_SWITCH_ID_DOOR_CLOSED];
    ls_closed->port = closed_ls_port;
    ls_closed->pin_number = closed_ls_pin_number;
    ls_closed->exti_line = closed_ls_pin_number;
    ls_closed->current_stable_state = (GPIO_ReadPin(ls_closed->port, (1U << ls_closed->pin_number)) == LIMIT_SWITCH_ACTIVE_LEVEL);
    ls_closed->debounce_state = DEBOUNCE_STATE_IDLE;
    ls_closed->debounce_start_tick = 0;
    ls_closed->last_raw_pin_state = (ls_closed->current_stable_state ? LIMIT_SWITCH_ACTIVE_LEVEL : !LIMIT_SWITCH_ACTIVE_LEVEL);

    EXTI_InitPin(ls_closed->port, ls_closed->pin_number, EXTI_TRIGGER_BOTH,
                 nvic_priority, ls_closed_exti_handler);
}

void LimitSwitchService_ProcessDebounce(void) {
	neu ngat sw -> switchPressCnt=1
	getPin == 0, state = 1
	getPin == 1 && state = 1
	 cnt+1
	if(switchPress==1)
	{

	}
    for (int i = 0; i < LIMIT_SWITCH_COUNT; ++i) {
        LimitSwitch_Internal_t* ls = &g_limit_switches[i];

        if (ls->debounce_state == DEBOUNCE_STATE_WAITING) {
            if ((GetTick() - ls->debounce_start_tick) >= DEBOUNCE_TIME_MS) {
                // Thời gian debounce đã hết, đọc lại trạng thái pin
                uint8_t current_pin_state = GPIO_ReadPin(ls->port, (1U << ls->pin_number));

                // Chỉ cập nhật trạng thái ổn định nếu trạng thái đọc được VẪN GIỐNG
                // với trạng thái thô cuối cùng được ghi nhận bởi ISR.
                // Điều này giúp xử lý trường hợp pin thay đổi nhiều lần trong thời gian debounce.
                // Chúng ta quan tâm đến trạng thái cuối cùng sau khi hết debounce.
                // Hoặc đơn giản hơn, chỉ cần lấy trạng thái hiện tại sau khi debounce.
                // Cách đơn giản:
                bool new_stable_state = (current_pin_state == LIMIT_SWITCH_ACTIVE_LEVEL);

                if (ls->current_stable_state != new_stable_state) {
                    ls->current_stable_state = new_stable_state;
                    // TODO: Có thể gọi một callback ở đây để báo cho Application biết
                    // trạng thái công tắc đã thay đổi ổn định.
                    // Ví dụ: if (ls_state_change_callback[i]) ls_state_change_callback[i](ls->current_stable_state);
                }
                ls->debounce_state = DEBOUNCE_STATE_IDLE; // Quay lại chờ ngắt tiếp theo
            }
        }
    }
}

bool LimitSwitchService_IsDoorFullyOpen(void) {
    // Quan trọng: Cần bảo vệ việc đọc biến này nếu ProcessDebounce có thể chạy từ ngắt khác
    // hoặc nếu có đa luồng. Trong trường hợp ProcessDebounce chạy ở main loop,
    // và ISR chỉ set cờ, thì việc đọc trực tiếp có thể ổn.
    // Để an toàn, có thể dùng critical section ngắn.
    bool state;
    __disable_irq(); // critical section
    state = g_limit_switches[LIMIT_SWITCH_ID_DOOR_OPEN].current_stable_state;
    __enable_irq();
    return state;
}

bool LimitSwitchService_IsDoorFullyClosed(void) {
    bool state;
    __disable_irq();
    state = g_limit_switches[LIMIT_SWITCH_ID_DOOR_CLOSED].current_stable_state;
    __enable_irq();
    return state;
}
