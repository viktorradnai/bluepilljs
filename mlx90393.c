#include "hal.h"
#include "ch.h"
#include "chprintf.h"

#include "i2c_util.h"
#include "mlx90393.h"

extern SerialUSBDriver SDU1;

bool_t mlx90393_init(void)
{
    uint8_t out1[] = { 0x60, 0x00, 0x5C, 0x00 << 2 };
    uint8_t out2[] = { 0x60, 0x02, 0xB4, 0x02 << 2 };
    uint8_t out3[] = { 0x50, 0x02 << 2 };
    uint8_t in[4];

    if (!i2c_write(MLX90393_I2C_ADDR, out1, sizeof(out1), in, 2)) return FALSE;
    if (!i2c_write(MLX90393_I2C_ADDR, out2, sizeof(out2), in, 2)) return FALSE;
    return i2c_write(MLX90393_I2C_ADDR, out3, sizeof(out3), in, 4);
}

bool_t mlx90393_read(int16_t* data)
{
    uint8_t in[8];

    uint8_t out1[] = { 0x3E };
    uint8_t out2[] = { 0x4E };

    if (!i2c_write(MLX90393_I2C_ADDR, out1, sizeof(out1), in, 2)) return FALSE;

    chThdSleepMilliseconds(100);

    if (!i2c_write(MLX90393_I2C_ADDR, out2, sizeof(out2), in, sizeof(in))) return FALSE;

    data[0] = (in[3] << 8) | in[2];
    data[1] = (in[5] << 8) | in[4];
    data[2] = (in[7] << 8) | in[6];

    chprintf((BaseSequentialStream *)&SDU1, "MLX90393 XYZ: %d %d %d\r\n", data[0], data[1], data[2]);
    return TRUE;
}
