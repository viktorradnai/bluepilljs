#include "hal.h"
#include "ch.h"
#include "printf.h"
#include "hal_flash_lld.h"
#include "joystick.h"
#include "usb_hid.h"

#define FLASH_END 0x20000
#define CALDATA_SECTOR (flashGetDescriptor((BaseFlash *) &FD1)->sectors_count - 1)
#define CALDATA_ADDR   (flashGetSectorOffset((BaseFlash *) &FD1, (flash_sector_t) CALDATA_SECTOR));

extern FlashDriver FD1;

void cmd_reset(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: reset\r\n");
        return;
    }

    chprintf(chp, "Will reset in 200ms\r\n");
    chThdSleepMilliseconds(200);
    NVIC_SystemReset();
}

void cmd_calibrate(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: cal\r\n");
    return;
    }

    jscal_switch = 1;
    while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
        chprintf(chp, "%d\r\n", hid_in_data.x);
        chThdSleepMilliseconds(200);
    }
    jscal_switch = 0;

    chprintf(chp, "\r\nDone\r\n");
    chThdSleepMilliseconds(200);
    chprintf(chp, "M-: %f, M+: %f, offset: %f\r\n", cal_data.m_neg, cal_data.m_pos, cal_data.offset);
}

void cmd_flashwrite(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    flash_error_t err;
    flash_offset_t addr = CALDATA_ADDR;
    char pattern[] = "Hello Flash!";

    if (argc > 0) {
        chprintf(chp, "Usage: flashread\r\n");
        return;
    }

    chprintf(chp, "Will write in 1s\r\n");
    chThdSleepMilliseconds(1000);
    err = flashStartEraseSector(&FD1, CALDATA_SECTOR);
    if(err != FLASH_NO_ERROR) {
        chprintf(chp, "Error erasing flash: %d\r\n", err);
        return;
    }
    err = flashWaitErase((BaseFlash *)&FD1);
    if(err != FLASH_NO_ERROR) {
        chprintf(chp, "Error waiting for flash erase: %d\r\n", err);
        return;
    }
    err = flashProgram(&FD1, addr, sizeof(pattern), (uint8_t *) pattern);
    if(err != FLASH_NO_ERROR) {
        chprintf(chp, "Error writing flash: %d\r\n", err);
        return;
    }
    chprintf(chp, "Flash written\r\n");
}

void cmd_flashread(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    flash_error_t err;
    flash_offset_t addr = CALDATA_ADDR;
    uint8_t buffer[128];
    uint8_t i;

    if (argc > 0) {
        chprintf(chp, "Usage: flashread\r\n");
        return;
    }

    chprintf(chp, "Will read in 1s\r\n");
    chThdSleepMilliseconds(1000);
    err = flashRead(&FD1, addr, 128, buffer);
    if(err != FLASH_NO_ERROR) {
        chprintf(chp, "Error reading flash: %d\r\n", err);
    }

    chprintf(chp, "Addr: %x, data: ", addr, buffer);
    for(i=0; i<128; i++) {
        chprintf(chp, "%c", buffer[i]);
    }
    chprintf(chp, "\r\n");
    chprintf(chp, "%s\r\n", (char *) addr);
}

void cmd_flashinfo(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    const flash_descriptor_t *fd = flashGetDescriptor(&FD1);

    if (argc > 0) {
        chprintf(chp, "Usage: flashinfo\r\n");
        return;
    }

    chprintf(chp, "Flash address: %x, sector count: %d, sector size: %x\r\n", fd->address, fd->sectors_count, fd->sectors_size);
    chprintf(chp, "Flash offset: %x, sector size: %x\r\n", flashGetSectorOffset((BaseFlash *) &FD1, 0), flashGetSectorSize((BaseFlash *) &FD1, 0));
    flash_offset_t last_sector = flashGetSectorOffset((BaseFlash *) &FD1, fd->sectors_count - 1);
    chprintf(chp, "Cal data sector: %x, cal data address: %x\r\n", last_sector / fd->sectors_size, last_sector);
}

void cmd_flashdump(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    flash_error_t err = FLASH_NO_ERROR;
    flash_offset_t addr = 0;
    uint8_t buffer[2048];
    uint8_t i;

    if (argc > 0) {
        chprintf(chp, "Usage: flashread\r\n");
        return;
    }

    chprintf(chp, "Will read in 1s\r\n");
    chThdSleepMilliseconds(1000);
    while(err == FLASH_NO_ERROR && addr < FLASH_END) {
        err = flashRead(&FD1, addr, 128, buffer);
        chprintf(chp, "Addr: %x, data: ", addr, buffer);
        for(i=0; i<128; i++) {
            chprintf(chp, "%c", buffer[i]);
        }
        chprintf(chp, "\r\n");
        chThdSleepMilliseconds(100);
        addr += 128;
    }
}
