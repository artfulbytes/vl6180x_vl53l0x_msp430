#include "gpio.h"
#include <msp430.h>
#include <stdint.h>

void gpio_init(void)
{
    /* P1.0 GPIO and output low */
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    /* P1.1 GPIO and output low */
    P1SEL &= ~BIT1;
    P1DIR |= BIT1;
    P1OUT &= ~BIT1;

    /* P1.2 GPIO and output low */
    P1SEL &= ~BIT2;
    P1DIR |= BIT2;
    P1OUT &= ~BIT2;
}

void gpio_set_output(gpio_t gpio, bool enable)
{
    uint16_t bit = 0;
    switch (gpio)
    {
    case GPIO_XSHUT_FIRST:
        bit = BIT0;
        break;
    case GPIO_XSHUT_SECOND:
        bit = BIT1;
        break;
    case GPIO_XSHUT_THIRD:
        bit = BIT2;
        break;
    }

    if (enable) {
        P1OUT |= bit;
    } else {
        P1OUT &= ~bit;
    }
}
