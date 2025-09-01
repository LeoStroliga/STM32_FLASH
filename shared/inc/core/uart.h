#ifndef INC_UART_H
#define INC_UART_H

#include "common-defines.h"


void uart_setup1(void);
void uart_setup2(void);
void uart_write1(uint8_t* data, const uint32_t lenght);
void uart_write2(uint8_t* data, const uint32_t lenght);
void uart_write_byte1(uint8_t data);
void uart_write_byte2(uint8_t data);
uint32_t uart_read1(uint8_t* data, const uint32_t lenght);
uint32_t uart_read2(uint8_t* data, const uint32_t lenght);
uint8_t uart_read_byte1(void);
uint8_t uart_read_byte2(void);
bool uart_data_available1();
bool uart_data_available2();
uint32_t uart_data_read_status1();
uint32_t uart_data_read_status2();
uint32_t uart_data_write_status1();
uint32_t uart_data_write_status2();

#endif // INC_UART_H

