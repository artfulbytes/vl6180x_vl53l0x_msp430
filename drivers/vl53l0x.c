#include "vl53l0x.h"
#include "i2c.h"
#include "gpio.h"

#define REG_IDENTIFICATION_MODEL_ID (0xC0)
#define REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV (0x89)
#define REG_MSRC_CONFIG_CONTROL (0x60)
#define REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT (0x44)
#define REG_SYSTEM_SEQUENCE_CONFIG (0x01)
#define REG_DYNAMIC_SPAD_REF_EN_START_OFFSET (0x4F)
#define REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD (0x4E)
#define REG_GLOBAL_CONFIG_REF_EN_START_SELECT (0xB6)
#define REG_SYSTEM_INTERRUPT_CONFIG_GPIO (0x0A)
#define REG_GPIO_HV_MUX_ACTIVE_HIGH (0x84)
#define REG_SYSTEM_INTERRUPT_CLEAR (0x0B)
#define REG_RESULT_INTERRUPT_STATUS (0x13)
#define REG_SYSRANGE_START (0x00)
#define REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 (0xB0)
#define REG_RESULT_RANGE_STATUS (0x14)
#define REG_SLAVE_DEVICE_ADDRESS (0x8A)

#define RANGE_SEQUENCE_STEP_TCC (0x10) /* Target CentreCheck */
#define RANGE_SEQUENCE_STEP_MSRC (0x04) /* Minimum Signal Rate Check */
#define RANGE_SEQUENCE_STEP_DSS (0x28) /* Dynamic SPAD selection */
#define RANGE_SEQUENCE_STEP_PRE_RANGE (0x40)
#define RANGE_SEQUENCE_STEP_FINAL_RANGE (0x80)

#define VL53L0X_EXPECTED_DEVICE_ID (0xEE)
#define VL53L0X_DEFAULT_ADDRESS (0x29)

typedef struct vl53l0x_info
{
    uint8_t addr;
    gpio_t xshut_gpio;
} vl53l0x_info_t;

typedef enum
{
    CALIBRATION_TYPE_VHV,
    CALIBRATION_TYPE_PHASE
} calibration_type_t;

static const vl53l0x_info_t vl53l0x_infos[] =
{
    [VL53L0X_IDX_FIRST] = { .addr = 0x30, .xshut_gpio = GPIO_XSHUT_FIRST },
#ifdef VL53L0X_SECOND
    [VL53L0X_IDX_SECOND] = { .addr = 0x31, .xshut_gpio = GPIO_XSHUT_SECOND },
#endif
#ifdef VL53L0X_THIRD
    [VL53L0X_IDX_THIRD] = { .addr = 0x32, .xshut_gpio = GPIO_XSHUT_THIRD },
#endif
};

static uint8_t stop_variable = 0;

/**
 * We can read the model id to confirm that the device is booted.
 * (There is no fresh_out_of_reset as on the vl6180x)
 */
static bool device_is_booted()
{
    uint8_t device_id = 0;
    if (!i2c_read_addr8_data8(REG_IDENTIFICATION_MODEL_ID, &device_id)) {
        return false;
    }
    return device_id == VL53L0X_EXPECTED_DEVICE_ID;
}

/**
 * One time device initialization
 */
static bool data_init()
{
    bool success = false;

    /* Set 2v8 mode */
    uint8_t vhv_config_scl_sda = 0;
    if (!i2c_read_addr8_data8(REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, &vhv_config_scl_sda)) {
        return false;
    }
    vhv_config_scl_sda |= 0x01;
    if (!i2c_write_addr8_data8(REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, vhv_config_scl_sda)) {
        return false;
    }

    /* Set I2C standard mode */
    success = i2c_write_addr8_data8(0x88, 0x00);

    success &= i2c_write_addr8_data8(0x80, 0x01);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x00, 0x00);
    /* It may be unnecessary to retrieve the stop variable for each sensor */
    success &= i2c_read_addr8_data8(0x91, &stop_variable);
    success &= i2c_write_addr8_data8(0x00, 0x01);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x80, 0x00);

    return success;
}

