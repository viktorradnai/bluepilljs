#ifndef FLASH_H_INCLUDED
#define FLASH_H_INCLUDED

#include "hal_flash_lld.h"

#define FLASH_DEV FD1
#define FLASH_END flashGetSectorOffset((BaseFlash *) &FLASH_DEV, flashGetDescriptor(&FLASH_DEV)->sectors_count)
#define CALDATA_SECTOR (flashGetDescriptor((BaseFlash *) &FLASH_DEV)->sectors_count - 1)
#define CALDATA_ADDR   (flashGetSectorOffset((BaseFlash *) &FLASH_DEV, (flash_sector_t) CALDATA_SECTOR))

extern FlashDriver FLASH_DEV;

bool flash_read(char *buf, uint8_t len);
bool flash_write(char *buf, uint8_t len);

#endif // FLASH_H_INCLUDED
