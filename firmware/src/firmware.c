//#define STM32F4
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "core/system.h"
#include "core/uart.h"
#include "core/fw_flash.h"

#define BOOTLOADER_SIZE (0x8000U)
#define UART_PORT       (GPIOA)
#define RX_PIN1          (GPIO10)
#define TX_PIN1          (GPIO9)
#define RX_PIN2          (GPIO3)
#define TX_PIN2          (GPIO2)
#define FLASH_SECTOR_6_ADDRESS   (0x08040000)
#define FLASH_SECTOR_6_SIZE   (0x20000)
#define FLASH_SECTOR_7_ADDRESS   (0x08060000)
#define FLASH_SECTOR_7_SIZE   (0x20000)


static void gpio_setup(void){  
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN1|RX_PIN1|TX_PIN2|RX_PIN2);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN1|RX_PIN1|TX_PIN2|RX_PIN2);
}

//$GNRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
//Time: 12:34:56.00 UTC | Lat: 45.123456 | Lon: 15.987654

const bool format_raw_gps_data(const char* data, char *out, uint32_t out_size){
     if (strncmp(data, "$GNRMC", 6) != 0 && strncmp(data, "$GPRMC", 6) != 0) {
       
        return false; // Not an RMC sentence
    }
 
    char buff[256];
    strncpy(buff,data,256);
    buff[255]='\0';
  
    char *token;
    int field = 0;
    char *time_str = NULL,*status =NULL, *lat_str = NULL, *ns = NULL, *lon_str = NULL, *ew = NULL;

    token = strtok(buff,",");
   
      while (token != NULL) {
        field++;
        if (field == 2) time_str = token;  // UTC time
        if (field == 3) status = token;     //Valid or invalid
        if (field == 4) lat_str = token;   // Latitude
        if (field == 5) ns = token;
        if (field == 6) lon_str = token;   // Longitude
        if (field == 7) ew = token;
        token = strtok(NULL, ",");
        
    }

    if (!status || status[0] != 'A'){
        
        return false;
    }     //Invalid status

        if (lat_str && lon_str && ns && ew && time_str) {
            
    
        // Convert latitude
        int lat_decimal_pos=3;
        if(lat_str[0]=='0'){
            lat_decimal_pos=2;
        }
        else if(lat_str[1]=='0'){
            lat_decimal_pos=1;
        }
       int lat_deg = atoi(lat_str) / 100;
        int lat_min = atoi(lat_str) % 100;
        int lat_millionths = lat_deg * 1000000 + lat_min * 1000000 / 60;
        if (*ns == 'S') lat_millionths = -lat_millionths;

        // Convert longitude
        int lon_decimal_pos=3;
        if(lon_str[0]=='0'){
            lon_decimal_pos=2;
        }
        else if(lon_str[1]=='0'){
            lon_decimal_pos=1;
        }
        int lon_deg = atoi(lon_str) / 100;
        int lon_min = atoi(lon_str) % 100;
        int lon_millionths = lon_deg * 1000000 + lon_min * 1000000 / 60;
        if (*ew == 'W') lon_millionths = -lon_millionths;
       
       int hour=0, min=0, sec_int=0, sec_frac=0;
if (strlen(time_str) >= 9) {
    hour = (time_str[0]-'0')*10 + (time_str[1]-'0');
    min  = (time_str[2]-'0')*10 + (time_str[3]-'0');
    sec_int  = (time_str[4]-'0')*10 + (time_str[5]-'0');
    sec_frac = (time_str[7]-'0')*10 + (time_str[8]-'0');  
}
    int pos = 0;
    // Time:
    if(pos + 5 < out_size){
        out[pos++] = 'T';
        out[pos++] = 'i';
        out[pos++] = 'm';
        out[pos++] = 'e';
        out[pos++] = ':';
    }
    // HH:MM:SS
    if(pos + 8 < out_size){
        out[pos++] = '0' + hour/10;
        out[pos++] = '0' + hour%10;
        out[pos++] = ':';
        out[pos++] = '0' + min/10;
        out[pos++] = '0' + min%10;
        out[pos++] = ':';
        out[pos++] = '0' + sec_int/10;
        out[pos++] = '0' + sec_int%10;
        out[pos++] = '.';
        out[pos++] = '0' + sec_frac/10;
        out[pos++] = '0' + sec_frac%10;
    }
    // separator
    if(pos+7 < out_size){
        out[pos++] = ' ';
        out[pos++] = 'U';
        out[pos++] = 'T';
        out[pos++] = 'C';
        out[pos++] = ' ';
        out[pos++] = '|';
        out[pos++] = ' ';
    }

    // Lat:
    if(pos+4 < out_size){
        out[pos++] = 'L';
        out[pos++] = 'a';
        out[pos++] = 't';
        out[pos++] = ':';
    }
    // integer to string for latitude
    char buf[12];
    itoa(lat_millionths, buf, 10);
    int buf_len = strlen(buf);
    for(int i=0; buf[i]!='\0' && pos < out_size-1; i++){
        if(i==(buf_len-6)){
            out[pos++]='.';
            continue;
        }
        out[pos++] = buf[i];
    }

    // separator
    if(pos+4 < out_size){
        out[pos++] = ' ';
        out[pos++] = '|';
        out[pos++] = ' ';
        out[pos++] = 'L';
        out[pos++] = 'o';
        out[pos++] = 'n';
        out[pos++] = ':';
    }

    // longitude
    //$GNRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
//Time: 12:34:56.00 UTC | Lat: 45.123456 | Lon: 15.987654
    itoa(lon_millionths, buf, 10);
    buf_len = strlen(buf);
    for(int i=0; buf[i]!='\0' && pos < out_size-1; i++){
        if(i==(buf_len-6)){
            out[pos++]='.';
            continue;
        }
        out[pos++] = buf[i];
    }

        out[pos] = '\0';
    
        return true;

       
    }

}



