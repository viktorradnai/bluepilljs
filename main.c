#include "hal.h"
#include "ch.h"
#include "usb_hid.h"
#include "usbcfg.h"
#include <math.h>

#include "test.h"
#include "chprintf.h"
#include "shell.h"

#define MAG_DEV_ADDR 0x1E
// #define MAG_DEV_LSM303C 1
#define MAG_DEV_LSM303HDLC 2


/* Virtual serial port over USB.*/
SerialUSBDriver SDU1;

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

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreGetStatusX());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%08lx %08lx %4lu %4lu %9s\r\n",
             (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
             (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
             states[tp->p_state]);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  thread_t *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: test\r\n");
    return;
  }
  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriorityX(),
                           TestThread, chp);
  if (tp == NULL) {
    chprintf(chp, "out of memory\r\n");
    return;
  }
  chThdWait(tp);
}

static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"test", cmd_test},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};



static void writeByteI2C(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t cmd[] = {reg, val};
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, cmd, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
}
#if MAG_DEV_LSM303C
#define MAG_DEV_START_REG 0x28
static void magInit(void)
{
    // Highest speed
    writeByteI2C(MAG_DEV_ADDR, 0x20, 0x7C);
    // Lowest gain
    writeByteI2C(MAG_DEV_ADDR, 0x21, 0x60);
    // Continuous conversion mode
    writeByteI2C(MAG_DEV_ADDR, 0x22, 0x00);

    writeByteI2C(MAG_DEV_ADDR, 0x23, 0x0E);
}
#elif MAG_DEV_LSM303HDLC
#define MAG_DEV_START_REG 0x03
static void magInit(void)
{
    // Highest speed
    writeByteI2C(MAG_DEV_ADDR, 0x00, 0x1C);
    // Lowest gain
    writeByteI2C(MAG_DEV_ADDR, 0x01, 0xE0);
    // Continuous conversion mode
    writeByteI2C(MAG_DEV_ADDR, 0x02, 0x00);
}
#endif // MAG_DEV

static uint8_t magRead(float* data)
{
    uint8_t start_reg = MAG_DEV_START_REG;
    uint8_t out[7];
    i2cAcquireBus(&I2CD1);
    msg_t f = i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, &start_reg, 1, out, 7, TIME_INFINITE);
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
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

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
    thread_t *shelltp = NULL;

    halInit();
    chSysInit();

    /* Shell manager initialization */
    shellInit();

    i2cStart(&I2CD1, &i2cconfig);
    magInit();

    palSetPadMode(GPIOC, 13, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOC, 13);
    usb_init();

    float magData[3], magBuf[3];
    float hdg = 0, x, y;
    magBuf[0] = magBuf[1] = magBuf[2] = 0;


    while(1) {
        uint16_t btns;

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
