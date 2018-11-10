#include "ch.h"
#include "hal.h"
#include <math.h>

#include "printf.h"
#include "usb_hid.h"
#include "ems22a.h"
#include "mlx90393.h"
#include "lsm303.h"
#include "joystick.h"

typedef enum {
    JSCAL_OFF = 0,
    // JSCAL_START,
    JSCAL_ON,
    JSCAL_END
} js_state_t;

typedef struct {
    int8_t min;
    int8_t max;
    int8_t offset;
} cal_state;

bool_t jscal_switch = 0;
cal_data_t cal_data = { 0.0, 1, 1 };

float normalise(float val, float offset)
{
    float res = val + offset;
    if (res < -128.0) res += 256;
    if (res > 128.0) res -= 256;

    return res;
}

inline float prevent_wraparound(float v, cal_state *c)
{
    if(v < -96) { // adjust offset up
        c->offset += 64;
        c->min += 64;
        c->max += 64;
        v += 64;
    } else if(v > 96) { // adjust offset down
        c->offset -= 64;
        c->min -= 64;
        c->max -= 64;
        v -= 64;
    }
    return v;
}

inline static void calibrate(float val) {
    static js_state_t state = JSCAL_OFF;
    static cal_state cs = { 0, 0, 0 };
    // bool_t jscal_switch = palReadPad(GPIOB, 11);
    val = prevent_wraparound(normalise(val, cs.offset), &cs);

    switch(state) {
        case JSCAL_OFF:
            if (!jscal_switch) return;
            palClearPad(GPIOC, GPIOC_LED);
            cs.min = cs.max = val;
            cs.offset = 0;
            state = JSCAL_ON;
            break;
        case JSCAL_ON:
            if (!jscal_switch) state = JSCAL_END;
            if (cs.min > val) cs.min = val;
            if (cs.max < val) cs.max = val;
            break;
        case JSCAL_END:
            palSetPad(GPIOC, GPIOC_LED);
            cal_data.offset = cs.offset - val;
            cal_data.m_neg = -128 / (cs.min - val);
            cal_data.m_pos = 128 / (cs.max - val);
            cs.min = cs.max = cs.offset = 0;
            state = JSCAL_OFF;
            break;
    }
}

inline static uint8_t read_buttons(void)
{
    uint16_t btns = palReadPort(GPIOA);
    return (uint8_t)((~btns) & 0xFF);
}

float xy_to_hdg(float x, float y)
{
    float hdg = (atan2f(y, x) / M_PI);
    if (hdg < -1) hdg += 2;
    if (hdg > 1) hdg -= 2;
    hdg *= -128;

    return hdg;
}

inline static float apply_cal(float val)
{
    val = normalise(val, cal_data.offset);
    if(val < 0) val *= cal_data.m_neg;
    else val *= cal_data.m_pos;
    if(val < -128) val = -128;
    else if(val > 127) val = 127;
    return val;
}

void transmit(float hdg)
{
    calibrate(hdg);
    hdg = apply_cal(hdg);
    hid_in_data.x = (int8_t) hdg;
    hid_in_data.button = read_buttons();
    hid_transmit(&USBD1);
}

THD_FUNCTION(lsm303c_thread, arg)
{
    I2CDriver *device = (I2CDriver *) arg;
    int16_t data[3];
    int16_t buf[3] = { 0, 0, 0 };

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

        // printf(">L303C %05d %05d %05d\r\n", buf[0], buf[1], buf[2]);
        transmit(xy_to_hdg(buf[0], buf[1]));
    }
}

THD_FUNCTION(lsm303dlhc_thread, arg)
{
    I2CDriver *device = (I2CDriver *) arg;
    int16_t data[3];
    int16_t buf[3] = { 0, 0, 0 };

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

        // printf(">L303Dx %05d %05d %05d\r\n", buf[0], buf[1], buf[2]);
        transmit(xy_to_hdg(buf[0], buf[1]));
    }
}

THD_FUNCTION(mlx90393_thread, arg)
{
    (void)arg;
    int16_t data[3];

    chRegSetThreadName("MLX90393 thread");
    if(!mlx90393_init(&I2CD1)) {
        printf("MLX90393 not found\r\n");
        chRegSetThreadName("MLX90393 fail");
        chThdExit(0);
    }

    while(1) {
        mlx90393_read(&I2CD1, data);
        // printf("ML393 %05d %05d %05d\r\n", data[0], data[1], data[2]);
        transmit(xy_to_hdg(data[0], data[1]));
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
        transmit(hdg);
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
        float hdg;

        for (uint8_t i = 0; i < EMS22A_CHAIN_LEN; i++) {
           if (rxbuf[i].data.cordic_oflow | rxbuf[i].data.linearity_alarm | rxbuf[i].data.mag_increase | rxbuf[i].data.mag_decrease | rxbuf[i].data.parity) {
                printf("EMS22A error, device %d, value: %04d, error bits: end_offst_comp %d, cordic_oflow %d, linearity_alarm %d, mag_increase %d, mag_decrease %d, parity %d\r\n",
                    i, rxbuf[i].data.value,
                    rxbuf[i].data.end_offst_comp, rxbuf[i].data.cordic_oflow, rxbuf[i].data.linearity_alarm,
                    rxbuf[i].data.mag_increase, rxbuf[i].data.mag_decrease, rxbuf[i].data.parity);
            } else {

                printf(">E22A%d %04d\r\n", i, rxbuf[i].data.value);
                hdg = ((float) rxbuf[i].data.value) / 4 - 128;
                transmit(hdg);
            }
        }
        chThdSleepMilliseconds(10);
    }
}
