#include "fake-pi.h"
#include "rpi.h"

void gpio_set_pullup(unsigned pin) {
    trace("pin=%u\n", pin);
}

void gpio_set_pulldown(unsigned pin) {
    trace("pin=%u\n", pin);
}

void gpio_pud_off(unsigned pin) {
    trace("pin=%u\n", pin);
}
