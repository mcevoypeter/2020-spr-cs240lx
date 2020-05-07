#include "fake-pi.h"
#include "rpi.h"

unsigned timer_get_usec(void) {
    fake_time_usec += 1;
    trace("getting usec = %uusec\n", fake_time_usec);
    return fake_time_usec;
}
