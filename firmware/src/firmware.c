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
static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 8000; i++) __asm__("nop");
}
//$GNRMC,123519.00,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
//Time: 12:34:56.00 UTC | Lat: 45.123456 | Lon: 15.987654
//2025-09-08T12:34:56Z,16.17136,43.32502 for influx


// Convert string "DDMM.MMMM" or "DDDMM.MMMM" to millionths degrees
static int convert_gps_coord(const char *coord, char dir) {
    int deg_len = (coord[4] == '.' ? 2 : 3);  // Latitude 2 digits, Longitude 3 digits
    int deg = 0;
    for(int i=0;i<deg_len;i++) deg = deg*10 + (coord[i]-'0');

    int min = 0;
    for(int i=deg_len;i<deg_len+2;i++) min = min*10 + (coord[i]-'0');

    int frac = 0;
    int mul = 100000;
    for(int i=deg_len+3;i<deg_len+9;i++, mul/=10) {
        frac += (coord[i]-'0')*mul;
    }

    int result = deg*1000000 + (min*1000000)/60 + frac/60;
    if(dir=='S' || dir=='W') result = -result;
    return result;
}

// Format raw GPS RMC into "YYYY-MM-DDTHH:MM:SSZ,lon,lat"
static bool format_raw_gps_data(const char* data, char *out, uint32_t out_size) {
    if(strncmp(data,"$GNRMC",6)!=0 && strncmp(data,"$GPRMC",6)!=0) return false;

    char buff[256];
    strncpy(buff,data,255);
    buff[255]='\0';

    char *token;
    int field=0;
    char *time_str=NULL,*status=NULL,*lat_str=NULL,*ns=NULL,*lon_str=NULL,*ew=NULL,*date_str=NULL;

    token = strtok(buff,",");
    while(token) {
        field++;
        if(field==2) time_str=token;
        if(field==3) status=token;
        if(field==4) lat_str=token;
        if(field==5) ns=token;
        if(field==6) lon_str=token;
        if(field==7) ew=token;
        if(field==10) date_str=token;
        token = strtok(NULL,",");
    }

    if(!status || status[0]!='A') return false;
    if(!lat_str||!lon_str||!ns||!ew||!time_str) return false;

    int lat_millionths = convert_gps_coord(lat_str,*ns);
    int lon_millionths = convert_gps_coord(lon_str,*ew);
    int hour = 0, min = 0, sec_int = 0; 
    if (strlen(time_str) >= 6) { 
        hour = (time_str[0]-'0')*10 + (time_str[1]-'0'); 
        min = (time_str[2]-'0')*10 + (time_str[3]-'0'); 
        sec_int = (time_str[4]-'0')*10 + (time_str[5]-'0');
    }

    int day   = (date_str[0]-'0')*10 + (date_str[1]-'0');
    int month = (date_str[2]-'0')*10 + (date_str[3]-'0');
    int year  = 2000 + (date_str[4]-'0')*10 + (date_str[5]-'0');

    // DateTime YYYY-MM-DDTHH:MM:SSZ
    //2025-09-08T12:34:56Z,16.17136,43.32502

    int pos = 0; 
    if (pos + 20 < out_size) 
        { out[pos++] = '0' + (year/1000)%10;
        out[pos++] = '0' + (year/100)%10;
        out[pos++] = '0' + (year/10)%10;
        out[pos++] = '0' + (year%10); 
        out[pos++] = '-'; 
        out[pos++] = '0' + (month/10); 
        out[pos++] = '0' + (month%10); 
        out[pos++] = '-'; 
        out[pos++] = '0' + (day/10); 
        out[pos++] = '0' + (day%10); 
        out[pos++] = 'T'; out[pos++] = '0' + (hour/10); 
        out[pos++] = '0' + (hour%10); out[pos++] = ':'; 
        out[pos++] = '0' + (min/10); 
        out[pos++] = '0' + (min%10); 
        out[pos++] = ':'; out[pos++] = '0' + (sec_int/10); 
        out[pos++] = '0' + (sec_int%10); 
        out[pos++] = 'Z'; 
        out[pos++] = ','; }
    // Convert lon_millionths
    if(lon_millionths<0){ out[pos++]='-'; lon_millionths=-lon_millionths; }
    int lon_int = lon_millionths/1000000;
    int lon_frac = lon_millionths%1000000;
    char buf[12]; int len=0;
    if(lon_int==0) buf[len++]='0';
    else { int d[3],di=0,tmp=lon_int; while(tmp){d[di++]=tmp%10; tmp/=10;} for(int i=di-1;i>=0;i--) buf[len++]='0'+d[i]; }
    buf[len++]='.';
    int mul=100000; for(int i=0;i<6;i++,mul/=10) buf[len++]='0'+(lon_frac/mul)%10;
    for(int i=0;i<len;i++) out[pos++]=buf[i];
    out[pos++]=',';

    // Convert lat_millionths
    if(lat_millionths<0){ out[pos++]='-'; lat_millionths=-lat_millionths; }
    int lat_int = lat_millionths/1000000;
    int lat_frac = lat_millionths%1000000;
    len=0;
    if(lat_int==0) buf[len++]='0';
    else { int d[3],di=0,tmp=lat_int; while(tmp){d[di++]=tmp%10; tmp/=10;} for(int i=di-1;i>=0;i--) buf[len++]='0'+d[i]; }
    buf[len++]='.';
    mul=100000; for(int i=0;i<6;i++,mul/=10) buf[len++]='0'+(lat_frac/mul)%10;
    for(int i=0;i<len;i++) out[pos++]=buf[i];

    out[pos]='\0';
    return true;
}