/**
 * Load tuning settings (same as default tuning settings provided by ST api code)
 */
static bool load_default_tuning_settings()
{
    bool success = i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x00, 0x00);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x09, 0x00);
    success &= i2c_write_addr8_data8(0x10, 0x00);
    success &= i2c_write_addr8_data8(0x11, 0x00);
    success &= i2c_write_addr8_data8(0x24, 0x01);
    success &= i2c_write_addr8_data8(0x25, 0xFF);
    success &= i2c_write_addr8_data8(0x75, 0x00);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x4E, 0x2C);
    success &= i2c_write_addr8_data8(0x48, 0x00);
    success &= i2c_write_addr8_data8(0x30, 0x20);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x30, 0x09);
    success &= i2c_write_addr8_data8(0x54, 0x00);
    success &= i2c_write_addr8_data8(0x31, 0x04);
    success &= i2c_write_addr8_data8(0x32, 0x03);
    success &= i2c_write_addr8_data8(0x40, 0x83);
    success &= i2c_write_addr8_data8(0x46, 0x25);
    success &= i2c_write_addr8_data8(0x60, 0x00);
    success &= i2c_write_addr8_data8(0x27, 0x00);
    success &= i2c_write_addr8_data8(0x50, 0x06);
    success &= i2c_write_addr8_data8(0x51, 0x00);
    success &= i2c_write_addr8_data8(0x52, 0x96);
    success &= i2c_write_addr8_data8(0x56, 0x08);
    success &= i2c_write_addr8_data8(0x57, 0x30);
    success &= i2c_write_addr8_data8(0x61, 0x00);
    success &= i2c_write_addr8_data8(0x62, 0x00);
    success &= i2c_write_addr8_data8(0x64, 0x00);
    success &= i2c_write_addr8_data8(0x65, 0x00);
    success &= i2c_write_addr8_data8(0x66, 0xA0);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x22, 0x32);
    success &= i2c_write_addr8_data8(0x47, 0x14);
    success &= i2c_write_addr8_data8(0x49, 0xFF);
    success &= i2c_write_addr8_data8(0x4A, 0x00);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x7A, 0x0A);
    success &= i2c_write_addr8_data8(0x7B, 0x00);
    success &= i2c_write_addr8_data8(0x78, 0x21);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x23, 0x34);
    success &= i2c_write_addr8_data8(0x42, 0x00);
    success &= i2c_write_addr8_data8(0x44, 0xFF);
    success &= i2c_write_addr8_data8(0x45, 0x26);
    success &= i2c_write_addr8_data8(0x46, 0x05);
    success &= i2c_write_addr8_data8(0x40, 0x40);
    success &= i2c_write_addr8_data8(0x0E, 0x06);
    success &= i2c_write_addr8_data8(0x20, 0x1A);
    success &= i2c_write_addr8_data8(0x43, 0x40);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x34, 0x03);
    success &= i2c_write_addr8_data8(0x35, 0x44);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x31, 0x04);
    success &= i2c_write_addr8_data8(0x4B, 0x09);
    success &= i2c_write_addr8_data8(0x4C, 0x05);
    success &= i2c_write_addr8_data8(0x4D, 0x04);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x44, 0x00);
    success &= i2c_write_addr8_data8(0x45, 0x20);
    success &= i2c_write_addr8_data8(0x47, 0x08);
    success &= i2c_write_addr8_data8(0x48, 0x28);
    success &= i2c_write_addr8_data8(0x67, 0x00);
    success &= i2c_write_addr8_data8(0x70, 0x04);
    success &= i2c_write_addr8_data8(0x71, 0x01);
    success &= i2c_write_addr8_data8(0x72, 0xFE);
    success &= i2c_write_addr8_data8(0x76, 0x00);
    success &= i2c_write_addr8_data8(0x77, 0x00);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x0D, 0x01);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x80, 0x01);
    success &= i2c_write_addr8_data8(0x01, 0xF8);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x8E, 0x01);
    success &= i2c_write_addr8_data8(0x00, 0x01);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x80, 0x00);
    return success;
}

