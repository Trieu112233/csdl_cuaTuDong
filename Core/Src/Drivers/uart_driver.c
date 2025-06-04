/*
 * uart_driver.c
 *
 *  Created on: May 27, 2025
 *      Author: Admin
 */

#include "uart_driver.h"
#include "gpio_driver.h"

// --- RX Ring Buffer cho USART2 ---
static volatile uint8_t  g_uart2_rx_buffer[UART2_RX_BUFFER_SIZE];
static volatile uint16_t g_uart2_rx_buffer_head = 0;
static volatile uint16_t g_uart2_rx_buffer_tail = 0;
static volatile uint16_t g_uart2_rx_buffer_count = 0;

// --- TX Buffer cho USART2 ---
static volatile uint8_t  g_uart2_tx_buffer[UART2_TX_BUFFER_SIZE];
static volatile uint16_t g_uart2_tx_buffer_head = 0; // Index để ISR đọc
static volatile uint16_t g_uart2_tx_buffer_tail = 0; // Index để application ghi
static volatile uint16_t g_uart2_tx_buffer_count = 0; // Số byte đang chờ gửi
static volatile bool     g_uart2_tx_busy = false;

// --- TX Complete Callback ---
static uart_tx_complete_callback_t g_uart2_tx_complete_callback = NULL;

// Thêm biến theo dõi lỗi UART
static volatile uint8_t g_uart2_error_flags = 0;

void UART2_Init(uint32_t baudrate, uint32_t word_length, uint32_t parity, uint32_t stop_bits) {
    // 1. Bật clock cho USART2 và GPIOA
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    GPIO_ClockCmd(GPIOA, CLOCK_CMD_ENABLE);

    // 2. Cấu hình chân GPIOA PA2 (TX) và PA3 (RX) cho AF7 (USART2)
    GPIO_InitPin(GPIOA, (1U << 2), GPIO_MODE_AF, GPIO_PULL_UP, GPIO_SPEED_HIGH, GPIO_OTYPE_PUSHPULL, 7); // PA2 TX
    GPIO_InitPin(GPIOA, (1U << 3), GPIO_MODE_AF, GPIO_PULL_UP, GPIO_SPEED_HIGH, GPIO_OTYPE_PUSHPULL, 7); // PA3 RX

    // 3. Vô hiệu hóa UART trước khi cấu hình
    USART2->CR1 &= ~USART_CR1_UE;

    // 4. Cấu hình Word Length, Parity
    USART2->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);
    USART2->CR1 |= word_length;
    USART2->CR1 |= parity;

    // 5. Cấu hình Stop Bits
    USART2->CR2 &= ~USART_CR2_STOP;
    USART2->CR2 |= stop_bits;

    // 6. Cấu hình Baud Rate với độ chính xác cao hơn
    uint32_t pclk1_freq = PCLK1_FREQUENCY_HZ; // Lấy từ rcc_config.h
    // Tính toán USARTDIV với độ chính xác cao hơn
    uint32_t integer_div = (25 * pclk1_freq) / (4 * baudrate);
    uint32_t mantissa = integer_div / 100;
    uint32_t fraction = ((integer_div - (mantissa * 100)) * 16 + 50) / 100;
        
    // Xử lý trường hợp fraction = 16
    if (fraction == 16) {
        mantissa++;
        fraction = 0;
    }
        
    USART2->BRR = (mantissa << 4) | (fraction & 0xFU);

    // 7. Cấu hình Control Register: Bật TX, RX và RXNE Interrupt
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    // USART2->CR3 &= ~USART_CR3_CTSE; // Tắt CTS flow control
    // USART2->CR3 &= ~USART_CR3_RTSE; // Tắt RTS flow control

    // 8. Reset trạng thái buffer
    g_uart2_rx_buffer_head = 0;
    g_uart2_rx_buffer_tail = 0;
    g_uart2_rx_buffer_count = 0;
    g_uart2_tx_buffer_head = 0;
    g_uart2_tx_buffer_tail = 0;
    g_uart2_tx_buffer_count = 0;
    g_uart2_tx_busy = false;

    // 9. Cấu hình ngắt trong NVIC
    NVIC_SetPriority(USART2_IRQn, 14); // Đặt mức ưu tiên
    NVIC_EnableIRQ(USART2_IRQn);

    // 10. Bật UART
    USART2->CR1 |= USART_CR1_UE;
}

bool UART2_SendBuffer_IT(const uint8_t* buffer, uint16_t length) {
    if (length == 0) return true;
    if (g_uart2_tx_busy || (UART2_TX_BUFFER_SIZE - g_uart2_tx_buffer_count) < length) {
        return false;
    }

    // Tạm thời vô hiệu hóa ngắt TXE để cập nhật buffer an toàn
    USART2->CR1 &= ~USART_CR1_TXEIE;

    for (uint16_t i = 0; i < length; i++) {
        g_uart2_tx_buffer[g_uart2_tx_buffer_tail] = buffer[i];
        g_uart2_tx_buffer_tail = (g_uart2_tx_buffer_tail + 1) % UART2_TX_BUFFER_SIZE;
    }
    // đảm bảo critical section
    __disable_irq(); // Critical section
    g_uart2_tx_buffer_count += length;
    g_uart2_tx_busy = true;
    __enable_irq();  // End critical section

    // Kích hoạt ngắt TXE để bắt đầu gửi byte đầu tiên từ ISR
    USART2->CR1 |= USART_CR1_TXEIE;

    return true;
}

