#include "ch.h"
#include "hal.h"
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

const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

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

int main(void) {
    thread_t *shelltp = NULL;

    halInit();
    chSysInit();
    shellInit();

    i2cStart(&I2CD1, &i2cconfig);
    i2cStart(&I2CD2, &i2cconfig);
    spiStart(&SPID1, &spicfg);
    usb_init();
    cal_load();

    chThdCreateFromHeap(NULL, 512, "lsm303c_thread", NORMALPRIO + 2, lsm303c_thread, &I2CD1);
    // chThdCreateFromHeap(NULL, 512, "lsm303dlhc_thread", NORMALPRIO + 2, lsm303dlhc_thread, &I2CD1);
    // chThdCreateFromHeap(NULL, 512, "mlx90393_thread", NORMALPRIO + 2, mlx90393_thread, &I2CD1);
    // chThdCreateFromHeap(NULL, 512, "ems22a_thread", NORMALPRIO + 1, ems22a_thread, NULL);
    // chThdCreateStatic(NULL, 512, "dummy_thread", NORMALPRIO + 1, dummy_thread, NULL);

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
