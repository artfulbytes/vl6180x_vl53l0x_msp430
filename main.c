#include <msp430.h>
#include "drivers/i2c.h"
#include "drivers/vl6180x.h"

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
    vl6180x_init();
    uint8_t range = 0;

    while (1) {
        vl6180x_read_range_single(&range);
    }

    return 0;
}
