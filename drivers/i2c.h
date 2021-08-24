#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

void i2c_init(void);
void i2c_set_slave_address(uint8_t addr);

/**
 * Wrapper functions for reading from registers with different address
 * and data sizes.
 * @return True if success, False if error
 * NOTE: Reads the most significant byte first if multi-byte data.
 * NOTE: Polling-based
 */
bool i2c_read_addr8_data8(uint8_t addr, uint8_t *data);
bool i2c_read_addr8_data16(uint8_t addr, uint16_t *data);
bool i2c_read_addr16_data8(uint16_t addr, uint8_t *data);
bool i2c_read_addr16_data16(uint16_t addr, uint16_t *data);
bool i2c_read_addr8_data32(uint16_t addr, uint32_t *data);
bool i2c_read_addr16_data32(uint16_t addr, uint32_t *data);

/**
 * Read byte_count bytes from address addr
 * NOTE: Polling-based
 */
bool i2c_read_addr8_bytes(uint8_t start_addr, uint8_t *bytes, uint16_t byte_count);

/**
 * Wrapper functions for writing to registers with different address
 * and data sizes.
 * @return True if success, False if error
 * NOTE: Writes the most significant byte if multi-byte data.
 * NOTE: Polling-based
 */
bool i2c_write_addr8_data8(uint8_t addr, uint8_t data);
bool i2c_write_addr8_data16(uint8_t addr, uint16_t data);
bool i2c_write_addr16_data8(uint16_t addr, uint8_t data);
bool i2c_write_addr16_data16(uint16_t addr, uint16_t data);

/**
 * Write an array of bytes of size byte_count to address addr.
 * NOTE: Polling-based
 */
bool i2c_write_addr8_bytes(uint8_t start_addr, uint8_t *bytes, uint16_t byte_count);

#endif /* I2C_H */
