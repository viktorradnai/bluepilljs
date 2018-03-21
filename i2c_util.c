#include "hal.h"
#include "ch.h"

#include "chprintf.h"
#include "i2c_util.h"

extern SerialUSBDriver SDU1;

bool_t i2c_write(I2CDriver *device, uint8_t addr, uint8_t *tx, uint8_t txsize, uint8_t *rx, uint8_t rxsize)
{
    msg_t res;
    i2cAcquireBus(device);
    res = i2cMasterTransmitTimeout(device, addr, tx, txsize, rx, rxsize, 100);
    i2cReleaseBus(device);

    switch (res) {
        case MSG_OK:
            return TRUE;
        case MSG_RESET:
            chprintf((BaseSequentialStream *)&SDU1, "I2C Reset, addr: %02x\r\n", addr);
            return FALSE;
        case MSG_TIMEOUT:
            chprintf((BaseSequentialStream *)&SDU1, "I2C timeout, addr: %02x\r\n", addr);
            return FALSE;
        default:
            chprintf((BaseSequentialStream *)&SDU1, "I2C unknown result: %x, addr: %02x\r\n", res, addr);
            return FALSE;
    }
}
