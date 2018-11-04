#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "cmd.h"
#include "usb_hid.h"
#include "usbcfg.h"
#include "joystick.h"
#include "printf.h"

#include <math.h>

/* Virtual serial port over USB */
SerialUSBDriver SDU1;
extern const USBConfig usbcfg;
extern SerialUSBConfig serusbcfg;

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
    0
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
  {"cal", cmd_calibrate},
  {"flashwrite", cmd_flashwrite},
  {"flashread", cmd_flashread},
  {"flashdump", cmd_flashdump},
  {"flashinfo", cmd_flashinfo},
  {"reset", cmd_reset},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

void usb_init(void) {
    usbInitState = 0;
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    usbDisconnectBus(&USBD1);
    chThdSleepMilliseconds(1500);
    usbStart(&USBD1, &usbcfg);

    usbConnectBus(&USBD1);

    while (usbInitState == 0) {
        palTogglePad(GPIOC, GPIOC_LED);
        chThdSleepMilliseconds(50);
    }
    palSetPad(GPIOC, GPIOC_LED);
}

static THD_WORKING_AREA(mlx90393_thread_wa, 512);
static THD_FUNCTION(mlx90393_thread, arg)
{
    (void)arg;
    int16_t data[3];
    //int16_t buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("MLX90393 thread");

    if(!mlx90393_init(&I2CD1)) {
        printf("MLX90393 not found\r\n");
        chThdExit(0);
    }

    while(1) {
        mlx90393_read(&I2CD1, data);

        //buf[0] = (buf[0] * 7 + data[0]) / 8;
        //buf[1] = (buf[1] * 7 + data[1]) / 8;

        printf("ML393 %05d %05d %05d\r\n", data[0], data[1], data[2]);
        continue;

        hdg = xy_to_hdg(data[0], data[1]);
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
    int16_t buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("LSM303C thread");

    if(!lsm303c_init(&I2CD1)) {
        printf("LSM303C not found\r\n");
        chThdExit(0);
    }
    while(1) {
        lsm303c_read(&I2CD1, data);

        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        // printf(">L303C %05d %05d %05d\r\n", buf[0], buf[1], buf[2]);
        // continue;

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
    int16_t buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("LSM303DLHC thread");

    if(!lsm303dlhc_init(&I2CD1)) {
        printf("LSM303DLHC not found\r\n");
        chThdExit(0);
    }
    while(1) {
        lsm303dlhc_read(&I2CD1, data);

        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        printf(">L303D %05d %05d %05d\r\n", buf[0], buf[1], buf[2]);
        continue;

        hdg = xy_to_hdg(buf[0], buf[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

static THD_WORKING_AREA(dummy_thread_wa, 512);
static THD_FUNCTION(dummy_thread, arg)
{
    (void)arg;
    int8_t hdg = 0;
    int8_t add = 1;

    chRegSetThreadName("Dummy thread");

    while(1) {
        if (hdg == 127) add = -1;
        if (hdg == -128) add = 1;

        hdg += add;
        printf(">DUMMY %05d %05d\r\n", hdg, add);

        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);

        chThdSleepMilliseconds(10);
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
                printf("EMS22A error, device %d, value: %04d, error bits: end_offst_comp %d, cordic_oflow %d, linearity_alarm %d, mag_increase %d, mag_decrease %d, parity %d\r\n",
                    i, rxbuf[i].data.value,
                    rxbuf[i].data.end_offst_comp, rxbuf[i].data.cordic_oflow, rxbuf[i].data.linearity_alarm,
                    rxbuf[i].data.mag_increase, rxbuf[i].data.mag_decrease, rxbuf[i].data.parity);
            } else {

                printf(">E22A%d %04d\r\n", i, rxbuf[i].data.value);
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
    i2cStart(&I2CD2, &i2cconfig);
    spiStart(&SPID1, &spicfg);
    usb_init();

    //chThdCreateStatic(mlx90393_thread_wa, sizeof(mlx90393_thread_wa), NORMALPRIO + 4, mlx90393_thread, NULL);
    chThdCreateStatic(lsm303c_thread_wa, sizeof(lsm303c_thread_wa), NORMALPRIO + 3, lsm303c_thread, NULL);
    //chThdCreateStatic(lsm303dlhc_thread_wa, sizeof(lsm303dlhc_thread_wa), NORMALPRIO + 2, lsm303dlhc_thread, NULL);
    //chThdCreateStatic(dummy_thread_wa, sizeof(dummy_thread_wa), NORMALPRIO + 1, dummy_thread, NULL);
    //chThdCreateStatic(ems22a_thread_wa, sizeof(ems22a_thread_wa), NORMALPRIO + 1, ems22a_thread, NULL);

    while(1) {
        // chprintf((BaseSequentialStream *)&SDU1, "Idle");
        if (!shelltp) {
            if (SDU1.config->usbp->state == USB_ACTIVE) {
                /* Spawns a new shell.*/
                shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                            "shell", NORMALPRIO + 1,
                                            shellThread, (void *)&shell_cfg1);
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
