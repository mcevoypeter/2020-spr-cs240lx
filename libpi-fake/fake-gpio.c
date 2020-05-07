#include "fake-pi.h"
#include "rpi.h"

void gpio_set_function(unsigned pin, gpio_func_t function) {
    trace("pin=%u, function=%u\n", pin, function);
}

void gpio_set_input(unsigned pin) {
    trace("pin=%u\n", pin);
}

void gpio_set_output(unsigned pin) {
    trace("pin=%u\n", pin);
}

void gpio_write(unsigned pin, unsigned val) {
    trace("pin=%u\n, val=%u\n", pin, val);
}

int gpio_read(unsigned pin) {
    trace("pin=%u\n", pin);
    return fake_random() % 2;
}

void gpio_set_on(unsigned pin) {
    trace("pin=%u\n", pin);
}

void gpio_set_off(unsigned pin) {
    trace("pin=%u\n", pin);
}

int gpio_get_pud(unsigned pin) {
    trace("pin=%u\n", pin);
    return fake_random() % 2;
}
