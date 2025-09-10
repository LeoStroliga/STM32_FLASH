#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include "core/ring-buffer.h"
#include <stdbool.h>
#include <stdint.h>

#define BAUD_RATE 115200
#define RING_BUFFER_SIZE 1024

// USART1
static ring_buffer_t rb1 = {0};
static uint8_t data_buffer1[RING_BUFFER_SIZE] = {0};

// USART2
static ring_buffer_t rb2 = {0};
static uint8_t data_buffer2[RING_BUFFER_SIZE] = {0};

// ------------------- USART1 ISR -------------------
void usart1_isr(void) {
    bool overrun_occurred = usart_get_flag(USART1, USART_FLAG_ORE);
    bool received_data = usart_get_flag(USART1, USART_FLAG_RXNE);

    if (received_data || overrun_occurred) {
        ring_buffer_write(&rb1, (uint8_t)usart_recv(USART1));
    }
}

// ------------------- USART2 ISR -------------------
void usart2_isr(void) {
    bool overrun_occurred = usart_get_flag(USART2, USART_FLAG_ORE);
    bool received_data = usart_get_flag(USART2, USART_FLAG_RXNE);

    if (received_data || overrun_occurred) {
        ring_buffer_write(&rb2, (uint8_t)usart_recv(USART2));
    }
}

// ------------------- Setup functions -------------------
void uart_setup1(void) {
    ring_buffer_setup(&rb1, data_buffer1, RING_BUFFER_SIZE);
    rcc_periph_clock_enable(RCC_USART1);

    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_databits(USART1, 8);
    usart_set_baudrate(USART1, BAUD_RATE);
    usart_set_parity(USART1, 0);
    usart_set_stopbits(USART1, 1);

    usart_enable_rx_interrupt(USART1);
    nvic_enable_irq(NVIC_USART1_IRQ);

    usart_enable(USART1);
}

void uart_setup2(void) {
    ring_buffer_setup(&rb2, data_buffer2, RING_BUFFER_SIZE);
    rcc_periph_clock_enable(RCC_USART2);

    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_databits(USART2, 8);
    usart_set_baudrate(USART2, BAUD_RATE);
    usart_set_parity(USART2, 0);
    usart_set_stopbits(USART2, 1);

    usart_enable_rx_interrupt(USART2);
    nvic_enable_irq(NVIC_USART2_IRQ);

    usart_enable(USART2);
}

// ------------------- USART1 write/read -------------------
void uart_write_byte1(uint8_t data) {
    usart_send_blocking(USART1, data);
}

void uart_write1(uint8_t* data, const uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        uart_write_byte1(data[i]);
    }
}

uint32_t uart_read1(uint8_t* data, const uint32_t length) {
    if (length == 0) return 0;

    uint32_t i;
    for (i = 0; i < length; i++) {
        if (!ring_buffer_read(&rb1, &data[i])) break;
    }
    return i;
}

uint8_t uart_read_byte1(void) {
    uint8_t byte = 0;
    uart_read1(&byte, 1);
    return byte;
}

bool uart_data_available1(void) {
    return !ring_buffer_empty(&rb1);
}

uint32_t uart_data_read_status1(){
    return ring_buffer_read_status(&rb1);

}
uint32_t uart_data_write_status1(){
    return ring_buffer_write_status(&rb1);

}

// ------------------- USART2 write/read -------------------
void uart_write_byte2(uint8_t data) {
    usart_send_blocking(USART2, data);
}

void uart_write2(uint8_t* data, const uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        uart_write_byte2(data[i]);
    }
}

uint32_t uart_read2(uint8_t* data, const uint32_t length) {
    if (length == 0) return 0;

    uint32_t i;
    for (i = 0; i < length; i++) {
        if (!ring_buffer_read(&rb2, &data[i])) break;
    }
    return i;
}

uint8_t uart_read_byte2(void) {
    uint8_t byte = 0;
    uart_read2(&byte, 1);
    return byte;
}

bool uart_data_available2(void) {
    return !ring_buffer_empty(&rb2);
}

uint32_t uart_data_read_status2(){
    return ring_buffer_read_status(&rb2);

}
uint32_t uart_data_write_status2(){
    return ring_buffer_write_status(&rb2);

}