static bool configure_interrupt()
{
    /* Interrupt on new sample ready */
    if (!i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04)) {
        return false;
    }

    /* Configure active low since the pin is pulled-up on most breakout boards */
    uint8_t gpio_hv_mux_active_high = 0;
    if (!i2c_read_addr8_data8(REG_GPIO_HV_MUX_ACTIVE_HIGH, &gpio_hv_mux_active_high)) {
        return false;
    }
    gpio_hv_mux_active_high &= ~0x10;
    if (!i2c_write_addr8_data8(REG_GPIO_HV_MUX_ACTIVE_HIGH, gpio_hv_mux_active_high)) {
        return false;
    }

    if (!i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return false;
    }
    return true;
}


/**
 * Enable (or disable) specific steps in the sequence
 */
static bool set_sequence_steps_enabled(uint8_t sequence_step)
{
    return i2c_write_addr8_data8(REG_SYSTEM_SEQUENCE_CONFIG, sequence_step);
}

/**
 * Basic device initialization
 */
static bool static_init()
{
    if (!load_default_tuning_settings()) {
        return false;
    }

    if (!configure_interrupt()) {
        return false;
    }

    if (!set_sequence_steps_enabled(RANGE_SEQUENCE_STEP_DSS +
                                    RANGE_SEQUENCE_STEP_PRE_RANGE +
                                    RANGE_SEQUENCE_STEP_FINAL_RANGE)) {
        return false;
    }

    return true;
}

static bool perform_single_ref_calibration(calibration_type_t calib_type)
{
    uint8_t sysrange_start = 0;
    uint8_t sequence_config = 0;
    switch (calib_type)
    {
    case CALIBRATION_TYPE_VHV:
        sequence_config = 0x01;
        sysrange_start = 0x01 | 0x40;
        break;
    case CALIBRATION_TYPE_PHASE:
        sequence_config = 0x02;
        sysrange_start = 0x01 | 0x00;
        break;
    }
    if (!i2c_write_addr8_data8(REG_SYSTEM_SEQUENCE_CONFIG, sequence_config)) {
        return false;
    }
    if (!i2c_write_addr8_data8(REG_SYSRANGE_START, sysrange_start)) {
        return false;
    }
    /* Wait for interrupt */
    uint8_t interrupt_status = 0;
    bool success = false;
    do {
        success = i2c_read_addr8_data8(REG_RESULT_INTERRUPT_STATUS, &interrupt_status);
    } while (success && ((interrupt_status & 0x07) == 0));
    if (!success) {
        return false;
    }
    if (!i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return false;
    }

    if (!i2c_write_addr8_data8(REG_SYSRANGE_START, 0x00)) {
        return false;
    }
    return true;
}

/**
 * Temperature calibration needs to be run again if the temperature changes by
 * more than 8 degrees according to the datasheet.
 */
static bool perform_ref_calibration()
{
    if (!perform_single_ref_calibration(CALIBRATION_TYPE_VHV)) {
        return false;
    }
    if (!perform_single_ref_calibration(CALIBRATION_TYPE_PHASE)) {
        return false;
    }
    /* Restore sequence steps enabled */
    if (!set_sequence_steps_enabled(RANGE_SEQUENCE_STEP_DSS +
                                    RANGE_SEQUENCE_STEP_PRE_RANGE +
                                    RANGE_SEQUENCE_STEP_FINAL_RANGE)) {
        return false;
    }
    return true;
}

static bool configure_address(uint8_t addr)
{
    /* 7-bit address */
    return i2c_write_addr8_data8(REG_SLAVE_DEVICE_ADDRESS, addr & 0x7F);
}

/**
 * Sets the sensor in hardware standby by flipping the XSHUT pin.
 */
static void set_hardware_standby(vl53l0x_idx_t idx, bool enable)
{
    gpio_set_output(vl53l0x_infos[idx].xshut_gpio, !enable);
}

/**
 * Configures the GPIOs used for the XSHUT pin.
 * Output low by default means the sensors will be in
 * hardware standby after this function is called.
 *
 * NOTE: The pins are hard-coded to P1.0, P1.1, and P1.2.
 **/