int main(void)
{

    system_setup();
    gpio_setup();
    gpio_set(GPIOC, GPIO13);
    uart_setup1();
    uart_setup2();

    uint8_t flash_data[1024]  ={0};
    //fw_flash_erase_sector(6);
    //fw_flash_erase_sector(7);
    
    for(int i =0; i<1024;i++){
        flash_data[i]=0xaa;
    }
     uint32_t data_length = 64*sizeof(char);
    uint64_t start_time = system_get_ticks();
    uint8_t raw_gps_data[128]={'\0'};
    uint8_t gps_data[128]={'\0'};
    uint32_t offset = 0;
    uint32_t cnt = 0;
  
    static uint8_t* flash_write_addr = (const uint8_t *)FLASH_SECTOR_6_ADDRESS;
    while(*flash_write_addr!=0xff){
        flash_write_addr+=data_length;
        if(flash_write_addr==(const uint8_t *)FLASH_SECTOR_6_ADDRESS){
            fw_flash_erase_sector(6);
            break;
        }
    }
  
      
gpio_toggle(GPIOC, GPIO13);
   // fw_flash_write((uint32_t)flash_write_addr,flash_data,strlen(flash_data));
    while(1)
    {
      
      


      if(system_get_ticks()-start_time>=1000)
        {
        gpio_toggle(GPIOC, GPIO13);
        
        start_time=system_get_ticks();
     
        }
           // podatci s gps modula
        if(uart_data_available2()){
           

         
           
           uint8_t data2 = uart_read_byte2();
           raw_gps_data[cnt]=data2;
           cnt++;
        
           
           if (data2 == '\n') {
    // Optionally remove preceding '\r'
                    raw_gps_data[cnt] = '\0';
                 if (cnt > 1 && raw_gps_data[cnt - 2] == '\r') {
                     raw_gps_data[cnt - 2] = '\0'; // replace '\r' with null
                    } else {
                    raw_gps_data[cnt - 1] = '\0'; // replace '\n' with null
                      }
                        if(cnt >= 2 && raw_gps_data[cnt-2] == '\r')
                        raw_gps_data[cnt-2] = '\0';
                        else
                        raw_gps_data[cnt-1] = '\0';

                     uart_write2(raw_gps_data, strlen((char*)raw_gps_data));
                     format_raw_gps_data(raw_gps_data,(char*)gps_data,sizeof(gps_data));
                     uart_write2(gps_data, strlen((char*)gps_data));
                     size_t len = strlen(gps_data);
                        for(size_t i = len; i < 64; i++){
                            gps_data[i] = (char)0xAA;  // Padding for gps_data
                            }
                            gps_data[64] = '\0';
                    uint32_t a = strlen((char*)gps_data)/10;
                    uint32_t b = strlen((char*)gps_data)%10;
                     uart_write_byte2('\n');
                    uart_write2(gps_data, strlen((char*)gps_data));
                    uart_write_byte2('\n');
                    uart_write_byte2(a+'0');
                    uart_write_byte2(b+'0');
                    uart_write2((char*)gps_data, 64);  
                    //flash_write_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
                    fw_flash_write((uint32_t)flash_write_addr,gps_data,strlen(gps_data));
                    uint8_t buffer[128];
                    fw_flash_read((uint32_t)flash_write_addr,buffer,128);
                    uart_write2(buffer, 128);
                    flash_write_addr+=data_length;
                    //if sectors are full
                    if(flash_write_addr==(FLASH_SECTOR_6_SIZE+FLASH_SECTOR_6_SIZE)){  //end of sector 6
                    fw_flash_erase_sector(7);
                    }
                    else if(flash_write_addr==(FLASH_SECTOR_7_SIZE+FLASH_SECTOR_7_ADDRESS)){ //end of sector 7
                    fw_flash_erase_sector(6);
                    flash_write_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
                    }
                    
                     cnt = 0;
}
          

            //fw_flash_write((uint32_t)flash_write_addr,data2,data_length);

        }

        //podatci s terminala
       /* if(uart_data_available1()){
           // gpio_toggle(GPIOC, GPIO13);

            uint8_t data1 = uart_read_byte1();
            uint8_t data2 = uart_read_byte2();
            uart_write_byte1(data1);
            uart_write_byte1(data2);
            uart_write_byte2(data1);
            uart_write_byte2(data2);
            //citanje gps data sa uarta
            

        }*/

    }

    return 0;
}

