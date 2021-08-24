#include <msp430.h>
#include "drivers/i2c.h"
#include "drivers/vl6180x.h"
#include "drivers/vl53l0x.h"

#define VL53L0X_SINGLE
//#define VL6180X_MUTLIPLE

static void msp430_init()
{
    WDTCTL = WDTPW | WDTHOLD; /* Stop watchdog timer */
    /* Configure the MCLK (and SMCLK) for 1 MHz using the Digitally controlled
     * oscillator (DCO) as source. */
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
}

int main(void)
{
    msp430_init();
    i2c_init();

#ifdef VL6180X_MUTLIPLE
    bool success = vl6180x_init();
    uint8_t ranges[3] = { 0 };
    while (success) {
        success = vl6180x_read_range_single(VL6180X_IDX_FIRST, &ranges[0]);
#ifdef VL6180X_SECOND
        success &= vl6180x_read_range_single(VL6180X_IDX_SECOND, &ranges[1]);
#endif
#ifdef VL6180X_THIRD
        success &= vl6180x_read_range_single(VL6180X_IDX_THIRD, &ranges[2]);
#endif
    }
#elif defined VL53L0X_SINGLE
    bool success = vl53l0x_init();
    uint16_t range = 0;
    while (success) {
        success = vl53l0x_read_range_single(&range);
    }
#endif
    return 0;
}
