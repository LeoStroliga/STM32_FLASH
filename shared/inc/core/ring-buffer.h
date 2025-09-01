#ifndef INC_RING_BUFFER_H
#define INC_RING_BUFFER_H

#include "common-defines.h"

typedef struct ring_buffer_t{

    uint8_t* buffer;
    uint32_t mask;
    volatile uint32_t read_index;
    volatile uint32_t write_index;

}ring_buffer_t;

void ring_buffer_setup(ring_buffer_t* rb, uint8_t* buffer, uint32_t size);
bool ring_buffer_empty(ring_buffer_t* rb);
uint32_t ring_buffer_write_status(ring_buffer_t* rb);
uint32_t ring_buffer_read_status(ring_buffer_t* rb);
bool ring_buffer_write(ring_buffer_t* rb, uint8_t byte);
bool ring_buffer_read(ring_buffer_t* rb, uint8_t* byte);



#endif