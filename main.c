#include "hal.h"
#include "ch.h"

#include "usb_hid.h"
#include "usbcfg.h"
#include "cmd.h"
#include "mag.h"
#include "ems22a.h"
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

static THD_WORKING_AREA(mag_thread_wa, 512);
static THD_FUNCTION(mag_thread, arg) {
    (void)arg;
    uint16_t btns;
    float magData[3], magBuf[3];
    float hdg = 0, x, y;
    magBuf[0] = magBuf[1] = magBuf[2] = 0;

    chRegSetThreadName("Magnetometer thread");

    chThdSleepMilliseconds(5000);
    magInit();
    while(1) {
        magRead(magData);
        continue;
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
        //chprintf((BaseSequentialStream *)&SDU1, "Hdg: %d\r\n", hid_in_data.x);
    }
}

static THD_WORKING_AREA(spi_thread_wa, 256);
static THD_FUNCTION(spi_thread, p) {
    /*
     * The EMS22A prepends a dummy bit before the start of each 16 bit frame which makes each frame 17 bits each.
     * We must allocate enough extra space the extra bits. One extra 16 bit word gives us space for 16 * 17 bits = 16 daisy chained devices.
     */
    static frame rxbuf[EMS22A_CHAIN_LEN+1]; //
    (void)p;

    chRegSetThreadName("SPI thread");
    while (1) {
        spiSelect(&SPID1);
        spiReceive(&SPID1, EMS22A_CHAIN_LEN+1, (uint16_t *)rxbuf);
        spiUnselect(&SPID1);

        /* 17 bit to 16 bit data conversion */
        uint16_t tmp;
        for (uint8_t i = 0; i < EMS22A_CHAIN_LEN+1; i++) {
            tmp = rxbuf[i].word >> (16-i); // Only keep the bits which will get moved to the previous frame
            rxbuf[i].word <<= (i+1);
            if (i > 0) {
                rxbuf[i-1].word += tmp;
                rxbuf[i-1].data.parity = ems22a_check_parity(&rxbuf[i-1]);
            }
        }

        for (uint8_t i = 0; i < EMS22A_CHAIN_LEN; i++) {
           if (rxbuf[i].word & 0x2f) {
                chprintf((BaseSequentialStream *)&SDU1, "EMS22A error, device %d, value: %04d, error bits: %d %d %d %d %d %d \r\n", i, rxbuf[i].data.value,
                    rxbuf[i].data.end_offst_comp, rxbuf[i].data.cordic_oflow, rxbuf[i].data.linearity_alarm,
                    rxbuf[i].data.mag_increase, rxbuf[i].data.mag_decrease, rxbuf[i].data.parity);
            }

        }

        chThdSleepMilliseconds(5);
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

    chThdCreateStatic(mag_thread_wa, sizeof(mag_thread_wa), NORMALPRIO + 2, mag_thread, NULL);
    chThdCreateStatic(spi_thread_wa, sizeof(spi_thread_wa), NORMALPRIO + 3, spi_thread, NULL);


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
