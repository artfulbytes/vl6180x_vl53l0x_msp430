#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

void i2c_init(void);
bool i2c_write_byte(uint8_t byte);
bool i2c_read_byte(uint8_t *received_byte);

#endif /* I2C_H */
