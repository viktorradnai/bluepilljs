#ifndef MLX90393_H_INCLUDED
#define MLX90393_H_INCLUDED

#define MLX90393_I2C_ADDR 0x0C

typedef int bool_t;

bool_t mlx90393_init(void);
bool_t mlx90393_read(int16_t* data);
bool_t mlx90393_result(msg_t res);

#endif // MLX90393_H_INCLUDED
