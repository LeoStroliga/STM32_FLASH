#ifndef INC_FW_FLASH_H
#define INC_FW_FLASH_H

#include "common-defines.h"

void fw_flash_erase_sector(const uint8_t sector);
void fw_flash_write(const uint32_t address, const uint8_t* data,const uint32_t length);
void fw_flash_read(const uint32_t address, uint8_t* buffer,const uint32_t length);


#endif

