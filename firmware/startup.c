#include <stdint.h>

/* Function prototypes */
extern int main(void);
void Reset_Handler(void);
void sys_tick_handler(void);
void Default_Handler(void);
void usart2_isr(void);

/* Symbols defined in the linker script */
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _stack;

/* Vector table */
__attribute__ ((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void (*)(void))(&_stack),
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    0,0,0,0,
    Default_Handler,
    Default_Handler,
    0,
    Default_Handler,
    sys_tick_handler,
    [54] = usart2_isr   // USART2
};


/* Default handler used for unused IRQs */
void Default_Handler(void) {
    while (1);
}

/* Reset handler implementation */
void Reset_Handler(void) {
    // Copy .data section from flash to RAM
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // Zero initialize .bss section
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // Call the application's entry point
    main();

    // Loop forever if main returns
    while (1);
}