static void configure_gpio()
{
    gpio_init();
    gpio_set_output(GPIO_XSHUT_FIRST, false);
    gpio_set_output(GPIO_XSHUT_SECOND, false);
    gpio_set_output(GPIO_XSHUT_THIRD, false);
}

/* Sets the address of a single VL53L0X sensor.
 * This functions assumes that all non-configured VL53L0X are still
 * in hardware standby. */
static bool init_address(vl53l0x_idx_t idx)
{
    set_hardware_standby(idx, false);
    i2c_set_slave_address(VL53L0X_DEFAULT_ADDRESS);

    /* The datasheet doesn't say how long we must wait to leave hw standby,
     * but using the same delay as vl6180x seems to work fine. */
    __delay_cycles(400);

    if (!device_is_booted()) {
        return false;
    }

    if (!configure_address(vl53l0x_infos[idx].addr)) {
        return false;
    }
    return true;
}

/**
 * Initializes the sensors by putting them in hw standby and then
 * waking them up one-by-one as described in AN4846.
 */
static bool init_addresses()
{
    /* Puts all sensors in hardware standby */
    configure_gpio();

    /* Wake each sensor up one by one and set a unique address for each one */
    if (!init_address(VL53L0X_IDX_FIRST)) {
        return false;
    }
#ifdef VL53L0X_SECOND
    if (!init_address(VL53L0X_IDX_SECOND)) {
        return false;
    }
#endif
#ifdef VL53L0X_THIRD
    if (!init_address(VL53L0X_IDX_THIRD)) {
        return false;
    }
#endif

    return true;
}

static bool init_config(vl53l0x_idx_t idx)
{
    i2c_set_slave_address(vl53l0x_infos[idx].addr);
    if (!data_init()) {
        return false;
    }
    if (!static_init()) {
        return false;
    }
    if (!perform_ref_calibration()) {
        return false;
    }
    return true;
}

bool vl53l0x_init()
{
    if (!init_addresses()) {
        return false;
    }
    if (!init_config(VL53L0X_IDX_FIRST)) {
        return false;
    }
#ifdef VL53L0X_SECOND
    if (!init_config(VL53L0X_IDX_SECOND)) {
        return false;
    }
#endif
#ifdef VL53L0X_THIRD
    if (!init_config(VL53L0X_IDX_THIRD)) {
        return false;
    }
#endif
    return true;
}

bool vl53l0x_read_range_single(vl53l0x_idx_t idx, uint16_t *range)
{
    i2c_set_slave_address(vl53l0x_infos[idx].addr);
    bool success = i2c_write_addr8_data8(0x80, 0x01);
    success &= i2c_write_addr8_data8(0xFF, 0x01);
    success &= i2c_write_addr8_data8(0x00, 0x00);
    success &= i2c_write_addr8_data8(0x91, stop_variable);
    success &= i2c_write_addr8_data8(0x00, 0x01);
    success &= i2c_write_addr8_data8(0xFF, 0x00);
    success &= i2c_write_addr8_data8(0x80, 0x00);
    if (!success) {
        return false;
    }

    if (!i2c_write_addr8_data8(REG_SYSRANGE_START, 0x01)) {
        return false;
    }

    uint8_t sysrange_start = 0;
    do {
        success = i2c_read_addr8_data8(REG_SYSRANGE_START, &sysrange_start);
    } while (success && (sysrange_start & 0x01));
    if (!success) {
        return false;
    }

    uint8_t interrupt_status = 0;
    do {
        success = i2c_read_addr8_data8(REG_RESULT_INTERRUPT_STATUS, &interrupt_status);
    } while (success && ((interrupt_status & 0x07) == 0));
    if (!success) {
        return false;
    }

    if (!i2c_read_addr8_data16(REG_RESULT_RANGE_STATUS + 10, range)) {
        return false;
    }

    if (!i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return false;
    }

    /* 8190 or 8191 may be returned when obstacle is out of range. */
    if (*range == 8190 || *range == 8191) {
        *range = VL53L0X_OUT_OF_RANGE;
    }

    return true;
}
