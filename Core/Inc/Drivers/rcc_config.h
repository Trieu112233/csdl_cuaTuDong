/*
 * rcc_config.h
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#ifndef INC_DRIVERS_RCC_CONFIG_H_
#define INC_DRIVERS_RCC_CONFIG_H_

#include <stdint.h>

#define HCLK_FREQUENCY_HZ      84000000UL
#define PCLK1_FREQUENCY_HZ     (HCLK_FREQUENCY_HZ / 2) // Vì PPRE1 là DIV2
#define PCLK2_FREQUENCY_HZ     (HCLK_FREQUENCY_HZ / 1) // Vì PPRE2 là DIV1

void SystemClock_Config(void);

#endif /* INC_DRIVERS_RCC_CONFIG_H_ */
