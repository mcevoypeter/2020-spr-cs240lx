#include "rpi-asm.h"

MK_FN(dump_regs)
    stmia r0, {r0-r14}
    bx lr;