// file: main.c  (libopencm3, STM32F4 primjer)
/*
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <string.h>
#include <stdio.h>

#include "core/system.h"
#include "core/uart.h"

#define RYLR_USART USART2
#define DEBUG_USART USART1

#define BOOTLOADER_SIZE (0x8000U)
#define UART_PORT       (GPIOA)
#define RX_PIN1          (GPIO10)
#define TX_PIN1          (GPIO9)
#define RX_PIN2          (GPIO3)
#define TX_PIN2          (GPIO2)

#define LORA_BAUD 115200
#define DEBUG_BAUD 115200

#define DEV_EUI  "70B3D57ED0072A39"
#define APP_EUI  "70B3D57ED0012345"
#define APP_KEY  "D4212159C20535F0C91F5CD169B31804"


static void gpio_setup(void){  
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN1|RX_PIN1|TX_PIN2|RX_PIN2);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN1|RX_PIN1|TX_PIN2|RX_PIN2);
}


// Simple delay (approx)
static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 8000; i++) __asm__("nop");
}

// USART init
// Send string over USART
static void usart_send_str2(const char *str) {
    uart_write2((uint8_t*)str, strlen(str));
    uart_write_byte2('\r');  // dodaj CR
    uart_write_byte2('\n');  // dodaj LF
}

// Debug print
static void debug(const char *msg) {
      uart_write1((uint8_t*)msg, strlen(msg));
    uart_write_byte1('\r');  // dodaj CR
    uart_write_byte1('\n');  // dodaj LF
}

// Read response from LoRa (with timeout)// Čita odgovor LoRa modula u predani buffer
static void lora_read_response(char *resp, uint32_t resp_size, uint32_t timeout_ms) {
    uint32_t idx = 0;
    uint32_t start = system_get_ticks();
     uint8_t c;
    while ((system_get_ticks() - start) < timeout_ms) {
        while (uart_data_available2()) {  // koristi ring-buffer funkciju
            c = uart_read_byte2();
            if (idx < resp_size - 1) resp[idx++] = c;
            if (idx >= 2 && resp[idx-2]=='\r' && resp[idx-1]=='\n') break;
        }
        if (idx >= 2 && resp[idx-2]=='\r' && resp[idx-1]=='\n') break;
    }
    resp[idx] = '\0';
}

// Šalje AT komandu i vraća odgovor u predani buffer
static void lora_send_cmd(const char *cmd) {
    usart_send_str2(cmd);
   // lora_read_response(resp, resp_size, timeout_ms);
}


// LoRa OTAA setup
static void lora_setup(void) {
    char response[256];
    debug("Configuring LoRa...");
 delay_ms(3000);
    // Reset module
    debug("AT+RESET");
    lora_send_cmd("AT+RESET");
     delay_ms(3000);
     lora_read_response(response, 256, 2000);
     debug(response);
     
    // Set LoRaWAN mode
    debug("AT+MODE=0");
    lora_send_cmd("AT+MODE=0");
    delay_ms(3000);
    lora_read_response(response, 256, 2000);
    debug(response);
 
    debug("AT$DEVEUi?");
    lora_send_cmd("AT$DEVEUI?");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
  debug(response);
    // Set OTAA credentials
    debug("AT$DEVEUi");
    lora_send_cmd("AT$DEVEUI=70B3D57ED0072A39");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
  debug(response);
  debug("AT$DEVEUi?");
    lora_send_cmd("AT$DEVEUI?");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
  debug(response);
   
    debug("AT$APPEUI");
    lora_send_cmd("AT$APPEUI=70B3D57ED0012345");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
      debug(response);
       
       debug("AT$APPKEY");
    lora_send_cmd("AT$APPKEY=D4212159C20535F0C91F5CD169B31804");
     delay_ms(3000);
     lora_read_response(response, 256, 2000);
  debug(response);
   

// Set frequency band
 debug("AT+BAND");
    lora_send_cmd("AT+BAND=868000000");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
     debug("AT$MODE=1");
    lora_send_cmd("AT$MODE=1");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
     debug("AT$NWK=1");
    lora_send_cmd("AT$NWK=1");
     delay_ms(3000);
     lora_read_response(response, 256, 2000);
  debug(response);
    // Join network
    debug("AT$JOIN");
    lora_send_cmd("AT$JOIN");
    delay_ms(3000);
    lora_read_response(response, sizeof(response), 500);
     debug(response);
       delay_ms(30000);
    debug("PROSAO DELAY");
    if(uart_data_available2()==true)
    {
        uart_read2((uint8_t*)response,256);
        debug(response);
    }
    debug("Drugi read");
    if(uart_data_available2()==true)
    {
        uart_read2((uint8_t*)response,256);
        debug(response);
    }
     debug("AT$JOINSTATUS");
    lora_send_cmd("AT$JOINSTATUS");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
  
    debug("AT$NJS");
    lora_send_cmd("AT$NJS");
    delay_ms(3000);
     lora_read_response(response, 256, 2000);
    
    debug("Joining network...");
    delay_ms(2000);
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
    gpio_setup();
    uart_setup1();
    uart_setup2();
    
    debug("STM32 LoRa OTAA start");

    lora_setup();
    char response[256];

    while (1) {
        debug("Sending data...");
        lora_send_cmd("AT+SEND=2,9,HELLO_TTN");
        lora_read_response(response,256,3000);
         if(uart_data_available2()==true)
    {
        uart_read2((uint8_t*)response,256);
        debug(response);
    }
        delay_ms(10000);
    }

    return 0;
}*/