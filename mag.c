#include "hal.h"
#include "ch.h"

#include "chprintf.h"

#define MAG_DEV_LSM303C 1
#define MAG_DEV_LSM303HDLC 2
#define MAG_DEV_MLX90393 3
#define MAG_DEV_TYPE MAG_DEV_MLX90393
extern SerialUSBDriver SDU1;


void writeByteI2C(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t in[] = {reg, val};
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, in, sizeof(in), NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
}
#if 0
#define MAG_DEV_ADDR 0x1E
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
#elif 0
#define MAG_DEV_ADDR 0x1E
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
#elif 1
#define MAG_DEV_ADDR 0x0C
void magInit(void)
{
    uint8_t out1[] = { 0x60, 0x00, 0x5C, 0x00 << 2 };
    uint8_t out2[] = { 0x60, 0x02, 0xB4, 0x02 << 2 };
    uint8_t out3[] = { 0x50, 0x02 << 2 };
    uint8_t in1[2], in2[2], in3[4];
    msg_t res1, res2, res3;
    i2cAcquireBus(&I2CD1);
    res2 = i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, out2, sizeof(out2), in2, sizeof(in2), TIME_INFINITE);
    switch (res2) {
        case MSG_OK:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init OK: %x %x\r\n", res2, in2[0]);
            break;
        case MSG_RESET:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init error: %x %x\r\n", res2, in2[0]);
            break;
        case MSG_TIMEOUT:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init timeout: %x\r\n", res2);
            break;
        default:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 unknown result: %x\r\n", res2);
            break;
    }
    res1 = i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, out1, sizeof(out1), in1, sizeof(in1), TIME_INFINITE);

    switch (res1) {
        case MSG_OK:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init OK: %x %x\r\n", res1, in1[0]);
            break;
        case MSG_RESET:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init error: %x %x\r\n", res1, in1[0]);
            break;
        case MSG_TIMEOUT:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init timeout: %x\r\n", res1);
            break;
        default:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 unknown result: %x\r\n", res1);
            break;
    }

    res3 = i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, out3, sizeof(out3), in3, sizeof(in3), 100);

    switch (res3) {
        case MSG_OK:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init OK: %x %x %x %x\r\n", res3, in3[0], in3[1], in3[2]);
            break;
        case MSG_RESET:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init error: %x %x %x\r\n", res3, in3[0], in3[1]);
            break;
        case MSG_TIMEOUT:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 Init timeout: %x\r\n", res3);
            break;
        default:
            chprintf((BaseSequentialStream *)&SDU1, "MLX30393 unknown result: %x\r\n", res3);
            break;
    }
    i2cReleaseBus(&I2CD1);
}
#endif // MAG_DEV

void magRead(float* data)
{
    uint8_t in1[2];
    uint8_t in[8];
    palTogglePad(GPIOC, GPIOC_LED);
#if MAG_DEV_TYPE == MAG_DEV_MLX90393
    uint8_t out1[] = { 0x3E };
    uint8_t out2[] = { 0x4E };
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, out1, sizeof(out1), in1, sizeof(in1), 100);
    i2cReleaseBus(&I2CD1);
    chThdSleepMilliseconds(100);
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, out2, sizeof(out2), in, sizeof(in), 100);
    i2cReleaseBus(&I2CD1);

    int16_t val_x = (in[3] << 8) | in[2];
    int16_t val_z = (in[5] << 8) | in[4];
    int16_t val_y = (in[7] << 8) | in[6];
#else
    uint8_t out[] = { MAG_DEV_START_REG };
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, MAG_DEV_ADDR, out, sizeof(out), in, sizeof(in), TIME_INFINITE);
    i2cReleaseBus(&I2CD1);

    //out[6] doesn't seem to reflect actual new data, so just discard it
    int16_t val_x = (in[0] << 8) | in[1];
    int16_t val_z = (in[2] << 8) | in[3];
    int16_t val_y = (in[4] << 8) | in[5];
#endif // MAG_DEV_TYPE



    chprintf((BaseSequentialStream *)&SDU1, "XYZ: %d %d %d\r\n", val_x,val_y, val_z);
    data[0] = ((float)val_x)*1.22;
    data[1] = ((float)val_y)*1.22;
    data[2] = ((float)val_z)*1.22;
}
