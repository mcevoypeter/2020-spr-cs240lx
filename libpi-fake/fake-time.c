// simple fake time implementation.  add other time code you need.
#include "fake-pi.h"
#include "rpi.h"

unsigned fake_time_usec = 0;

// set the starting fake time.
void fake_time_init(unsigned init_time) {
    fake_time_usec = init_time;
}

unsigned fake_time_inc(unsigned inc) {
    fake_time_usec += inc;
    return fake_time_usec;
}
