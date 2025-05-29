/*
 * pwm_driver.h
 *
 *  Created on: May 28, 2025
 *      Author: Admin
 */

#ifndef INC_DRIVERS_PWM_DRIVER_H_
#define INC_DRIVERS_PWM_DRIVER_H_

#include <stdint.h>
#include "gpio_driver.h"
#include "stm32f401xe.h"
#include "rcc_config.h"

// Enum để chọn Timer
typedef enum {
    PWM_TIMER2 = 0,
    PWM_TIMER3,
    PWM_TIMER4,
    PWM_TIMER5
} PWM_TimerInstance_e;

// Enum để chọn kênh Timer (1 đến 4)
typedef enum {
    PWM_CHANNEL_1 = 1,
    PWM_CHANNEL_2 = 2,
    PWM_CHANNEL_3 = 3,
    PWM_CHANNEL_4 = 4
} PWM_TimerChannel_e;

/**
 * @brief Initializes a specific Timer channel for PWM generation.
 * @param timer_instance: Which Timer to use (e.g., PWM_TIMER2).
 * @param channel: Which channel of the Timer to use (e.g., PWM_CHANNEL_1).
 * @param pwm_frequency_hz: Desired PWM frequency in Hz.
 * @param gpio_port: The GPIO port for the PWM output pin (e.g., GPIOA).
 * @param gpio_pin_number: The pin number (0-15) for the PWM output.
 * @param gpio_af_mode: The Alternate Function mode for this TIM/Channel combination.
 *
 * @note  The duty cycle is initially set to 0%.
 *        This function handles enabling clocks for Timer and GPIO,
 *        and configuring GPIO pin to Alternate Function.
 * @return 0 on success, -1 on error (e.g., invalid parameters).
 */
int8_t PWM_InitChannel(PWM_TimerInstance_e timer_instance, PWM_TimerChannel_e channel,
                       uint32_t pwm_frequency_hz, GPIO_TypeDef* gpio_port, uint8_t gpio_pin_number,
					   uint8_t gpio_af_mode);

/**
 * @brief Sets the duty cycle for a previously initialized PWM channel.
 * @param timer_instance: Which Timer to use.
 * @param channel: Which channel of the Timer.
 * @param duty_cycle_percent: Desired duty cycle in percent (0.0 to 100.0).
 *                            Values outside this range will be clamped.
 * @return 0 on success, -1 on error.
 */
int8_t PWM_SetDutyCycle(PWM_TimerInstance_e timer_instance, PWM_TimerChannel_e channel, float duty_cycle_percent);

/**
 * @brief Starts the PWM generation on the specified Timer.
 * @param timer_instance: Which Timer to start.
 * @return 0 on success, -1 on error.
 */
int8_t PWM_Start(PWM_TimerInstance_e timer_instance);

/**
 * @brief Stops the PWM generation on the specified Timer.
 * @param timer_instance: Which Timer to stop.
 * @return 0 on success, -1 on error.
 */
int8_t PWM_Stop(PWM_TimerInstance_e timer_instance);

#endif /* INC_DRIVERS_PWM_DRIVER_H_ */
