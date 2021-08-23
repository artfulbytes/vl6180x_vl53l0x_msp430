#include <msp430.h>
#include "drivers/i2c.h"

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
    uint8_t data = 0;

    while (1) {
        i2c_write_addr16_data8(0x10A, 13);
        i2c_read_addr16_data8(0x10A, &data);
    }

    return 0;
}
