#include "ch.h"
#include "hal.h"
#include <math.h>

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

