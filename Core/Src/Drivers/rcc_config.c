/*
 * rcc_config.c
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#include "rcc_config.h"
#include "stm32f401xe.h"

#ifndef HSE_VALUE
#define HSE_VALUE    8000000U // Giá trị thạch anh ngoài (Hz)
#endif

// Với HSE = 8MHz, để đạt SYSCLK = 84MHz:
// PLLM = 8   => Input cho PLL = 8MHz / 8 = 1MHz
// PLLN = 336 => VCO output = 1MHz * 336 = 336MHz
// PLLP = 4   => SYSCLK = 336MHz / 4 = 84MHz
#define PLL_M_VAL      8
#define PLL_N_VAL      336
#define PLL_P_VAL      4   // Giá trị thực tế (2, 4, 6, 8)
#define PLL_Q_VAL      7   // Cho USB OTG FS, SDIO. (336MHz / 7 = 48MHz)

// Helper để chuyển đổi PLL_P_VAL (2,4,6,8) thành giá trị ghi vào thanh ghi (0,1,2,3)
#if (PLL_P_VAL == 2)
  #define PLL_P_REG_VAL 0x00U
#elif (PLL_P_VAL == 4)
  #define PLL_P_REG_VAL 0x01U
#elif (PLL_P_VAL == 6)
  #define PLL_P_REG_VAL 0x02U
#elif (PLL_P_VAL == 8)
  #define PLL_P_REG_VAL 0x03U
#else
  #error "Invalid PLL_P_VAL. Must be 2, 4, 6, or 8."
#endif

void SystemClock_Config(void) {
    uint32_t temp_reg; // Biến tạm, tránh warning unused nếu không có MODIFY_REG

    // 1. Bật HSE và đợi nó ổn định (nếu dùng HSE là nguồn PLL)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // 2. Cấu hình Power Control Register
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    temp_reg = PWR->CR;
    temp_reg &= ~PWR_CR_VOS;
    temp_reg |= PWR_CR_VOS_1; // Scale 2
    PWR->CR = temp_reg;

    // 3. Cấu hình các hệ số chia cho HCLK, PCLK1, PCLK2
    temp_reg = RCC->CFGR;
    temp_reg &= ~RCC_CFGR_HPRE;
    temp_reg |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR = temp_reg;

    temp_reg = RCC->CFGR;
    temp_reg &= ~RCC_CFGR_PPRE1;
    temp_reg |= RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR = temp_reg;

    temp_reg = RCC->CFGR;
    temp_reg &= ~RCC_CFGR_PPRE2;
    temp_reg |= RCC_CFGR_PPRE2_DIV1;
    RCC->CFGR = temp_reg;

    // 4. Cấu hình PLL
    RCC->CR &= ~RCC_CR_PLLON;
    while(RCC->CR & RCC_CR_PLLRDY);

    RCC->PLLCFGR = (PLL_Q_VAL << RCC_PLLCFGR_PLLQ_Pos) |
                   RCC_PLLCFGR_PLLSRC_HSE |
                   (PLL_P_REG_VAL << RCC_PLLCFGR_PLLP_Pos)|
                   (PLL_N_VAL << RCC_PLLCFGR_PLLN_Pos) |
                   (PLL_M_VAL << RCC_PLLCFGR_PLLM_Pos);

    // 5. Bật PLL và đợi nó ổn định
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // 6. Cấu hình Flash latency
    temp_reg = FLASH->ACR;
    temp_reg &= ~FLASH_ACR_LATENCY;
    temp_reg |= FLASH_ACR_LATENCY_2WS;
    temp_reg |= FLASH_ACR_PRFTEN;
    FLASH->ACR = temp_reg;

    // 7. Chọn PLL làm nguồn SYSCLK
    temp_reg = RCC->CFGR;
    temp_reg &= ~RCC_CFGR_SW;
    temp_reg |= RCC_CFGR_SW_PLL;
    RCC->CFGR = temp_reg;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
