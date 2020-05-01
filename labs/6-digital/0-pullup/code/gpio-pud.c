// put your pull-up/pull-down implementations here.
#include "rpi.h"

// see p. 91 of Broadcom BCM2835 peripherals document
struct gpio_pud {
    unsigned gppud;
    unsigned gppud_clk[2];
};
static volatile struct gpio_pud *gpio_pud = (struct gpio_pud *)0x20200094;

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
    if (pin > MAX_PIN)
        return;

    // set the control signal and delay 150 cycles
    gpio_pud->gppud = pull_up;
    delay_us(DELAY);

    // clock the control signal and delay 150 cycles
    gpio_pud->gppud_clk[BANK(pin)] |= 1 << SHIFT(pin);
    delay_us(DELAY);
    
    // remove the control signal and clock
    gpio_pud->gppud = disable;
    unsigned mask = ~(1 << SHIFT(pin));
    gpio_pud->gppud_clk[BANK(pin)] &= mask;
}

void gpio_set_pulldown(unsigned pin) {
    if (pin > MAX_PIN)
        return;

    // set the control signal and delay 150 cycles
    gpio_pud->gppud = pull_down;
    delay_us(DELAY);

    // clock the control signal and delay 150 cycles
    gpio_pud->gppud_clk[BANK(pin)] |= 1 << SHIFT(pin);
    delay_us(DELAY);
    
    // remove the control signal and clock
    gpio_pud->gppud = disable;
    unsigned mask = ~(1 << SHIFT(pin));
    gpio_pud->gppud_clk[BANK(pin)] &= mask;
}

void gpio_pud_off(unsigned pin) { 
    if (pin > MAX_PIN)
        return;

    // set the control signal and delay 150 cycles
    gpio_pud->gppud = disable;
    delay_us(DELAY);

    // clock the control signal and delay 150 cycles
    gpio_pud->gppud_clk[BANK(pin)] |= 1 << SHIFT(pin);
    delay_us(DELAY);
    
    // remove the control signal and clock
    gpio_pud->gppud = disable;
    unsigned mask = ~(1 << SHIFT(pin));
    gpio_pud->gppud_clk[BANK(pin)] &= mask;
}
