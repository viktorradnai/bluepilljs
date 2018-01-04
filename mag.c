#include "hal.h"
#include "ch.h"

#define MAG_DEV_ADDR 0x1E
#define MAG_DEV_LSM303C 1
#define MAG_DEV_LSM303HDLC 2
#define MAG_DEV_TYPE MAG_DEV_LSM303HDLC

void writeByteI2C(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t in[] = {reg, val};
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, in, sizeof(in), NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
}
#if MAG_DEV_TYPE == MAG_DEV_LSM303C
#define MAG_DEV_START_REG 0x28
void magInit(void)
{
    // Highest speed
    writeByteI2C(MAG_DEV_ADDR, 0x20, 0x7C);
    // Lowest gain
    writeByteI2C(MAG_DEV_ADDR, 0x21, 0x60);
    // Continuous conversion mode
    writeByteI2C(MAG_DEV_ADDR, 0x22, 0x00);

    writeByteI2C(MAG_DEV_ADDR, 0x23, 0x0E);
}
#elif MAG_DEV_TYPE == MAG_DEV_LSM303HDLC
#define MAG_DEV_START_REG 0x03
void magInit(void)
{
    // Highest speed
    writeByteI2C(MAG_DEV_ADDR, 0x00, 0x1C);
    // Lowest gain
    writeByteI2C(MAG_DEV_ADDR, 0x01, 0xE0);
    // Continuous conversion mode
    writeByteI2C(MAG_DEV_ADDR, 0x02, 0x00);
}
#endif // MAG_DEV

void magRead(float* data)
{
    uint8_t in[] = { MAG_DEV_START_REG };
    uint8_t out[7];
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, in, sizeof(in), out, sizeof(out), TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    //out[6] doesn't seem to reflect actual new data, so just discard it
    int16_t val_x = (out[0] << 8) | out[1];
    int16_t val_z = (out[2] << 8) | out[3];
    int16_t val_y = (out[4] << 8) | out[5];
    data[0] = ((float)val_x)*1.22;
    data[1] = ((float)val_y)*1.22;
    data[2] = ((float)val_z)*1.22;
}
