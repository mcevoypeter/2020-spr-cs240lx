#include "rpi.h"
#include "tsop322.h"

void notmain(void) {
#   define DATA 21
    tsop322_init(DATA);
    unsigned packet = tsop322_reverse_engineer();
}
