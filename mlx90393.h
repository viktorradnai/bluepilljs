#ifndef MLX90393_H_INCLUDED
#define MLX90393_H_INCLUDED

#define MLX90393_I2C_ADDR 0x0C
#define MLX90393_CMD_READ_REG 0x50
#define MLX90393_CMD_WRITE_REG 0x60
#define MLX90393_CMD_MEASURE_XYZ 0x3E
#define MLX90393_CMD_READ_XYZ 0x4E


// Enables (1) or disables (0) the on-chip sensitivity drift compensation.
#define MLX90393_TCMP_EN 0


typedef int bool_t;

bool_t mlx90393_init(I2CDriver *device);
bool_t mlx90393_read(I2CDriver *device, int16_t* data);
bool_t mlx90393_result(msg_t res);

#endif // MLX90393_H_INCLUDED
