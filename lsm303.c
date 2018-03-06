#include "hal.h"
#include "ch.h"
#include "chprintf.h"

#include "i2c_util.h"
#include "lsm303.h"

extern SerialUSBDriver SDU1;

bool_t lsm303c_init(void)
{
    uint8_t out1[] = { 0x20, 0x7C }; // Highest speed
    uint8_t out2[] = { 0x21, 0x60 }; // Lowest gain
    uint8_t out3[] = { 0x22, 0x00 }; // Continuous conversion mode
    uint8_t out4[] = { 0x23, 0x0E };

    if (!i2c_write(LSM303C_I2C_ADDR, out1, sizeof(out1), NULL, 0)) return FALSE;
    if (!i2c_write(LSM303C_I2C_ADDR, out2, sizeof(out2), NULL, 0)) return FALSE;
    if (!i2c_write(LSM303C_I2C_ADDR, out3, sizeof(out3), NULL, 0)) return FALSE;
    return i2c_write(LSM303C_I2C_ADDR, out4, sizeof(out4), NULL, 0);
}

bool_t lsm303dhlc_init(void)
{
    uint8_t out1[] = { 0x00, 0x1C }; // Highest speed
    uint8_t out2[] = { 0x01, 0xE0 }; // Lowest gain
    uint8_t out3[] = { 0x02, 0x00 }; // Continuous conversion mode

    if (!i2c_write(LSM303DHLC_I2C_ADDR, out1, sizeof(out1), NULL, 0)) return FALSE;
    if (!i2c_write(LSM303DHLC_I2C_ADDR, out2, sizeof(out2), NULL, 0)) return FALSE;
    return i2c_write(LSM303DHLC_I2C_ADDR, out3, sizeof(out3), NULL, 0);
}

bool_t lsm303_read(uint8_t addr, uint8_t out, float* data)
{
    uint8_t in[8];
    if (!i2c_write(addr, &out, sizeof(out), in, sizeof(in))) return FALSE;

    //out[6] doesn't seem to reflect actual new data, so just discard it
    int16_t val_x = (in[0] << 8) | in[1];
    int16_t val_z = (in[2] << 8) | in[3];
    int16_t val_y = (in[4] << 8) | in[5];

    chprintf((BaseSequentialStream *)&SDU1, "XYZ: %d %d %d\r\n", val_x,val_y, val_z);
    data[0] = ((float)val_x)*1.22;
    data[1] = ((float)val_y)*1.22;
    data[2] = ((float)val_z)*1.22;

    return TRUE;
}
