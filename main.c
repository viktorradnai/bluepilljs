#include "hal.h"
#include "ch.h"

#include "usb_hid.h"
#include "usbcfg.h"
#include "cmd.h"
#include "mag.h"
#include <math.h>

#include "chprintf.h"
#include "shell.h"

/* Virtual serial port over USB */
SerialUSBDriver SDU1;

/* I2C1 config */
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


#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

static void usb_init(void) {
    usbInitState = 0;
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    usbDisconnectBus(&USBD1);
    chThdSleepMilliseconds(1000);
    usbStart(&USBD1, &usbcfg);
    usbConnectBus(&USBD1);

    while (usbInitState == 0)
        chThdSleepMilliseconds(50);
}

#if 0
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
#endif

static THD_WORKING_AREA(waThread1, 512);
static THD_FUNCTION(Thread1, arg) {
    (void)arg;
    uint16_t btns;
    float magData[3], magBuf[3];
    float hdg = 0, x, y;
    magBuf[0] = magBuf[1] = magBuf[2] = 0;

    while(1) {
        palTogglePad(GPIOC, GPIOC_LED);
        magRead(magData);
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


int main(void) {
    thread_t *shelltp = NULL;

    halInit();
    chSysInit();
    shellInit();

    palSetPadMode(GPIOC, GPIOC_LED, PAL_MODE_OUTPUT_PUSHPULL);

    i2cStart(&I2CD1, &i2cconfig);
    usb_init();
    magInit();

    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO + 10, Thread1, NULL);

    while(1) {
        if (!shelltp) {
            if (SDU1.config->usbp->state == USB_ACTIVE) {
                /* Spawns a new shell.*/
                shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
            }
        } else {
            /* If the previous shell exited.*/
            if (chThdTerminatedX(shelltp)) {
                /* Recovers memory of the previous shell.*/
                chThdRelease(shelltp);
                shelltp = NULL;
            }
        }
        chThdSleepMilliseconds(5);
    }
    return 0;
}