bool UART2_IsTxBusy(void) {
    return g_uart2_tx_busy;
}

bool UART2_ReadByte_FromBuffer(uint8_t* data) {
    if (g_uart2_rx_buffer_count == 0) {
        return false; // Buffer rỗng
    }

    // Vô hiệu hóa chỉ ngắt UART2 thay vì tất cả các ngắt
    NVIC_DisableIRQ(USART2_IRQn);
    
    *data = g_uart2_rx_buffer[g_uart2_rx_buffer_head];
    g_uart2_rx_buffer_head = (g_uart2_rx_buffer_head + 1) % UART2_RX_BUFFER_SIZE;
    g_uart2_rx_buffer_count--;
    
    // Kích hoạt lại ngắt UART2
    NVIC_EnableIRQ(USART2_IRQn);

    return true;
}

uint16_t UART2_GetRxBufferCount(void) {
    // Đọc biến volatile
    return g_uart2_rx_buffer_count;
}

void UART2_RegisterTxCompleteCallback(uart_tx_complete_callback_t callback) {
    g_uart2_tx_complete_callback = callback;
}

uint8_t UART2_GetErrorFlags(void) {
    return g_uart2_error_flags;
}

void UART2_ClearErrorFlags(uint8_t flags) {
    __disable_irq();
    g_uart2_error_flags &= ~flags;
    __enable_irq();
}

void USART2_IRQHandler(void) {
    uint32_t sr_reg = USART2->SR; // Đọc thanh ghi SR một lần

    // --- Xử lý ngắt RXNE (Receive Data Register Not Empty) ---
    if ((sr_reg & USART_SR_RXNE) && (USART2->CR1 & USART_CR1_RXNEIE)) {
        uint8_t received_byte = (uint8_t)(USART2->DR & 0xFFU);

        if (g_uart2_rx_buffer_count < UART2_RX_BUFFER_SIZE) {
            g_uart2_rx_buffer[g_uart2_rx_buffer_tail] = received_byte;
            g_uart2_rx_buffer_tail = (g_uart2_rx_buffer_tail + 1) % UART2_RX_BUFFER_SIZE;
            g_uart2_rx_buffer_count++;
        } else {
            // Buffer RX đầy, đánh dấu lỗi bằng cách thêm cờ
            g_uart2_error_flags |= UART_ERROR_BUFFER_FULL;
        }
        // Cờ RXNE tự xóa khi đọc DR.
    }

    // --- Xử lý ngắt TXE (Transmit Data Register Empty) ---
    if ((sr_reg & USART_SR_TXE) && (USART2->CR1 & USART_CR1_TXEIE)) {
        if (g_uart2_tx_buffer_count > 0) {
            USART2->DR = g_uart2_tx_buffer[g_uart2_tx_buffer_head];
            g_uart2_tx_buffer_head = (g_uart2_tx_buffer_head + 1) % UART2_TX_BUFFER_SIZE;
            g_uart2_tx_buffer_count--;

            if (g_uart2_tx_buffer_count == 0) {
                // Đã gửi hết buffer, tắt ngắt TXE
                USART2->CR1 &= ~USART_CR1_TXEIE;
                // Bật ngắt TC (Transmission Complete) để biết khi nào byte cuối cùng thực sự ra khỏi shift register
                USART2->CR1 |= USART_CR1_TCIE;
            }
        }
        // Cờ TXE tự xóa khi ghi vào DR.
    }

    // --- Xử lý ngắt TC (Transmission Complete) ---
    if ((sr_reg & USART_SR_TC) && (USART2->CR1 & USART_CR1_TCIE)) {
        // Byte cuối cùng đã được gửi hoàn toàn
        USART2->CR1 &= ~USART_CR1_TCIE; // Tắt ngắt TC
        g_uart2_tx_busy = false;      // Đánh dấu  không còn bận gửi

        if (g_uart2_tx_complete_callback != NULL) {
            g_uart2_tx_complete_callback();
        }
        // Cờ TC được xóa bằng cách ghi 0 vào nó (hoặc đọc SR rồi ghi vào DR - nhưng ở đây đã xong TX)
        USART2->SR &= ~USART_SR_TC; // Xóa cờ TC
    }


    // --- Xử lý các cờ lỗi (Overrun, Noise, Framing, Parity) ---
    if (sr_reg & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        // Lưu lại lỗi để ứng dụng có thể kiểm tra
        if (sr_reg & USART_SR_ORE) g_uart2_error_flags |= UART_ERROR_OVERRUN;
        if (sr_reg & USART_SR_NE) g_uart2_error_flags |= UART_ERROR_NOISE;
        if (sr_reg & USART_SR_FE) g_uart2_error_flags |= UART_ERROR_FRAMING;
        if (sr_reg & USART_SR_PE) g_uart2_error_flags |= UART_ERROR_PARITY;
        
        // Đọc DR để xóa cờ lỗi
        volatile uint32_t temp_dr = USART2->DR;
        (void)temp_dr; // Tránh warning
    }
}


