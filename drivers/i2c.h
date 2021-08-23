#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

void i2c_init(void);
bool i2c_read_addr16_data8(uint16_t addr, uint8_t *data);
bool i2c_write_addr16_data8(uint16_t addr, uint8_t data);

#endif /* I2C_H */
