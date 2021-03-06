#include "hal.h"
#include "ch.h"
#include "printf.h"
#include "i2c_util.h"
#include "lsm303.h"

bool_t lsm303c_init(I2CDriver *device)
{
    uint8_t out1[] = { 0x20, 0x7C }; // Highest speed
    uint8_t out2[] = { 0x21, 0x60 }; // Lowest gain
    uint8_t out3[] = { 0x22, 0x00 }; // Continuous conversion mode
    uint8_t out4[] = { 0x23, 0x0E };

#if 0
    uint8_t ident_out = { 0x0F }; // WHO_AM_I register
    uint8_t ident_in[1];

    if (!i2c_write(device, LSM303C_I2C_ADDR, ident_out, 1, ident_in, 1)) return false;
    if (ident_in[0] != 0x3D) {
        printf("Device at address LSM303C_I2C_ADDR and ident %x does not appear to be an LSM303C\r\n", ident_in[0]);
        return false;
    }

#endif // 0
    if (!i2c_write(device, LSM303C_I2C_ADDR, out1, sizeof(out1), NULL, 0)) return false;
    if (!i2c_write(device, LSM303C_I2C_ADDR, out2, sizeof(out2), NULL, 0)) return false;
    if (!i2c_write(device, LSM303C_I2C_ADDR, out3, sizeof(out3), NULL, 0)) return false;
    return i2c_write(device, LSM303C_I2C_ADDR, out4, sizeof(out4), NULL, 0);
}

bool_t lsm303dlhc_init(I2CDriver *device)
{
    uint8_t out1[] = { 0x00, 0x1C }; // Highest speed
    uint8_t out2[] = { 0x01, 0xE0 }; // Lowest gain
    uint8_t out3[] = { 0x02, 0x00 }; // Continuous conversion mode

    if (!i2c_write(device, LSM303DLHC_I2C_ADDR, out1, sizeof(out1), NULL, 0)) return false;
    if (!i2c_write(device, LSM303DLHC_I2C_ADDR, out2, sizeof(out2), NULL, 0)) return false;
    return i2c_write(device, LSM303DLHC_I2C_ADDR, out3, sizeof(out3), NULL, 0);
}

bool_t lsm303_read(I2CDriver *device, uint8_t addr, uint8_t out, int16_t* data)
{
    uint8_t in[8];
    if (!i2c_write(device, addr, &out, sizeof(out), in, sizeof(in))) return false;

    //out[6] doesn't seem to reflect actual new data, so just discard it
    data[0] = (in[0] << 8) | in[1];
    data[1] = (in[2] << 8) | in[3];
    data[2] = (in[4] << 8) | in[5];

    return TRUE;
}
