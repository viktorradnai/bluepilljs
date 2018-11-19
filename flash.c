#include "ch.h"
#include "hal.h"
#include "flash.h"
#include "printf.h"

bool flash_read(char *buf, uint8_t len)
{
    flash_error_t err;
    flash_offset_t addr = CALDATA_ADDR;

    err = flashRead(&FLASH_DEV, addr, len, (uint8_t *) buf);
    if(err != FLASH_NO_ERROR) {
        printf("Error reading flash: %d\r\n", err);
        return false;
    }
    return true;
}


bool flash_write(char *buf, uint8_t len)
{
    flash_error_t err;
    flash_offset_t addr = CALDATA_ADDR;

    err = flashStartEraseSector(&FLASH_DEV, CALDATA_SECTOR);
    if(err != FLASH_NO_ERROR) {
        printf("Error erasing flash: %d\r\n", err);
        return false;
    }
    err = flashWaitErase((BaseFlash *)&FLASH_DEV);
    if(err != FLASH_NO_ERROR) {
        printf("Error waiting for flash erase: %d\r\n", err);
        return false;
    }
    err = flashProgram(&FLASH_DEV, addr, len, (uint8_t *) buf);
    if(err != FLASH_NO_ERROR) {
        printf("Error writing flash: %d\r\n", err);
        return false;
    }
    return true;
}
