
#include<libopencm3/stm32/flash.h>
#include"core/fw_flash.h"


    #define FLASH_SECTOR_6_ADDRESS   (0x08040000)
    #define FALSH_SECTOR_7_ADDRESS   (0x08060000)

    void fw_flash_erase_sector(uint8_t sector){
        flash_unlock();
        flash_erase_sector(sector, FLASH_CR_PROGRAM_X8);
        flash_lock();

    }
    void fw_flash_write(const uint32_t address, const uint8_t* data,const uint32_t length){
        flash_unlock();
        flash_program(address,data,length);
        flash_lock();

    }

    void fw_flash_read(const uint32_t address, uint8_t* buffer,const uint32_t length) {
        const uint8_t *flash_ptr = (const uint8_t *)(address);
        for (uint32_t i = 0; i < length; i++) {
            buffer[i] = flash_ptr[i];
        }
    }