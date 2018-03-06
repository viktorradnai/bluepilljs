#ifndef I2C_H_INCLUDED
#define I2C_H_INCLUDED

typedef int bool_t;

bool_t i2c_write(uint8_t addr, uint8_t *tx, uint8_t txsize, uint8_t *rx, uint8_t rxsize);

#endif // I2C_H_INCLUDED
