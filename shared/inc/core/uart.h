#ifndef INC_UART_H
#define INC_UART_H

#include "common-defines.h"

void uart_setup(void);
void uart_write(uint8_t* data, const uint32_t lenght,int usart_number);
void uart_write_byte(uint8_t data, int usart_number);
uint32_t uart_read(uint8_t* data, const uint32_t lenght,int usart_number);
uint8_t uart_read_byte(int usart_number);
bool uart_data_available(int usart_number);

#endif // INC_UART_H