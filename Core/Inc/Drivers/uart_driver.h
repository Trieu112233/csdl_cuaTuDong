/*
 * uart_driver.h
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#ifndef INC_DRIVERS_UART_DRIVER_H_
#define INC_DRIVERS_UART_DRIVER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32f401xe.h"
#include "rcc_config.h" // Để lấy PCLK1_FREQUENCY_HZ

// Kích thước buffer
#define UART2_RX_BUFFER_SIZE 128
#define UART2_TX_BUFFER_SIZE 128

// Định nghĩa cho các tham số UART
#define UART_WORDLENGTH_8B     0x00U
#define UART_PARITY_NONE       0x00U
#define UART_STOPBITS_1        0x00U

void UART2_Init(uint32_t baudrate, uint32_t word_length, uint32_t parity, uint32_t stop_bits);

/**
 * @brief Sends a buffer of data via USART2 using interrupt-driven TX.
 * @param buffer: Pointer to the data buffer.
 * @param length: Number of bytes to send.
 * @return true if data was accepted for transmission, false if TX buffer is busy.
 */
bool UART2_SendBuffer_IT(const uint8_t* buffer, uint16_t length);

/**
 * @brief Checks if USART2 is currently busy transmitting.
 * @return true if busy, false otherwise.
 */
bool UART2_IsTxBusy(void);

/**
 * @brief Reads a byte from the RX ring buffer.
 * @param data: Pointer to a variable to store the received byte.
 * @return true if a byte was read, false if the RX buffer is empty.
 */
bool UART2_ReadByte_FromBuffer(uint8_t* data);

/**
 * @brief Gets the number of bytes available in the RX ring buffer.
 * @return Number of bytes available.
 */
uint16_t UART2_GetRxBufferCount(void);

// Callback function pointer type for TX complete
typedef void (*uart_tx_complete_callback_t)(void);

/**
 * @brief Registers a callback function to be called when a TX buffer transmission is complete.
 * @param callback: Pointer to the callback function.
 */
void UART2_RegisterTxCompleteCallback(uart_tx_complete_callback_t callback);

void USART2_IRQHandler(void);

#endif /* INC_DRIVERS_UART_DRIVER_H_ */
