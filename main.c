#include "hal.h"
#include "ch.h"
#include "chprintf.h"
#include "shell.h"

#include "usb_hid.h"
#include "usbcfg.h"
#include "cmd.h"
#include "ems22a.h"
#include "mlx90393.h"
#include "lsm303.h"

#include <math.h>

#define printf(args...) chprintf((BaseSequentialStream *)&SDU1, args);

/* Virtual serial port over USB */
SerialUSBDriver SDU1;

/* I2C1 config */
static const I2CConfig i2cconfig = {
    OPMODE_I2C,
    40000,
    STD_DUTY_CYCLE,
};

static const SPIConfig spicfg = {
    NULL,
    GPIOA,
    GPIOA_SPI1NSS,
    SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0 | SPI_CR1_CPOL | SPI_CR1_DFF,
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

    while (usbInitState == 0) {
        palTogglePad(GPIOC, GPIOC_LED);
        chThdSleepMilliseconds(50);
    }
    palSetPad(GPIOC, GPIOC_LED);
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

static float xy_to_hdg(float x, float y)
{
    float hdg = (atan2f(y, x) / M_PI);
    if (hdg < -1) hdg += 2;
    if (hdg > 1) hdg -= 2;
    hdg *= 128;

    //calibrate(hdg);
    //hdg += cal_data.offset;
    //if (hdg > 0) hdg *= cal_data.m2;
    //else hdg *= cal_data.m1;
    if (hdg > 128) hdg = 128;
    else if (hdg < -127) hdg = -127;
    return hdg;
}

static uint8_t read_buttons(void)
{
    uint16_t btns = palReadPort(GPIOA);
    return (uint8_t)((~btns) & 0xFF);
}

static THD_WORKING_AREA(mlx90393_thread_wa, 512);
static THD_FUNCTION(mlx90393_thread, arg)
{
    (void)arg;
    int16_t data[3];
    float buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("MLX90393 thread");

    if(!mlx90393_init()) {
        printf("MLX90393 not found\r\n");
        chThdExit(0);
    }

    while(1) {
        mlx90393_read(data);
        printf("MLX90393 XYZ: %d %d %d\r\n", data[0], data[1], data[2]);

        continue;
        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        hdg = xy_to_hdg(buf[0], buf[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

static THD_WORKING_AREA(lsm303c_thread_wa, 512);
static THD_FUNCTION(lsm303c_thread, arg)
{
    (void)arg;
    int16_t data[3];
    float buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("LSM303C thread");

    if(!lsm303c_init()) {
        printf("LSM303C not found\r\n");
        chThdExit(0);
    }
    while(1) {
        lsm303c_read(data);
        printf("LSM303C XYZ: %d %d %d\r\n", data[0], data[1], data[2]);

        continue;
        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        hdg = xy_to_hdg(buf[0], buf[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

static THD_WORKING_AREA(lsm303dlhc_thread_wa, 512);
static THD_FUNCTION(lsm303dlhc_thread, arg)
{
    (void)arg;
    int16_t data[3];
    float buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("LSM303DLHC thread");

    if(!lsm303dlhc_init()) {
        printf("LSM303DLHC not found\r\n");
        chThdExit(0);
    }
    while(1) {
        lsm303dlhc_read(data);
        printf("LSM303DLHC XYZ: %d %d %d\r\n", data[0], data[1], data[2]);

        continue;
        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        hdg = xy_to_hdg(buf[0], buf[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

static THD_WORKING_AREA(ems22a_thread_wa, 256);
static THD_FUNCTION(ems22a_thread, p) {
    /*
     * The EMS22A prepends a dummy bit before the start of each 16 bit frame which makes each frame 17 bits each.
     * We must allocate enough extra space the extra bits. One extra 16 bit word gives us space for 16 * 17 bits = 16 daisy chained devices.
     */
    static ems22a_frame rxbuf[EMS22A_CHAIN_LEN+1]; //
    (void)p;

    chRegSetThreadName("EMS22A thread");
    while (1) {
        ems22a_receive(rxbuf, EMS22A_CHAIN_LEN);

        for (uint8_t i = 0; i < EMS22A_CHAIN_LEN; i++) {
           if (rxbuf[i].data.cordic_oflow | rxbuf[i].data.linearity_alarm | rxbuf[i].data.mag_increase | rxbuf[i].data.mag_decrease | rxbuf[i].data.parity) {
                printf("EMS22A error, device %d, value: %04d, error bits: end_offst_comp %d, cordic_oflow %d, linearity_alarm %d, mag_increase %d, mag_decrease %d, parity %d\r\n", i, rxbuf[i].data.value,
                    rxbuf[i].data.end_offst_comp, rxbuf[i].data.cordic_oflow, rxbuf[i].data.linearity_alarm,
                    rxbuf[i].data.mag_increase, rxbuf[i].data.mag_decrease, rxbuf[i].data.parity);
            } else {

                printf("EMS22A device %d, value: %04d\r\n", i, rxbuf[i].data.value);
            }
        }
        chThdSleepMilliseconds(10);
    }
}

int main(void) {
    thread_t *shelltp = NULL;

    halInit();
    chSysInit();
    shellInit();

    i2cStart(&I2CD1, &i2cconfig);
    spiStart(&SPID1, &spicfg);
    usb_init();

    chprintf((BaseSequentialStream *)&SDU1, "Sleeping for 5 seconds");
    chThdSleepMilliseconds(5000);

    chThdCreateStatic(mlx90393_thread_wa, sizeof(mlx90393_thread_wa), NORMALPRIO + 4, mlx90393_thread, NULL);
    chThdCreateStatic(lsm303c_thread_wa, sizeof(lsm303c_thread_wa), NORMALPRIO + 3, lsm303c_thread, NULL);
    chThdCreateStatic(lsm303dlhc_thread_wa, sizeof(lsm303dlhc_thread_wa), NORMALPRIO + 2, lsm303dlhc_thread, NULL);
    chThdCreateStatic(ems22a_thread_wa, sizeof(ems22a_thread_wa), NORMALPRIO + 1, ems22a_thread, NULL);


    while(1) {
        // chprintf((BaseSequentialStream *)&SDU1, "Idle");

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
