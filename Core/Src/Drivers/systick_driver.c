/*
 * systick_driver.c
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#include "systick_driver.h"
#include "stm32f401xe.h"
#include "rcc_config.h"

static volatile uint32_t g_sysTickCounter = 0;

void SysTick_Init(void){
	// 1. Tính toán giá trị nạp cho SysTick để có tick mỗi 1ms
	// (HCLK / 1000) - 1 vì SysTick đếm từ LOAD_VAL về 0 (LOAD_VAL + 1 chu kỳ)
	uint32_t reload_value = (HCLK_FREQUENCY_HZ / 1000U) - 1U;

	// 2. Cấu hình SysTick
    // Vô hiệu hóa SysTick trước khi cấu hình
    SysTick->CTRL = 0;

    // Nạp giá trị reload
    SysTick->LOAD = reload_value;

    // Đặt ưu tiên ngắt SysTick cho Cortex-M4 (4 bit ưu tiên)
    NVIC_SetPriority(SysTick_IRQn, 15); // Mức ưu tiên 15 (thấp nhất nếu 4 bit prio)

    // Reset bộ đếm SysTick hiện tại
    SysTick->VAL = 0;

    // Kích hoạt SysTick với nguồn clock là HCLK và bật ngắt
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk; // CLKSOURCE = AHB clock
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;   // Enable SysTick interrupt
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;    // Enable SysTick
}

void Delay_ms(volatile uint32_t ms){
	uint32_t start_tick = g_sysTickCounter;
	while ((g_sysTickCounter - start_tick) < ms){

	}
}

uint32_t GetTick(void){
	return g_sysTickCounter;
}

void IncTick(void){
	g_sysTickCounter ++;
}

void SysTick_Handler(void){
	IncTick();
}
