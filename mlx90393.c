#include "hal.h"
#include "ch.h"

#include "i2c_util.h"
#include "mlx90393.h"

extern SerialUSBDriver SDU1;

bool_t mlx90393_init(I2CDriver *device)
{
    // MLX903939 Write register format: CMD, DATA_H, DATA_L, ADDR << 2
    // Address must be shifted up by 2 bits

    uint8_t out1[] = { MLX90393_CMD_WRITE_REG, 0x00, 0x5C, 0x00 << 2 }; // GAIN_SEL = 0x5, HALLCONF = 0xC
    uint8_t out2[] = { MLX90393_CMD_WRITE_REG, MLX90393_TCMP_EN << 2, 0x00, 0x01 << 2 };
    uint8_t out3[] = { MLX90393_CMD_WRITE_REG, 0x02, 0xA8, 0x02 << 2 }; // RES_XYZ = 0b010101, DIG_FILT = 0b101
    uint8_t out4[] = { MLX90393_CMD_READ_REG, 0x02 << 2 };
    uint8_t in[4];

    if (!i2c_write(device, MLX90393_I2C_ADDR, out1, sizeof(out1), in, 2)) return FALSE;
    if (!i2c_write(device, MLX90393_I2C_ADDR, out2, sizeof(out2), in, 2)) return FALSE;
    if (!i2c_write(device, MLX90393_I2C_ADDR, out3, sizeof(out3), in, 2)) return FALSE;
    return i2c_write(device, MLX90393_I2C_ADDR, out4, sizeof(out4), in, 4);
}

bool_t mlx90393_read(I2CDriver *device, int16_t* data)
{
    uint8_t in[8];

    uint8_t out1[] = { MLX90393_CMD_MEASURE_XYZ };
    uint8_t out2[] = { MLX90393_CMD_READ_XYZ };

    // Start single measurement
    if (!i2c_write(device, MLX90393_I2C_ADDR, out1, sizeof(out1), in, 2)) return FALSE;

    // Conversion time = 0.63ms * (2 + 2^DIG_FILT) * 3
    chThdSleepMilliseconds(20);

    // Fetch result
    if (!i2c_write(device, MLX90393_I2C_ADDR, out2, sizeof(out2), in, sizeof(in))) return FALSE;

#if MLX90393_TCMP_EN
    /*
     * Enabling the temperature compensation changes the way the magnetic values
     * are encoded and transmitted so conversion is necessary
     */
    int32_t tmp;
    tmp = (in[3] << 8) | in[2];
    data[0] = (int16_t) (tmp - 32768);
    tmp = (in[5] << 8) | in[4];
    data[1] = (int16_t) (tmp - 32768);
    tmp = (in[7] << 8) | in[6];
    data[2] = (int16_t) (tmp - 32768);
#else
    data[0] = (in[3] << 8) | in[2];
    data[1] = (in[5] << 8) | in[4];
    data[2] = (in[7] << 8) | in[6];
#endif // MLX90393_TCMP_EN
    return TRUE;
}
