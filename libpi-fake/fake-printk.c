#include "fake-pi.h"
#include "rpi.h"
#include <stdarg.h>

int printk(const char *format, ...) {
    printf("PI:");
    va_list args;
    va_start(args, format);
    int rv = vprintf(format, args);
    va_end(args);
    return rv;
}
