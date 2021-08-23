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
    uint8_t received_byte = 0;

    while (1) {
        i2c_read_byte(&received_byte);
        i2c_write_byte(received_byte);
    }

    return 0;
}
