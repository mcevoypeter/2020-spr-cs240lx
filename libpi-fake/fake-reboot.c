#include "fake-pi.h"
#include "rpi.h"

void clean_reboot(void) {
    trace("clean reboot\n");
    exit(0);
}
