#include "hal.h"
#include "ch.h"
#include "usb_hid.h"
#include "usbcfg.h"
#include <math.h>

/*
 * I2C1 config.
 */
static const I2CConfig i2cconfig = {
    OPMODE_I2C,
    40000,
    STD_DUTY_CYCLE,
};

typedef struct {
    float offset;
    float min;
    float max;
    float m1;
    float m2;
} cal_data_t;

cal_data_t cal_data = { 0.0, -127.0, 128.0, 1, 1 };

typedef enum {
    JSCAL_OFF = 0,
    JSCAL_START,
    JSCAL_ON,
    JSCAL_END
} js_state_t;

static void writeByteI2C(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t cmd[] = {reg, val};
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, cmd, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
}

static void magInit(void)
{
    // Highest speed
    writeByteI2C(0x1E, 0x00, 0x1C);
    // Lowest gain
    writeByteI2C(0x1E, 0x01, 0xE0);
    // Continuous conversion mode
    writeByteI2C(0x1E, 0x02, 0x00);
}

static uint8_t magRead(float* data)
{
    uint8_t start_reg = 0x03;
    uint8_t out[7];
    i2cAcquireBus(&I2CD1);
    msg_t f = i2cMasterTransmitTimeout(&I2CD1, 0x1E, &start_reg, 1, out, 7, TIME_INFINITE);
    (void)f;
    i2cReleaseBus(&I2CD1);
    //out[6] doesn't seem to reflect actual new data, so just push out every time
    int16_t val_x = (out[0] << 8) | out[1];
    int16_t val_z = (out[2] << 8) | out[3];
    int16_t val_y = (out[4] << 8) | out[5];
    data[0] = ((float)val_x)*1.22;
    data[1] = ((float)val_y)*1.22;
    data[2] = ((float)val_z)*1.22;
    return 1;
}

static void usb_init(void) {
    usbInitState = 0;
    usbDisconnectBus(&USBD1);
    chThdSleepMilliseconds(1000);
    usbStart(&USBD1, &usbcfg);
    usbConnectBus(&USBD1);

    while (usbInitState == 0)
        chThdSleepMilliseconds(50);
}

static void calibrate(float hdg) {
    static uint8_t state = JSCAL_OFF;
    bool_t cal_switch = palReadPad(GPIOB, 11);

    switch(state) {
        case JSCAL_OFF:
            if (cal_switch) state = JSCAL_START;
            break;
        case JSCAL_START:
            palSetPad(GPIOC, GPIOC_LED);
            cal_data.offset = -hdg;
            cal_data.min = hdg;
            cal_data.max = hdg;
            state = JSCAL_ON;
            break;
        case JSCAL_ON:
            if (!cal_switch) state = JSCAL_END;
            if (cal_data.min > hdg) cal_data.min = hdg;
            if (cal_data.max < hdg) cal_data.max = hdg;

            cal_data.m1 = 128 / cal_data.min;
            cal_data.m2 = 128 / cal_data.max;
            break;
        case JSCAL_END:
            palClearPad(GPIOC, GPIOC_LED);
            state = JSCAL_OFF;
            break;
    }
}

int main(void) {
    halInit();
    chSysInit();
    i2cStart(&I2CD1, &i2cconfig);
    usb_init();
    magInit();

    float magData[3], magBuf[3];
    float hdg = 0, x, y;
    magBuf[0] = magBuf[1] = magBuf[2] = 0;

    while(1) {
        uint16_t btns;
        if (magRead(magData)) {
            x = magBuf[0] = (magBuf[0] * 7 + magData[0]) / 8;
            y = magBuf[1] = (magBuf[1] * 7 + magData[1]) / 8;
            hdg = (atan2f(y, x) / M_PI);
            if (hdg < -1) hdg += 2;
            if (hdg > 1) hdg -= 2;
            hdg *= 128;

            //calibrate(hdg);
            //hdg += cal_data.offset;
            //if (hdg > 0) hdg *= cal_data.m2;
            //else hdg *= cal_data.m1;
            if (hdg > 128) hdg = 128;
            else if (hdg < -127) hdg = -127;

            hid_in_data.x = (int8_t) hdg;
            btns = palReadPort(GPIOA);
            hid_in_data.button = (uint8_t)((~btns) & 0xFF);
            hid_transmit(&USBD1);
        }

    }
    return 0;
}
