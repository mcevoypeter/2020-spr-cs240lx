#ifndef __FAST_GPIO_H__
#define __FAST_GPIO_H__

volatile unsigned *LEV0 = (unsigned *)0x20200034;
inline unsigned fast_gpio_read(unsigned pin) {
    return (*LEV0 >> pin) & 1;
}


volatile unsigned *SET0 = (unsigned *)0x2020001c;
volatile unsigned *CLR0 = (unsigned *)0x20200028;
inline void fast_gpio_write(unsigned val, unsigned pin) {
    if (val)
        *SET0 = 1 << pin;
    else
        *CLR0 = 1 << pin;
}

#endif
