/*
 * pwm_driver.c
 *
 *  Created on: May 28, 2025
 *      Author: Admin
 */

#include "pwm_driver.h"
#include <stdio.h>

// Helper function để lấy con trỏ đến Timer dựa trên enum
static TIM_TypeDef* get_timer_peripheral(PWM_TimerInstance_e timer_instance) {
    switch (timer_instance) {
        case PWM_TIMER2: return TIM2;
        case PWM_TIMER3: return TIM3;
        case PWM_TIMER4: return TIM4;
        case PWM_TIMER5: return TIM5;
        default: return NULL;
    }
}

// Helper function để bật clock cho Timer
static void pwm_timer_clock_cmd(PWM_TimerInstance_e timer_instance, uint8_t enable_disable) {
    TIM_TypeDef* TIMx = get_timer_peripheral(timer_instance);
    if (TIMx == NULL) return;

    if (enable_disable == CLOCK_CMD_ENABLE) { // Giả sử CLOCK_CMD_ENABLE = 1
        if (TIMx == TIM2 || TIMx == TIM3 || TIMx == TIM4 || TIMx == TIM5) {
            RCC->APB1ENR |= (1U << ( ( (uint32_t)TIMx - APB1PERIPH_BASE ) / 0x400UL) );
            // TIM2 is bit 0, TIM3 is bit 1, etc. on APB1ENR
            // This calculation is a bit generic, simpler direct mapping:
            // if (TIMx == TIM2) RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
            // else if (TIMx == TIM3) RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; ...
        }
        // Thêm cho các timer trên APB2 nếu cần (TIM1, TIM8, TIM9, TIM10, TIM11)
    } else {
        // Logic để tắt clock (ít dùng hơn)
    }
}

// Lưu trữ giá trị ARR của mỗi timer để tính toán duty cycle
// Cần khởi tạo các giá trị này trong PWM_InitChannel
static uint32_t g_timer_arr_values[4] = {0}; // TIM2, TIM3, TIM4, TIM5

int8_t PWM_InitChannel(PWM_TimerInstance_e timer_instance, PWM_TimerChannel_e channel,
                       uint32_t pwm_frequency_hz,
                       GPIO_TypeDef* gpio_port, uint8_t gpio_pin_number, uint8_t gpio_af_mode) {
    TIM_TypeDef* TIMx = get_timer_peripheral(timer_instance);
    if (TIMx == NULL || channel < PWM_CHANNEL_1 || channel > PWM_CHANNEL_4 || pwm_frequency_hz == 0) {
        return -1; // Tham số không hợp lệ
    }

    // 1. Bật clock cho Timer và GPIO Port
    pwm_timer_clock_cmd(timer_instance, CLOCK_CMD_ENABLE);
    GPIO_ClockCmd(gpio_port, CLOCK_CMD_ENABLE); // Giả sử hàm này tồn tại và hoạt động

    // 2. Cấu hình chân GPIO ở chế độ Alternate Function
    GPIO_InitPin(gpio_port, (1U << gpio_pin_number), GPIO_MODE_AF, GPIO_PULL_NO,
                 GPIO_SPEED_HIGH, GPIO_OTYPE_PUSHPULL, gpio_af_mode);

    // 3. Tính toán Prescaler (PSC) và Auto-Reload Register (ARR)
    // Tần số clock của Timer (ví dụ PCLK1 cho TIM2/3/4/5)
    // Nếu Prescaler của APB1 khác 1, PCLK1_TIMER_CLOCK = PCLK1_FREQUENCY_HZ * 2
    // (Tham khảo mục "Timer clock" trong Reference Manual)
    uint32_t timer_clock_hz;
    if (TIMx == TIM2 || TIMx == TIM3 || TIMx == TIM4 || TIMx == TIM5) { // Thuộc APB1
        if ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1) { // APB1 Prescaler = 1
            timer_clock_hz = PCLK1_FREQUENCY_HZ;
        } else { // APB1 Prescaler > 1
            timer_clock_hz = PCLK1_FREQUENCY_HZ * 2;
        }
    } else {
        // Thêm logic cho timer trên APB2 (ví dụ TIM1)
        // if ((RCC->CFGR & RCC_CFGR_PPRE2) == RCC_CFGR_PPRE2_DIV1) { ... }
        return -1; // Timer không được hỗ trợ
    }

    // Tìm PSC và ARR. Ưu tiên ARR lớn để có độ phân giải duty cycle tốt hơn.
    // pwm_frequency = timer_clock_hz / ((PSC + 1) * (ARR + 1))
    // Giả sử ARR khoảng 1000-65535. PSC = (timer_clock_hz / (pwm_frequency * (ARR+1))) - 1
    uint32_t arr_val = (timer_clock_hz / pwm_frequency_hz) / 1000U; // Thử ARR ~1000
    if (arr_val > 0xFFFFU) arr_val = 0xFFFFU; // Giới hạn ARR
    if (arr_val < 100U && pwm_frequency_hz < 1000) arr_val = 999; // Tăng ARR nếu tần số PWM thấp
    if (arr_val == 0) arr_val = 1; // Tránh chia cho 0

    uint32_t psc_val = (timer_clock_hz / (pwm_frequency_hz * (arr_val + 1))) -1;
    if (psc_val > 0xFFFFU) psc_val = 0xFFFFU;


    g_timer_arr_values[timer_instance] = arr_val; // Lưu lại ARR

    // 4. Cấu hình Timer cơ bản
    TIMx->PSC = (uint16_t)psc_val;
    TIMx->ARR = (uint16_t)arr_val;
    TIMx->CNT = 0; // Reset bộ đếm

    // 5. Cấu hình kênh Output Compare cho PWM
    volatile uint16_t* ccmr_reg;
    volatile uint16_t* ccer_reg;
    volatile uint32_t* ccr_reg; // CCRx là 32-bit nhưng chỉ dùng 16-bit thấp

    if (channel == PWM_CHANNEL_1 || channel == PWM_CHANNEL_2) {
        ccmr_reg = (volatile uint16_t*)&(TIMx->CCMR1);
    } else { // Kênh 3 hoặc 4
        ccmr_reg = (volatile uint16_t*)&(TIMx->CCMR2);
    }
    ccer_reg = (volatile uint16_t*)&(TIMx->CCER);

    switch (channel) {
        case PWM_CHANNEL_1: ccr_reg = &(TIMx->CCR1); break;
        case PWM_CHANNEL_2: ccr_reg = &(TIMx->CCR2); break;
        case PWM_CHANNEL_3: ccr_reg = &(TIMx->CCR3); break;
        case PWM_CHANNEL_4: ccr_reg = &(TIMx->CCR4); break;
        default: return -1; // Should not happen
    }

    // Cấu hình PWM Mode 1 (OCxM = 110) và bật Preload Enable (OCxPE = 1)
    // Preload cho phép cập nhật CCRx một cách an toàn vào cuối mỗi chu kỳ PWM
    uint16_t oc_mode_bits = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE;
    uint16_t ccer_enable_bit = TIM_CCER_CC1E << ((channel - 1) * 4);

    if (channel == PWM_CHANNEL_1 || channel == PWM_CHANNEL_3) { // Kênh 1 và 3 dùng 8 bit thấp của CCMRx
        *ccmr_reg &= ~(0xFFU << ((channel == PWM_CHANNEL_3) ? 8 : 0)); // Xóa bit cũ (OCxM, OCxPE, OCxFE, CCxS)
        *ccmr_reg |= (oc_mode_bits << ((channel == PWM_CHANNEL_3) ? 8 : 0));
    } else { // Kênh 2 và 4 dùng 8 bit cao của CCMRx
        *ccmr_reg &= ~(0xFF00U >> ((channel == PWM_CHANNEL_4) ? 0 : 8)); // Xóa bit cũ
        *ccmr_reg |= (oc_mode_bits << ((channel == PWM_CHANNEL_4) ? 8 : 0));
    }

    // Bật output cho kênh
    *ccer_reg |= ccer_enable_bit;

    // Đặt duty cycle ban đầu là 0%
    *ccr_reg = 0;

    // Bật bit ARPE (Auto-Reload Preload Enable) trong CR1 để ARR được buffer
    TIMx->CR1 |= TIM_CR1_ARPE;

    // (Tùy chọn) Tạo một update event để nạp các giá trị preload vào shadow registers
    TIMx->EGR |= TIM_EGR_UG;

    // Timer chưa được start ở đây, sẽ start bằng PWM_Start()

    return 0;
}

