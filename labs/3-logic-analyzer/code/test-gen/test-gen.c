// engler, cs240lx: skeleton for test generation.  
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/cycle-util.h"

#include "../scope-constants.h"

static volatile unsigned *SET0 = (unsigned *)0x2020001c;
static volatile unsigned *CLR0 = (unsigned *)0x20200028;
static inline void fast_gpio_write(unsigned pin, unsigned val) {
    if (val)
        *SET0 = 1 << 21;
    else
        *CLR0 = 1 << 21;
}

// send N samples at <ncycle> cycles each in a simple way.
void test_gen(unsigned pin, unsigned N, unsigned ncycle) {
    enable_cache();
    unsigned val = 1;
    unsigned end = N*ncycle;
    unsigned cycles = ncycle;
    unsigned start = cycle_cnt_read();
    for (;;) {
        fast_gpio_write(pin, val); 
        while (cycle_cnt_read() - start < cycles);
        val = 1-val;
        cycles += ncycle;
        if (cycles > end)
            break;
    }
    end = cycle_cnt_read();

    // crude check how accurate we were ourselves.
    printk("expected %d cycles, have %d\n", ncycle*N, end-start);
}

void notmain(void) {
    /*delay_ms(5*1000);*/
    int pin = 21;
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    test_gen(pin, 11, CYCLE_PER_FLIP);

    clean_reboot();
}
