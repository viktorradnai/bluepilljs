#include "hal.h"
#include "ch.h"
#include "printf.h"
#include "flash.h"
#include "joystick.h"
#include "usb_hid.h"

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

void cmd_restart(BaseSequentialStream *chp, int argc, char *argv[])
{
    chThdCreateFromHeap(NULL, 512, "lsm303c_thread", NORMALPRIO + 2, lsm303c_thread, &I2CD1);
}

void cmd_calibrate(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: cal\r\n");
        return;
    }

    jscal_switch = 1;
    chprintf(chp, "Move joystick to both end points then center it and press any key\r\n");

    while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
        chThdSleepMilliseconds(200);
    }
    jscal_switch = 0;

    chprintf(chp, "\r\nCalibration complete\r\n");
    chThdSleepMilliseconds(200); // Wait for calibration data to be written by JS thread
    chprintf(chp, "M-: %f, M+: %f, offset: %f\r\n", cal_data->m_neg, cal_data->m_pos, cal_data->offset);
}

void cmd_calprint(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: calwrite\r\n");
        return;
    }

    chprintf(chp, "M-: %f, M+: %f, offset: %f\r\n", cal_data->m_neg, cal_data->m_pos, cal_data->offset);
}

void cmd_cal_save(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: cal_save\r\n");
        return;
    }

    if(flash_write(cal_frame.stream, sizeof(cal_frame))) chprintf(chp, "Flash written\r\n");
}

void cmd_cal_load(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;

    if (argc > 0) {
        chprintf(chp, "Usage: cal_load\r\n");
        return;
    }

    cal_load();
}

void cmd_calread(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    char buffer[128];
    uint8_t i;

    if (argc > 0) {
        chprintf(chp, "Usage: calread\r\n");
        return;
    }

    if (!flash_read(buffer, sizeof(buffer))) return;

    for(i=0; i<sizeof(buffer); i++) {
        chprintf(chp, "%c", buffer[i]);
    }
    chprintf(chp, "\r\n");
}

void cmd_flashwrite(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    char pattern[] = "Hello Flash!";

    if (argc > 0) {
        chprintf(chp, "Usage: flashwrite\r\n");
        return;
    }

    if(flash_write(pattern, sizeof(pattern))) chprintf(chp, "Flash written\r\n");
}

void cmd_flashread(BaseSequentialStream *chp, int argc, char *argv[]) {
    (void)argv;
    char buffer[128];
    uint8_t i;

    if (argc > 0) {
        chprintf(chp, "Usage: flashread\r\n");
        return;
    }

    if (!flash_read(buffer, sizeof(buffer))) return;

    for(i=0; i<sizeof(buffer); i++) {
        chprintf(chp, "%c", buffer[i]);
    }
    chprintf(chp, "\r\n");
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
    flash_offset_t flash_end = flashGetSectorOffset((BaseFlash *) &FD1, fd->sectors_count);
    chprintf(chp, "Cal data sector: %x, cal data address: %x\r\n", last_sector / fd->sectors_size, last_sector);
    chprintf(chp, "Sector limit: %x, address limit: %x\r\n", flash_end / fd->sectors_size, flash_end);
}
