#include "fake-pi.h"
#include "rpi.h"

void delay_us(unsigned us) {
    fake_time_inc(us);
    trace("delay_us = %uusec\n", us);
}

void delay_ms(unsigned ms) {
    fake_time_inc(1000*ms);
    trace("delay_ms = %umsec\n", ms);
}