int8_t PWM_SetDutyCycle(PWM_TimerInstance_e timer_instance, PWM_TimerChannel_e channel, float duty_cycle_percent) {
    TIM_TypeDef* TIMx = get_timer_peripheral(timer_instance);
    if (TIMx == NULL || channel < PWM_CHANNEL_1 || channel > PWM_CHANNEL_4) {
        return -1;
    }

    if (duty_cycle_percent < 0.0f) duty_cycle_percent = 0.0f;
    if (duty_cycle_percent > 100.0f) duty_cycle_percent = 100.0f;

    uint32_t arr_val = g_timer_arr_values[timer_instance];
    if (arr_val == 0) return -1; // Timer chưa được init đúng cách

    uint32_t ccr_val = (uint32_t)((duty_cycle_percent / 100.0f) * (float)(arr_val +1));
    // arr_val+1 vì duty cycle 100% ứng với CCR = ARR+1 (hoặc ARR nếu dùng PWM mode khác)
    // Với PWM Mode 1 (đếm lên): 0% -> CCR=0; 100% -> CCR = ARR+1 (hoặc lớn hơn ARR)
    // Để đơn giản, nếu CCR > ARR, thì nó sẽ là 100% duty.

    if (ccr_val > arr_val) ccr_val = arr_val +1; // Đảm bảo 100% là CCR > ARR

    volatile uint32_t* ccr_reg;
    switch (channel) {
        case PWM_CHANNEL_1: ccr_reg = &(TIMx->CCR1); break;
        case PWM_CHANNEL_2: ccr_reg = &(TIMx->CCR2); break;
        case PWM_CHANNEL_3: ccr_reg = &(TIMx->CCR3); break;
        case PWM_CHANNEL_4: ccr_reg = &(TIMx->CCR4); break;
        default: return -1;
    }

    *ccr_reg = ccr_val;
    return 0;
}

int8_t PWM_Start(PWM_TimerInstance_e timer_instance) {
    TIM_TypeDef* TIMx = get_timer_peripheral(timer_instance);
    if (TIMx == NULL) return -1;

    // Bật bộ đếm
    TIMx->CR1 |= TIM_CR1_CEN;
    return 0;
}

int8_t PWM_Stop(PWM_TimerInstance_e timer_instance) {
    TIM_TypeDef* TIMx = get_timer_peripheral(timer_instance);
    if (TIMx == NULL) return -1;

    // Tắt bộ đếm
    TIMx->CR1 &= ~TIM_CR1_CEN;
    return 0;
}
