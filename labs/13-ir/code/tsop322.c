#include "cycle-count.h"
#include "rpi.h"
#include "tsop322.h"


static unsigned data_pin;
void tsop322_init(unsigned data) {
    data_pin = data;
    gpio_set_input(data_pin);
    gpio_set_pullup(data_pin);
    cycle_cnt_init();
}

#define wait_for_falling_edge() (wait_for_transition(1))
#define wait_for_rising_edge() (wait_for_transition(0))
static inline unsigned wait_for_transition(unsigned init_val) {
#   define TIMEOUT (3*1000*1000)
    unsigned start = timer_get_usec();
    while (1) {
        if (gpio_read(data_pin) == init_val)
            return 1;
        if (timer_get_usec() > start + TIMEOUT)
            return 0;
    }
}

static inline unsigned consume_bit(void) {
    if (!wait_for_falling_edge()) {
        printk("Timed out waiting for falling edge\n");
        return 0;
    }
    unsigned start = timer_get_usec();
    if (!wait_for_rising_edge()) {
        printk("Timed out waiting for rising edge\n");
        return 0;
    }
    return timer_get_usec() - start;
}

typedef struct {
    unsigned val, duration;
} event_t;
void tsop322_poll(void) {
#   define MAX_EVENTS 128
    event_t events[MAX_EVENTS];

    printk("Polling the receiver for events\n");
    unsigned cnt = 0;
    for (; cnt < MAX_EVENTS; cnt++) {
        unsigned duration = consume_bit();
        events[cnt] = (event_t){ 
            .val = 0, 
            .duration = duration
        };
    }
    printk("Finished polling\n");

    for (unsigned i = 0; i < cnt; i++) {
        printk("%04u: val=%u, duration=%uusec\n",
                i, events[i].val, events[i].duration);
    }
}


unsigned tsop322_reverse_engineer(void) {
    printk("Reverse engineering a button press\n");
    unsigned v = 0; 
    
    // TODO: error check
    // start of transmission appears to be two bits 
    //   - ~2,000,000us
    //   - ~4,500us
    consume_bit();
    consume_bit();

    for (unsigned i = 0; i < 4; i++) {
        unsigned byte_offset = 8*i;
        v |= consume_bit() << (byte_offset);
        v |= consume_bit() << (byte_offset + 1);
        v |= consume_bit() << (byte_offset + 2);
        v |= consume_bit() << (byte_offset + 3);
        v |= consume_bit() << (byte_offset + 4);
        v |= consume_bit() << (byte_offset + 5);
        v |= consume_bit() << (byte_offset + 6);
        v |= consume_bit() << (byte_offset + 7);
    }

    // end of transmission appears to be four bits
    //   - ~40,000us
    //   - ~2,000us
    //   - ~96000us
    //   - ~2,000us
    consume_bit();
    consume_bit();
    consume_bit();
    consume_bit();
    printk("Finished reverse engineering: v=%x\n", v);
    return v;
}
