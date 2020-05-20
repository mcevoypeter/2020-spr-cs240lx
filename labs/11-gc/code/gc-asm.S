#include "rpi-asm.h"

MK_FN(dump_regs)
    stm r0, {r0-r13}
    bx lr;


