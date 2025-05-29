/*
 * exti_driver.c
 *
 *  Created on: May 28, 2025
 *      Author: Admin
 */

#include "exti_driver.h"

// Mảng lưu trữ các callback cho từng EXTI line (0-15)
static exti_callback_t g_exti_callbacks[16] = {NULL};

// Helper function để lấy port source cho SYSCFG_EXTICR
static uint8_t get_gpio_port_source(GPIO_TypeDef* GPIOx) {
    if (GPIOx == GPIOA) return 0x00U;
    if (GPIOx == GPIOB) return 0x01U;
    if (GPIOx == GPIOC) return 0x02U;
    if (GPIOx == GPIOD) return 0x03U;
    if (GPIOx == GPIOE) return 0x04U;
    if (GPIOx == GPIOH) return 0x07U; // GPIOH là 0b111
    return 0xFFU; // Lỗi
}

void EXTI_InitPin(GPIO_TypeDef* GPIOx, uint8_t pin_number, uint8_t trigger_type,
                  uint8_t priority, exti_callback_t callback) {
    if (pin_number > 15) return; // Chỉ hỗ trợ pin 0-15

    // 1. Cấu hình chân GPIO làm input (thường có pull-up hoặc pull-down)
    // Giả sử người dùng đã bật clock cho GPIOx hoặc GPIO_InitPin sẽ làm điều đó.
    // Ví dụ, cấu hình input với pull-up:
    GPIO_InitPin(GPIOx, (1U << pin_number), GPIO_MODE_INPUT, GPIO_PULL_UP,
                 GPIO_SPEED_LOW, 0, 0); // otype và alt_func không dùng cho input thường

    // 2. Bật clock cho SYSCFG
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // 3. Kết nối EXTI Line với chân GPIO thông qua SYSCFG
    // SYSCFG_EXTICRx có 4 trường, mỗi trường 4 bit, cho 4 EXTI line.
    // EXTICR[0] cho lines 0-3, EXTICR[1] cho lines 4-7, ...
    uint8_t exti_cr_index = pin_number / 4; // 0, 1, 2, 3
    uint8_t exti_cr_pos = (pin_number % 4) * 4; // 0, 4, 8, 12
    uint8_t port_source = get_gpio_port_source(GPIOx);

    if (port_source == 0xFFU) return; // Port không hợp lệ

    // Xóa các bit cũ và set port source mới
    SYSCFG->EXTICR[exti_cr_index] &= ~(0x0FU << exti_cr_pos);
    SYSCFG->EXTICR[exti_cr_index] |= (port_source << exti_cr_pos);

    // 4. Cấu hình EXTI Line
    // Bỏ mask ngắt (cho phép ngắt)
    EXTI->IMR |= (1U << pin_number);
    // Không dùng event mask ở đây
    // EXTI->EMR &= ~(1U << pin_number);

    // Cấu hình trigger type
    if (trigger_type == EXTI_TRIGGER_RISING || trigger_type == EXTI_TRIGGER_BOTH) {
        EXTI->RTSR |= (1U << pin_number);
    } else {
        EXTI->RTSR &= ~(1U << pin_number); // Clear nếu không phải rising
    }

    if (trigger_type == EXTI_TRIGGER_FALLING || trigger_type == EXTI_TRIGGER_BOTH) {
        EXTI->FTSR |= (1U << pin_number);
    } else {
        EXTI->FTSR &= ~(1U << pin_number); // Clear nếu không phải falling
    }

    // 5. Đăng ký callback
    if (pin_number < 16) {
        g_exti_callbacks[pin_number] = callback;
    }

    // 6. Cấu hình và kích hoạt ngắt trong NVIC
    IRQn_Type irq_number;
    if (pin_number <= 4) { // EXTI0 đến EXTI4
        irq_number = (IRQn_Type)(EXTI0_IRQn + pin_number);
    } else if (pin_number <= 9) { // EXTI5 đến EXTI9
        irq_number = EXTI9_5_IRQn;
    } else { // EXTI10 đến EXTI15
        irq_number = EXTI15_10_IRQn;
    }

    NVIC_SetPriority(irq_number, priority);
    NVIC_EnableIRQ(irq_number);
}

void EXTI_ClearPendingBit(uint8_t exti_line) {
    if (exti_line > 15) return;
    // Ghi 1 vào bit tương ứng trong Pending Register để xóa nó
    EXTI->PR = (1U << exti_line);
}

// --- Trình xử lý ngắt (ISRs) ---
// Các hàm này cần được định nghĩa với tên chính xác như trong vector ngắt.
// Chúng sẽ kiểm tra cờ pending và gọi callback tương ứng.

// Helper function để xử lý logic chung trong ISR
static void EXTI_IRQHandler_Helper(uint8_t exti_line_start, uint8_t exti_line_end) {
    for (uint8_t line = exti_line_start; line <= exti_line_end; ++line) {
        // Kiểm tra cờ Pending Bit cho từng line
        if ((EXTI->PR & (1U << line)) != 0) {
            // Xóa Pending Bit trước khi gọi callback (quan trọng!)
            EXTI_ClearPendingBit(line);

            // Gọi callback nếu có
            if (g_exti_callbacks[line] != NULL) {
                g_exti_callbacks[line](line);
            }
        }
    }
}

// ISR cho EXTI Line 0
void EXTI0_IRQHandler(void) {
    EXTI_IRQHandler_Helper(0, 0);
}

// ISR cho EXTI Line 1
void EXTI1_IRQHandler(void) {
    EXTI_IRQHandler_Helper(1, 1);
}

// ISR cho EXTI Line 2
void EXTI2_IRQHandler(void) {
    EXTI_IRQHandler_Helper(2, 2);
}

// ISR cho EXTI Line 3
void EXTI3_IRQHandler(void) {
    EXTI_IRQHandler_Helper(3, 3);
}

// ISR cho EXTI Line 4
void EXTI4_IRQHandler(void) {
    EXTI_IRQHandler_Helper(4, 4);
}

// ISR cho EXTI Lines 5 đến 9
void EXTI9_5_IRQHandler(void) {
    EXTI_IRQHandler_Helper(5, 9);
}

// ISR cho EXTI Lines 10 đến 15
void EXTI15_10_IRQHandler(void) {
    EXTI_IRQHandler_Helper(10, 15);
}
