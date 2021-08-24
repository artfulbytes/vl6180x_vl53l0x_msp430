#include "i2c.h"
#include <msp430.h>

#define DEFAULT_SLAVE_ADDRESS (0x29)

typedef enum
{
    ADDR_SIZE_8BIT,
    ADDR_SIZE_16BIT
} addr_size_t;

typedef enum
{
    REG_SIZE_8BIT,
    REG_SIZE_16BIT,
    REG_SIZE_32BIT
} reg_size_t;

static bool start_transfer(addr_size_t addr_size, uint16_t addr)
{
    bool success = false;
    UCB0CTL1 |= UCTXSTT + UCTR; /* Set up master as TX and send start condition */

    /* Note, when master is TX, we must write to TXBUF before waiting for UCTXSTT */
    switch (addr_size) {
    case ADDR_SIZE_8BIT:
        UCB0TXBUF = addr & 0xFF;
        break;
    case ADDR_SIZE_16BIT:
        UCB0TXBUF = (addr >> 8) & 0xFF; /* Send the most significant byte of the 16-bit address */
        break;
    }

    while (UCB0CTL1 & UCTXSTT); /* Wait for start condition to be sent */
    success = !(UCB0STAT & UCNACKIFG);
    if (success) {
        while (!(IFG2 & UCB0TXIFG)); /* Wait for byte to be sent */
        success = !(UCB0STAT & UCNACKIFG);
    }

    if (success) {
        switch (addr_size) {
        case ADDR_SIZE_8BIT:
            break;
        case ADDR_SIZE_16BIT:
            UCB0TXBUF = addr & 0xFF; /* Send the least significant byte of the 16-bit address */
            while (!(IFG2 & UCB0TXIFG)); /* Wait for byte to be sent */
            success = !(UCB0STAT & UCNACKIFG);
            break;
        }
    }
    return success;
}

static void stop_transfer()
{
    UCB0CTL1 |= UCTXSTP; /* Send stop condition */
    while (UCB0CTL1 & UCTXSTP); /* Wait for stop condition to be sent */
}

/* Read a register of size reg_size at address addr.
 * NOTE: The bytes are read from MSB to LSB. */
static bool read_reg(addr_size_t addr_size, uint16_t addr, reg_size_t reg_size, uint8_t *data)
{
    bool success = false;

    if (!start_transfer(addr_size, addr)) {
        return false;
    }

    /* Address sent, now configure as receiver and get the data */
    UCB0CTL1 &= ~UCTR; /* Set as a receiver */
    UCB0CTL1 |= UCTXSTT; /* Send (repeating) start condition (including address of slave) */
    while (UCB0CTL1 & UCTXSTT); /* Wait for start condition to be sent */
    success = !(UCB0STAT & UCNACKIFG);
    if (success) {
        switch (reg_size) {
        case REG_SIZE_8BIT:
            break;
        case REG_SIZE_16BIT:
            /* Bytes are read from most to least significant */
            while ((IFG2 & UCB0RXIFG) == 0); /* Wait for byte before reading the buffer */
            data[1] = UCB0RXBUF; /* RX interrupt is cleared automatically afterwards */
            break;
        case REG_SIZE_32BIT:
            /* Bytes are read from most to least significant */
            while ((IFG2 & UCB0RXIFG) == 0);
            data[3] = UCB0RXBUF;
            while ((IFG2 & UCB0RXIFG) == 0);
            data[2] = UCB0RXBUF;
            while ((IFG2 & UCB0RXIFG) == 0);
            data[1] = UCB0RXBUF;
            break;
        }
        stop_transfer();
        while ((IFG2 & UCB0RXIFG) == 0); /* Wait for byte before reading the buffer */
        data[0] = UCB0RXBUF; /* RX interrupt is cleared automatically afterwards */
    }

    return success;
}

static bool read_reg_bytes(addr_size_t addr_size, uint16_t addr, uint8_t *bytes, uint16_t byte_count)
{
    bool success = false;
    bool transfer_stopped = false;

    if (!start_transfer(addr_size, addr)) {
        return false;
    }

    /* Address sent, now configure as receiver and get the value */
    UCB0CTL1 &= ~UCTR; /* Set as a receiver */
    UCB0CTL1 |= UCTXSTT; /* Send (repeating) start condition (including address of slave) */
    while (UCB0CTL1 & UCTXSTT); /* Wait for start condition to be sent */
    success = !(UCB0STAT & UCNACKIFG);
    if (success) {
        for (int i = 0; i < byte_count; i++) {
            if (i + 1 == byte_count) {
                stop_transfer();
                transfer_stopped = true;
            }
            success = !(UCB0STAT & UCNACKIFG);
            if (success) {
                while ((IFG2 & UCB0RXIFG) == 0); /* Wait for byte before reading the buffer */
                bytes[i] = UCB0RXBUF; /* RX interrupt is cleared automatically afterwards */
            } else {
                break;
            }
        }
    }
    if (!transfer_stopped) {
        stop_transfer();
    }

    return success;
}

