// engler, cs240lx: skeleton for test generation.  
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"

#include "../scope-constants.h"

extern void generate(unsigned pin, unsigned N, unsigned ncycle);

// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, unsigned N, unsigned ncycle) {
    unsigned start = cycle_cnt_read();
    for (int i = 0; i < N; i++) {
        // Why doesn't this work? 
        // delay_cycles(ncycle);
        while (cycle_cnt_read() - start < (i+1)*ncycle)
            asm("nop");
        gpio_write(pin, (i & 1) ^ 1);
    }
#if 0
    // Why doesn't this work?
    asm("push {r0-r2, lr}");
    asm("mov r0, %[pin]" : : [pin] "r" (pin));
    asm("mov r1, %[N]" : : [N] "r" (N));
    asm("mov r2, %[ncycle]" : : [ncycle] "r" (ncycle));
    asm("bl generate");
    asm("pop {r0-r2, lr}");
#endif

    unsigned end = cycle_cnt_read();

    // crude check how accurate we were ourselves.
    printk("expected %d cycles, have %d\n", ncycle*N, end-start);
}

void notmain(void) {
    delay_ms(5*1000);
    int pin = 21;
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    test_gen(pin, 11*1000, CYCLE_PER_FLIP);

    clean_reboot();
}
