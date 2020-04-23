// engler, cs240lx: simple scope skeleton for logic analyzer.
#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "../scope-constants.h"

// dumb log.  use your own if you like!
typedef struct {
    unsigned v,ncycles;
} log_ent_t;


// compute the number of cycles per second
unsigned cycles_per_sec(unsigned s) {
    demand(s < 2, will overflow);
    unsigned start = cycle_cnt_read();
    delay_ms(1000*s);
    return cycle_cnt_read() - start;
}

static volatile unsigned *LEV0 = (unsigned *)0x20200034;
static inline unsigned fast_gpio_read(unsigned pin) {
    return (*LEV0 & (1 << pin)) >> pin;
}

// monitor <pin>, recording any transitions until either:
//  1. we have run for about <max_cycles>.  
//  2. we have recorded <n_max> samples.
//
// return value: the number of samples recorded.
unsigned scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
#   define TIMEOUT 100000
    unsigned i = 0, j;

    // Simpler log to minimize `ldr`/`str`.
    unsigned cycle_counts[n_max];

    // Wait for the first transition.
    unsigned val = fast_gpio_read(pin);
    while (fast_gpio_read(pin) == val);
    val = fast_gpio_read(pin);
    unsigned init_val = val;

    enable_cache();
    unsigned start = cycle_cnt_read();
    unsigned stop = start + max_cycles;
    for (;;) {
        for (j = 0; j < TIMEOUT; j++) {
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
            if (fast_gpio_read(pin) != val)
                break;
        }
        cycle_counts[i] = cycle_cnt_read();
        if (j == TIMEOUT)
            break;
        i++;
        val = 1-val;
        if (cycle_counts[i] > stop || i > n_max)
            break;
    }
    l[0].v = init_val;
    l[0].ncycles = cycle_counts[0] - start;
    for (int j = 1; j < i; j++) {
        l[j].v = init_val & (j ^ 1);
        l[j].ncycles = cycle_counts[j] - cycle_counts[j-1];
    }
    return i;
}

// dump out the log, calculating the error at each point,
// and the absolute cumulative error.
void dump_samples(log_ent_t *l, unsigned n, unsigned period) {
    unsigned tot = 0, tot_err = 0;

    for(int i = 0; i < n-1; i++) {
        log_ent_t *e = &l[i];

        unsigned ncyc = e->ncycles;
        tot += ncyc;

        unsigned exp = period * (i+1);
        unsigned err = tot > exp ? tot - exp : exp - tot;
        
        tot_err += err;

        printk(" %d: val=%d, time=%d, tot=%d: exp=%d (err=%d, toterr=%d)\n", i, e->v, ncyc, tot, exp, err, tot_err);
    }
}

void notmain(void) {
    int pin = 21;
    gpio_set_input(pin);
    cycle_cnt_init();

#   define MAXSAMPLES 32
    log_ent_t log[MAXSAMPLES];

    unsigned n = scope(pin, log, MAXSAMPLES, cycles_per_sec(1));

    // <CYCLE_PER_FLIP> is in ../scope-constants.h
    printk("CYCLE_PER_FLIP = %u\n", CYCLE_PER_FLIP);
    dump_samples(log, n, CYCLE_PER_FLIP);
    clean_reboot();
}