bool i2c_read_addr8_data8(uint8_t addr, uint8_t *data)
{
    return read_reg(ADDR_SIZE_8BIT, addr, REG_SIZE_8BIT, data);
}

bool i2c_read_addr8_data16(uint8_t addr, uint16_t *data)
{
    return read_reg(ADDR_SIZE_8BIT, addr, REG_SIZE_16BIT, (uint8_t *)data);
}

bool i2c_read_addr16_data8(uint16_t addr, uint8_t *data)
{
    return read_reg(ADDR_SIZE_16BIT, addr, REG_SIZE_8BIT, data);
}

bool i2c_read_addr16_data16(uint16_t addr, uint16_t *data)
{
    return read_reg(ADDR_SIZE_16BIT, addr, REG_SIZE_16BIT, (uint8_t *)data);
}

bool i2c_read_addr8_data32(uint16_t addr, uint32_t *data)
{
    return read_reg(ADDR_SIZE_8BIT, addr, REG_SIZE_32BIT, (uint8_t *)data);
}

bool i2c_read_addr16_data32(uint16_t addr, uint32_t *data)
{
    return read_reg(ADDR_SIZE_16BIT, addr, REG_SIZE_32BIT, (uint8_t *)data);
}

bool i2c_read_addr8_bytes(uint8_t start_addr, uint8_t *bytes, uint16_t byte_count)
{
    return read_reg_bytes(ADDR_SIZE_8BIT, start_addr, bytes, byte_count);
}

/* Write data to a register of size reg_size at address addr.
 * NOTE: Writes the most significant byte (MSB) first. */
static bool write_reg(addr_size_t addr_size, uint16_t addr, reg_size_t reg_size, uint16_t data)
{
    bool success = false;

    if (!start_transfer(addr_size, addr)) {
        return false;
    }

    switch (reg_size) {
    case REG_SIZE_8BIT:
        success = true;
        break;
    case REG_SIZE_16BIT:
        UCB0TXBUF = (data >> 8) & 0xFF; /* Start with the most significant byte */
        while (!(IFG2 & UCB0TXIFG)); /* Wait for byte to be sent */
        success = !(UCB0STAT & UCNACKIFG);
        break;
    case REG_SIZE_32BIT:
        /* Not supported */
        return false;
    }

    if (success) {
        UCB0TXBUF = 0xFF & data; /* Send the least significant byte */
        while (!(IFG2 & UCB0TXIFG)); /* Wait for byte to be sent */
        success = !(UCB0STAT & UCNACKIFG);
    }

    stop_transfer();
    return success;
}

static bool write_reg_bytes(addr_size_t addr_size, uint16_t addr, uint8_t *bytes, uint16_t byte_count)
{
    bool success = false;

    if (!start_transfer(addr_size, addr)) {
        return false;
    }

    for (uint16_t i = 0; i < byte_count; i++) {
        UCB0TXBUF = bytes[i];
        while (!(IFG2 & UCB0TXIFG)); /* Wait for byte to be sent */
        success = !(UCB0STAT & UCNACKIFG);
        if (!success) {
            break;
        }
    }

    stop_transfer();
    return success;
}

bool i2c_write_addr8_data8(uint8_t addr, uint8_t value)
{
    return write_reg(ADDR_SIZE_8BIT, addr, REG_SIZE_8BIT, value);
}

bool i2c_write_addr8_data16(uint8_t addr, uint16_t value)
{
    return write_reg(ADDR_SIZE_8BIT, addr, REG_SIZE_16BIT, value);
}

bool i2c_write_addr16_data8(uint16_t addr, uint8_t value)
{
    return write_reg(ADDR_SIZE_16BIT, addr, REG_SIZE_8BIT, value);
}

bool i2c_write_addr16_data16(uint16_t addr, uint16_t value)
{
    return write_reg(ADDR_SIZE_16BIT, addr, REG_SIZE_16BIT, value);
}
bool i2c_write_addr8_bytes(uint8_t start_addr, uint8_t *bytes, uint16_t byte_count)
{
    return write_reg_bytes(ADDR_SIZE_8BIT, start_addr, bytes, byte_count);
}

void i2c_set_slave_address(uint8_t addr)
{
    UCB0I2CSA = addr;
}

void i2c_init()
{
    /* Pinmux P1.6 (SCL) and P1.7 (SDA) to I2C peripheral  */
    P1SEL |= BIT6 + BIT7;
    P1SEL2 |= BIT6 + BIT7;

    UCB0CTL1 |= UCSWRST; /* Enable SW reset */
    UCB0CTL0 = UCMST + UCSYNC + UCMODE_3; /* Single master, synchronous mode, I2C mode */
    UCB0CTL1 |= UCSSEL_2; /* SMCLK */
    UCB0BR0 = 10; /* SMCLK / 10 = ~100kHz */
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST; /* Clear SW */
    i2c_set_slave_address(DEFAULT_SLAVE_ADDRESS);
}
