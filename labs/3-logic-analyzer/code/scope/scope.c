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
    return (*LEV0 >> pin) & 1;
}

// monitor <pin>, recording any transitions until either:
//  1. we have run for about <max_cycles>.  
//  2. we have recorded <n_max> samples.
//
// return value: the number of samples recorded.
unsigned scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    printk("ready to scope\n");
    enable_cache();

    // Simpler log to minimize `ldr`/`str`.
    unsigned cycle_cnts[n_max];

    // Declare these early.
    unsigned i, cycle_cnt = 0, n = 0;

    // Wait for the first transition.
    unsigned val = fast_gpio_read(pin);
    while (fast_gpio_read(pin) == val);
    val = 1-val;
    unsigned init_val = val;

    unsigned start = cycle_cnt_read();
    unsigned stop = start + max_cycles;
    while (cycle_cnt < stop && n < n_max) {
        for (i = 0; i < CYCLE_PER_FLIP; i++) {
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
            if (fast_gpio_read(pin) != val) {
                cycle_cnt = cycle_cnt_read();
                break;
            }
        }
        if (i == CYCLE_PER_FLIP)
            break;
        val = 1-val;
        cycle_cnts[n++] = cycle_cnt;
    }
    l[0].v = init_val;
    l[0].ncycles = cycle_cnts[0] - start;
    for (i = 1; i < n; i++) {
        l[i].v = init_val & (i ^ 1);
        l[i].ncycles = cycle_cnts[i] - cycle_cnts[i-1];
    }
    return n;
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

    log_ent_t log[MAX_SAMPLES];

    unsigned n = scope(pin, log, MAX_SAMPLES, cycles_per_sec(1));

    // <CYCLE_PER_FLIP> is in ../scope-constants.h
    dump_samples(log, n, CYCLE_PER_FLIP);
    clean_reboot();
}
