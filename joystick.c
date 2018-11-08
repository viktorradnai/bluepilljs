#include "ch.h"
#include "hal.h"
#include <math.h>

#include "printf.h"
#include "usb_hid.h"
#include "ems22a.h"
#include "mlx90393.h"
#include "lsm303.h"


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

float xy_to_hdg(float x, float y)
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

uint8_t read_buttons(void)
{
    uint16_t btns = palReadPort(GPIOA);
    return (uint8_t)((~btns) & 0xFF);
}

THD_FUNCTION(lsm303c_thread, arg)
{
    I2CDriver *device = (I2CDriver *) arg;
    int16_t data[3];
    int16_t buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("LSM303C thread");
    if(!lsm303c_init(device)) {
        printf("LSM303C not found\r\n");
        chRegSetThreadName("LSM303C fail");
        chThdExit(0);
    }
    while(1) {
        lsm303c_read(device, data);

        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        printf(">L303C %05d %05d %05d\r\n", buf[0], buf[1], buf[2]);
        continue;

        hdg = xy_to_hdg(buf[0], buf[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

THD_FUNCTION(lsm303dlhc_thread, arg)
{
    I2CDriver *device = (I2CDriver *) arg;
    int16_t data[3];
    int16_t buf[3] = { 0, 0, 0 };
    float hdg = 0;
    int16_t ctr = 0;

    chRegSetThreadName("LSM303DLHC thread");

    if(!lsm303dlhc_init(device)) {
        printf("LSM303DLHC not found\r\n");
        chRegSetThreadName("LSM303DLHC fail");
        chThdExit(0);
    }
    while(1) {
        lsm303dlhc_read(device, data);

        buf[0] = (buf[0] * 7 + data[0]) / 8;
        buf[1] = (buf[1] * 7 + data[1]) / 8;

        if(!ctr) printf(">L303Dx %05d %05d %05d\r\n", data[0], data[1], data[2]);
        ctr++;
        // continue;

        hdg = xy_to_hdg(buf[0], buf[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

THD_FUNCTION(mlx90393_thread, arg)
{
    (void)arg;
    int16_t data[3];
    //int16_t buf[3] = { 0, 0, 0 };
    float hdg = 0;

    chRegSetThreadName("MLX90393 thread");

    if(!mlx90393_init(&I2CD1)) {
        printf("MLX90393 not found\r\n");
        chRegSetThreadName("MLX90393 fail");
        chThdExit(0);
    }

    while(1) {
        mlx90393_read(&I2CD1, data);

        //buf[0] = (buf[0] * 7 + data[0]) / 8;
        //buf[1] = (buf[1] * 7 + data[1]) / 8;

        // printf("ML393 %05d %05d %05d\r\n", data[0], data[1], data[2]);
        continue;

        hdg = xy_to_hdg(data[0], data[1]);
        hid_in_data.x = (int8_t) hdg;
        hid_in_data.button = read_buttons();
        hid_transmit(&USBD1);
    }
}

THD_FUNCTION(dummy_thread, arg)
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

THD_FUNCTION(ems22a_thread, arg)
{
    (void) arg;
    /*
     * The EMS22A prepends a dummy bit before the start of each 16 bit frame which makes each frame 17 bits each.
     * We must allocate enough extra space the extra bits. One extra 16 bit word gives us space for 16 * 17 bits = 16 daisy chained devices.
     */
    static ems22a_frame rxbuf[EMS22A_CHAIN_LEN+1];

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
