#include "vl6180x.h"
#include "i2c.h"

#define REG_FRESH_OUT_OF_RESET (0x0016)
#define REG_AVERAGING_SAMPLE_PERIOD (0x010A)
#define REG_SYSALS_ANALOGUE_GAIN (0x003F)
#define REG_SYSRANGE_VHV_REPEAT_RATE (0x0031)
#define REG_SYSALS_INTEGRATION_PERIOD (0x0040)
#define REG_SYSRANGE_VHV_RECALIBRATE (0x002E)
#define REG_SYSRANGE_INTERMEASUREMENT_PERIOD (0x001B)
#define REG_SYSALS_INTERMEASUREMENT_PERIOD (0x003E)
#define REG_INTERRUPT_CONFIG_GPIO (0x014)
#define REG_MAX_CONVERGENCE_TIME (0x01C)
#define REG_INTERLEAVED_MODE_ENABLE (0x2A3)
#define REG_RANGE_START (0x018)
#define REG_RESULT_RANGE_STATUS (0x04d)
#define REG_INTERRUPT_STATUS_GPIO (0x04F)
#define REG_RANGE_VAL (0x062)
#define REG_INTERRUPT_CLEAR (0x015)
#define REG_SLAVE_DEVICE_ADDRESS (0x212)

/**
 * Waits for the device to be booted by reading the fresh out of reset
 * register.
 * NOTE: Blocks indefinitely if device is not freshly booted.
 */
static bool wait_device_booted()
{
    uint8_t fresh_out_of_reset = 0;
    bool success = false;
    do {
        success = i2c_read_addr16_data8(REG_FRESH_OUT_OF_RESET, &fresh_out_of_reset);
    } while (fresh_out_of_reset != 1 && !success);
    return success;
}

/**
 * Writes the settings recommended in the AN4545 application note.
 */
static bool write_standard_ranging_settings()
{
    bool success = i2c_write_addr16_data8(0x207, 0x01);
    success &= i2c_write_addr16_data8(0x208, 0x01);
    success &= i2c_write_addr16_data8(0x096, 0x00);
    success &= i2c_write_addr16_data8(0x097, 0xFD);
    success &= i2c_write_addr16_data8(0x0E3, 0x01);
    success &= i2c_write_addr16_data8(0x0E4, 0x03);
    success &= i2c_write_addr16_data8(0x0E5, 0x02);
    success &= i2c_write_addr16_data8(0x0E6, 0x01);
    success &= i2c_write_addr16_data8(0x0E7, 0x03);
    success &= i2c_write_addr16_data8(0x0F5, 0x02);
    success &= i2c_write_addr16_data8(0x0D9, 0x05);
    success &= i2c_write_addr16_data8(0x0DB, 0xCE);
    success &= i2c_write_addr16_data8(0x0DC, 0x03);
    success &= i2c_write_addr16_data8(0x0DD, 0xF8);
    success &= i2c_write_addr16_data8(0x09F, 0x00);
    success &= i2c_write_addr16_data8(0x0A3, 0x3C);
    success &= i2c_write_addr16_data8(0x0B7, 0x00);
    success &= i2c_write_addr16_data8(0x0BB, 0x3C);
    success &= i2c_write_addr16_data8(0x0B2, 0x09);
    success &= i2c_write_addr16_data8(0x0CA, 0x09);
    success &= i2c_write_addr16_data8(0x198, 0x01);
    success &= i2c_write_addr16_data8(0x1B0, 0x17);
    success &= i2c_write_addr16_data8(0x1AD, 0x00);
    success &= i2c_write_addr16_data8(0x0FF, 0x05);
    success &= i2c_write_addr16_data8(0x100, 0x05);
    success &= i2c_write_addr16_data8(0x199, 0x05);
    success &= i2c_write_addr16_data8(0x1A6, 0x1B);
    success &= i2c_write_addr16_data8(0x1AC, 0x3E);
    success &= i2c_write_addr16_data8(0x1A7, 0x1F);
    success &= i2c_write_addr16_data8(0x030, 0x00);
    return success;
}

static bool configure_default()
{
    bool success = false;
    /* Default configuration recommended by application note AN4545 */
    success = i2c_write_addr16_data8(REG_AVERAGING_SAMPLE_PERIOD, 0x30);
    success &= i2c_write_addr16_data8(REG_SYSALS_ANALOGUE_GAIN, 0x46);
    success &= i2c_write_addr16_data8(REG_SYSRANGE_VHV_REPEAT_RATE, 0xFF);
    success &= i2c_write_addr16_data16(REG_SYSALS_INTEGRATION_PERIOD, 0x63);
    success &= i2c_write_addr16_data8(REG_SYSRANGE_VHV_RECALIBRATE, 0x01);
    success &= i2c_write_addr16_data8(REG_SYSRANGE_INTERMEASUREMENT_PERIOD, 0x09);
    success &= i2c_write_addr16_data8(REG_SYSALS_INTERMEASUREMENT_PERIOD, 0x31);
    success &= i2c_write_addr16_data8(REG_INTERRUPT_CONFIG_GPIO, 0x24);
    if (!success) {
        return false;
    }

    /* Abort measurement after 50 ms */
    if (!i2c_write_addr16_data8(REG_MAX_CONVERGENCE_TIME, 0x31)) {
        return false;
    }

    /* Disable interleaved mode */
    success = i2c_write_addr16_data8(REG_INTERLEAVED_MODE_ENABLE, 0);
    return success;
}

bool vl6180x_read_range_single(uint8_t *range)
{
    bool success = false;

    /* Wait device ready */
    uint8_t result_range_status = 0;
    do {
        success = i2c_read_addr16_data8(REG_RESULT_RANGE_STATUS, &result_range_status);
    } while (success && !(result_range_status & 0x01));
    if (!success) {
        return false;
    }

    /* Start range measurement */
    if (!i2c_write_addr16_data8(REG_RANGE_START, 0x01)) {
        return false;
    }

    /* Wait for interrupt (polling) */
    uint8_t interrupt_status = 0;
    do {
        success = i2c_read_addr16_data8(REG_INTERRUPT_STATUS_GPIO, &interrupt_status);
    } while (success && (interrupt_status == 0));
    if (!success) {
        return false;
    }

    /* Read range result */
    if (!i2c_read_addr16_data8(REG_RANGE_VAL, range)) {
        return false;
    }

    /* Clear interrupt */
    success = i2c_write_addr16_data8(REG_INTERRUPT_CLEAR, 0x07);
    return success;
}

bool vl6180x_init()
{
    bool success = false;

    if (!wait_device_booted()) {
        return false;
    }

    if (!write_standard_ranging_settings()) {
        return false;
    }

    if (!configure_default()) {
        return false;
    }

    /* No longer fresh out of reset */
    success = i2c_write_addr16_data8(REG_FRESH_OUT_OF_RESET, 0);

    return success;
}
