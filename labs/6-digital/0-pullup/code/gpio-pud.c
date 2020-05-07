// put your pull-up/pull-down implementations here.
#include "rpi.h"

// see p. 91 of Broadcom BCM2835 peripherals document
struct gpio_pud {
    unsigned gppud;
    unsigned gppud_clk[2];
};
/*static volatile struct gpio_pud *gpio_pud = (struct gpio_pud *)0x20200094;*/
static volatile void *gppud = (void *)0x20200094;
static volatile void *gppud_clk0 = (void *)0x20200098;
/*static volatile void *gppud_clk1 = (void *)0x2020009c;*/


// see p. 101 of Broadcom BCM2835 peripherals document
enum {
    disable = 0,
    pull_down,
    pull_up,
    reserved
};

#define BANK(x) (x / 32)
#define DELAY 150
#define MAX_PIN 53
#define SHIFT(x) (x % 32)

void gpio_set_pullup(unsigned pin) {
    dev_barrier();

    if (pin > MAX_PIN)
        return;
    
    put32(gppud, pull_up); 
    delay_us(DELAY);

    put32(gppud_clk0, 1 << SHIFT(pin));
    delay_us(DELAY);

    put32(gppud, disable);
    delay_us(DELAY);

    put32(gppud_clk0, disable);
    delay_us(DELAY);

    dev_barrier();
}

void gpio_set_pulldown(unsigned pin) {
    dev_barrier();

    if (pin > MAX_PIN)
        return;

    put32(gppud, pull_down); 
    delay_us(DELAY);

    put32(gppud_clk0, 1 << SHIFT(pin));
    delay_us(DELAY);

    put32(gppud, disable);
    delay_us(DELAY);

    put32(gppud_clk0, disable);
    delay_us(DELAY);
    
    dev_barrier();
}

void gpio_pud_off(unsigned pin) { 
    dev_barrier(); 

    if (pin > MAX_PIN)
        return;

    put32(gppud, disable); 
    delay_us(DELAY);

    put32(gppud_clk0, 1 << SHIFT(pin));
    delay_us(DELAY);

    put32(gppud, disable);
    delay_us(DELAY);

    put32(gppud_clk0, disable);
    delay_us(DELAY);

    dev_barrier();
}
