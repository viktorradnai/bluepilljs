#ifndef LSM303_H_INCLUDED
#define LSM303_H_INCLUDED

#define LSM303C_I2C_ADDR 0x1E
#define LSM303C_START_REG 0x28

#define LSM303DLHC_I2C_ADDR 0x1E
#define LSM303DLHC_START_REG 0x03

#define lsm303c_read(data) lsm303_read(LSM303C_I2C_ADDR, LSM303C_START_REG, data)
#define lsm303dlhc_read(data) lsm303_read(LSM303DLHC_I2C_ADDR, LSM303DLHC_START_REG, data)

typedef int bool_t;

bool_t lsm303c_init(void);
bool_t lsm303dlhc_init(void);
bool_t lsm303_read(uint8_t addr, uint8_t out, int16_t* data);

#endif // LSM303_H_INCLUDED
