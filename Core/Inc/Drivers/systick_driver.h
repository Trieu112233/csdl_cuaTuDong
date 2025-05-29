/*
 * systick_driver.h
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#ifndef INC_DRIVERS_SYSTICK_DRIVER_H_
#define INC_DRIVERS_SYSTICK_DRIVER_H_

#include <stdint.h>

void SysTick_Init(void);
void Delay_ms(volatile uint32_t ms);
uint32_t GetTick(void);
void IncTick(void);

#endif /* INC_DRIVERS_SYSTICK_DRIVER_H_ */
