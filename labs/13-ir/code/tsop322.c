#include "cycle-count.h"
#include "rpi.h"
#include "tsop322.h"

static const button_t buttons[] = {
    { .name = "MUTE", .hex_encoding = 0x6996bb42 },
    { .name = "POWER", .hex_encoding = 0x6d92bb42 },
    { .name = "MENU", .hex_encoding = 0x2bd4bb42 },
    { .name = "EXIT", .hex_encoding = 0x748bbb42 },
    { .name = "UP ARROW", .hex_encoding = 0x37c8bb42 },
    { .name = "RIGHT ARROW", .hex_encoding = 0x35cabb42 },
    { .name = "DOWN ARROW", .hex_encoding = 0x36c9bb42 },
    { .name = "LEFT ARROW", .hex_encoding = 0x34cbbb42 },
    { .name = "ENTER", .hex_encoding = 0xffbb42 },
    { .name = "INFO", .hex_encoding = 0x1febb42 },
    { .name = "SIGNAL", .hex_encoding = 0x6798bb42 },
    { .name = "VOLUME_UP", .hex_encoding = 0x659abb42 },
    { .name = "VOLUME_DOWN", .hex_encoding = 0x639cbb42 },
    { .name = "CHANNEL_UP", .hex_encoding = 0x649bbb42 },
    { .name = "CHANNEL_DOWN", .hex_encoding = 0x609fbb42 },
    { .name = "ONE", .hex_encoding = 0x7e81bb42 },
    { .name = "TWO", .hex_encoding = 0x7d82bb42 },
    { .name = "THREE", .hex_encoding = 0x7c83bb42 },
    { .name = "FOUR", .hex_encoding = 0x7b84bb42 },
    { .name = "FIVE", .hex_encoding = 0x7a85bb42 },
    { .name = "SIX", .hex_encoding = 0x7986bb42 },
    { .name = "SEVEN", .hex_encoding = 0x7887bb42 },
    { .name = "EIGHT", .hex_encoding = 0x7788bb42 },
    { .name = "NINE", .hex_encoding = 0x7689bb42 },
    { .name = "ZERO", .hex_encoding = 0x758abb42 },
    { .name = "CC", .hex_encoding = 0x738cbb42 },
    { .name = "DOT", .hex_encoding = 0x718ebb42 }
};

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

enum {
    eps = 200,
    start = 4500,
    zero = 500,
    one = 1600
};

static inline int abs(int x) { return x < 0 ? -x : x; }

static int within(unsigned t, unsigned bound, unsigned eps) {
    return abs(t-bound) < eps;
}

// return 0 if e is within eps of lb, 1 if its within eps of ub
static int pick(unsigned duration) {
    if (within(duration, zero, eps))
        return 0;
    if (within(duration, one, eps))
        return 1;
    panic("Invalid time: <%d> expected %d or %d\n", duration, zero, one);
}

unsigned tsop322_reverse_engineer(verbosity_t verbose, button_t *button) {
    if (verbose)
        printk("Reverse engineering a button press\n");
    unsigned v = 0; 
    
    unsigned duration;
    do {
        duration = consume_bit();
    } while (duration < start-eps || start+eps < duration);

    for (unsigned i = 0; i < 32; i++)
        v |= (pick(consume_bit()) << i);
    
    if (verbose) 
        printk("Finished reverse engineering: v=%x\n", v);

    for (unsigned i = 0; i < sizeof(buttons)/sizeof(*buttons); i++) {
        if (buttons[i].hex_encoding == v) {
            memcpy(button, &buttons[i], sizeof(button_t));
            return v;
        }
    }
    panic("Hex encoding %x did not match any known buttons\n");
}