int main(void)
{

    system_setup();
    gpio_setup();
    gpio_set(GPIOC, GPIO13);
    uart_setup1();
    uart_setup2();
     //fw_flash_erase_sector(6);
   //fw_flash_erase_sector(7);
   //gps setup
   uart_write2("@BSSL 0x20\r\n", 12);
   uart_write2("@GSOP 0 10000 0\r\n", 17);
    
    //lora setup
    uart_write1("AT+RESET\r\n", 10);
    delay_ms(500);
    uart_write1("AT+MODE=0\r\n", 11);
     delay_ms(500);
    uart_write1("AT+ADDRESS=1\r\n", 14);
     delay_ms(500);
    uart_write1("AT+NETWORKID=5\r\n", 16);
     delay_ms(500);
    uart_write1("AT+BAND=868000000\r\n", 19);
     delay_ms(500);
    uart_write1("AT+PARAMETER=9,7,1,4\r\n", 22);
     delay_ms(500);

// Send "HELLO" to address 2
//uart_write1("AT+SEND=2,5,HELLO\r\n", 19);

  
    //fw_flash_erase_sector(6);
    //fw_flash_erase_sector(7);
    
   
     uint32_t data_length = 64*sizeof(char);
    uint64_t start_time = system_get_ticks();
    uint8_t raw_gps_data[128]={'\0'};
    uint8_t gps_data[128]={'\0'};
    uint32_t offset = 0;
    uint32_t cnt = 0;
    uint8_t flash_data[128]={'\0'};
  
    static uint8_t* flash_write_addr = (const uint8_t *)FLASH_SECTOR_6_ADDRESS;
     static uint8_t* flash_read_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
    while(*flash_write_addr!=0xff){
        flash_write_addr+=data_length;
        if(flash_write_addr==(const uint8_t *)(FLASH_SECTOR_7_ADDRESS+FLASH_SECTOR_7_SIZE)){ //reached end of flash memory
            fw_flash_erase_sector(6);
            flash_write_addr = (const uint8_t *)FLASH_SECTOR_6_ADDRESS;
            break;
        }
    }
    if(*flash_read_addr==0xff&&*FLASH_SECTOR_7_ADDRESS!=0xff)
    flash_read_addr = (const uint8_t *)FLASH_SECTOR_7_ADDRESS;
   
  uint8_t sent_data_length_array[8]={'\0'};
  uint32_t sent_data_length=strlen("2025-09-08T12:34:56Z,16.171360,43.325020");
  itoa(sent_data_length,sent_data_length_array,10);
  uint8_t buffer[128];
      
gpio_toggle(GPIOC, GPIO13);
    uint8_t lora_data[26];
   // fw_flash_write((uint32_t)flash_write_addr,flash_data,strlen(flash_data));
    while(1)
    {
      
      


      if(system_get_ticks()-start_time>=3000)
        {
        gpio_toggle(GPIOC, GPIO13);
        
        start_time=system_get_ticks();
        fw_flash_read((uint32_t)flash_read_addr,flash_data,data_length);
        if(*FLASH_SECTOR_6_ADDRESS!=0xff||*FLASH_SECTOR_7_ADDRESS!=0xff)
        if(flash_data[0]!=0xff){
          //  uart_write1("AT+SEND=2,5,HELLO\r\n", 19);
            delay_ms(200);
        uart_write1("AT+SEND=2,", 10);
        uart_write1(sent_data_length_array, strlen(sent_data_length_array));
        uart_write_byte1(',');
        //uart_write1("DOTU", 4);
        uart_write1(flash_data, sent_data_length);
        uart_write_byte1('\r');
        uart_write_byte1('\n');
        if(uart_data_available1()){
            uart_read1(lora_data,26);
            flash_read_addr+=data_length;
            }
        //flash_read_addr+=data_length;
        }
        if(flash_read_addr==(FLASH_SECTOR_7_SIZE+FLASH_SECTOR_7_ADDRESS)){ //end of sector 7
                    flash_read_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
                    }
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
                     uart_write_byte2('\n');
                     format_raw_gps_data(raw_gps_data,(char*)gps_data,sizeof(gps_data));
                     uart_write2(gps_data, strlen((char*)gps_data));
                     uart_write_byte2('\n');
                     size_t len = strlen(gps_data);
                        for(size_t i = len; i < 32; i++){
                            gps_data[i] = '\0';  // Padding for gps_data
                            }
                            gps_data[64] = '\0';
                    //uint32_t a = strlen((char*)gps_data)/10;
                    //uint32_t b = strlen((char*)gps_data)%10;
                     uart_write_byte2('\n');
                    //uart_write2(gps_data, strlen((char*)gps_data));
                    //uart_write_byte2('\n');
                    //uart_write_byte2(a+'0');
                    //uart_write_byte2(b+'0');
                    //uart_write2((char*)gps_data, 64);  
                    //uart_write_byte2('\n');
                   // flash_write_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
                    fw_flash_write((uint32_t)flash_write_addr,gps_data,strlen(gps_data));
                    
                    fw_flash_read((uint32_t)flash_write_addr,buffer,128);
                    //uart_write2(buffer, 128);
                    //uart_write_byte2('\n');
                    flash_write_addr+=data_length;
                    //if sectors are full
                    if(flash_write_addr==(FLASH_SECTOR_6_SIZE+FLASH_SECTOR_6_SIZE)){  //end of sector 6
                    fw_flash_erase_sector(7);
                    if(flash_read_addr<(const uint8_t *)FLASH_SECTOR_7_ADDRESS)
                    flash_read_addr=(const uint8_t *)FLASH_SECTOR_7_ADDRESS;
                    }
                    else if(flash_write_addr==(FLASH_SECTOR_7_SIZE+FLASH_SECTOR_7_ADDRESS)){ //end of sector 7
                    fw_flash_erase_sector(6);
                    flash_write_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
                    if(flash_read_addr>=(const uint8_t *)FLASH_SECTOR_7_ADDRESS)
                    flash_read_addr=(const uint8_t *)FLASH_SECTOR_6_ADDRESS;
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