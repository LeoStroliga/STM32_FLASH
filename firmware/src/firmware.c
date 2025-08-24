//#define STM32F4
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "core/system.h"
#include "core/uart.h"


#define BOOTLOADER_SIZE (0x8000U)
#define UART_PORT       (GPIOA)
#define RX_PIN          (GPIO3)
#define TX_PIN          (GPIO2)


static void gpio_setup(void){  
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN|RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN|RX_PIN);
}



int main(void)
{

    system_setup();
    gpio_setup();
    gpio_set(GPIOC, GPIO13);
    uart_setup();
  

 
    uint64_t start_time = system_get_ticks();
           // gpio_toggle(GPIOC, GPIO13);
             for (int i = 0; i < 5000000; ++i) 
        {//gpio_toggle(GPIOC, GPIO13);
            __asm__("nop");
        }
gpio_toggle(GPIOC, GPIO13);
    
    while(1)
    {
      
      


      if(system_get_ticks()-start_time>=200)
        {
       // gpio_toggle(GPIOC, GPIO13);
        
        start_time=system_get_ticks();

        uart_write_byte('s');
        }

        if(uart_data_available()){
           // gpio_toggle(GPIOC, GPIO13);
            uint8_t data = uart_read_byte();
            uart_write_byte(data);
        }

    }

    return 0;
}
/*
int main(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    //gpio_toggle(GPIOC, GPIO13);   // toggle LED
    while (1) {
        gpio_toggle(GPIOC, GPIO13);   // toggle LED
        for (int i = 0; i < 5000000; ++i) 
        {
            __asm__("nop");
        }
    }
}
*/
