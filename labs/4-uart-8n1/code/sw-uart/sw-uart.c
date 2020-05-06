#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/sw-uart.h"
#include "../fast-gpio.h"

#define BAUD 115200
#define CYCLES_PER_BIT (700*10000*1000UL)/BAUD

struct my_sw_uart {
    char tx,rx;
    unsigned baud;
    unsigned cycle_per_bit;
};
typedef struct my_sw_uart my_sw_uart_t;

int sw_uart_get8(my_sw_uart_t *uart) {
    // Assume that `cycle_cnt_init` has already been called.

    unsigned rx = uart->rx;
    int b = 0;
    
    // Wait for start bit.
    while (fast_gpio_read(rx) != 0);
    delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT + CYCLES_PER_BIT/2);

    for (unsigned i = 0; i < 8; i++) {
        b |= (fast_gpio_read(rx) << i);
        delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);
    }

    // Wait for stop bit.
    while (fast_gpio_read(rx) != 1);

    return b;
}


int sw_uart_put8(my_sw_uart_t *uart, uint8_t b) {
    unsigned tx = uart->tx;

    // Write start bit for `CYCLES_PER_BIT` cycles.
    fast_gpio_write(0, tx);
    delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);

    for (unsigned i = 0; i < 8; i++) {
        fast_gpio_write((b >> i) & 1, tx);
        delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);
    }

    // Write stop bit for `CYCLES_PER_BIT` cycles.
    fast_gpio_write(1, tx);
    delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);

    return 0;
}


int sw_uart_get32(my_sw_uart_t *uart) {
    return sw_uart_get8(uart) 
        | sw_uart_get8(uart) << 8
        | sw_uart_get8(uart) << 16
        | sw_uart_get8(uart) << 24;
}


int sw_uart_put32(my_sw_uart_t *uart, uint32_t w) {
    for (unsigned i = 0; i < 32; i += 8)
        sw_uart_put8(uart, (w >> i) & 0xff);
    
    return 0;
}

#define WORD 800

// 5 second timeout
unsigned timeout = 5 * 1000 * 1000;
static void server(unsigned tx, unsigned rx, unsigned n) {
    gpio_set_input(rx);
    gpio_set_output(tx);
    printk("am a server\n");
    gpio_write(tx,1);

    unsigned bytes[n];
    my_sw_uart_t server_uart = { 
        .tx = tx,
        .rx = rx,
        .baud = BAUD,
        .cycle_per_bit = CYCLES_PER_BIT
    };
    // Wait for client to acknowledge us.
    unsigned expected = 1;
    if(!wait_until_usec(rx, expected, timeout)) 
        panic("timeout waiting for %d!\n", expected);

    for(unsigned i = 0; i < n; i++) {
        sw_uart_put32(&server_uart, i);
        bytes[i] = sw_uart_get32(&server_uart);
    }
    for (unsigned i = 0; i < n; i++) 
        printk("%u: server read %u\n", i, bytes[i]);
    printk("server done\n");
}

static void client(unsigned tx, unsigned rx, unsigned n) {
    gpio_set_input(rx);
    gpio_set_output(tx);
    printk("am a client\n");

    unsigned bytes[n];
    my_sw_uart_t client_uart = { 
        .tx = tx,
        .rx = rx,
        .baud = BAUD,
        .cycle_per_bit = CYCLES_PER_BIT
    };

    fast_gpio_write(1, client_uart.tx);
    for (unsigned i = 0; i < n; i++) {
        bytes[i] = sw_uart_get32(&client_uart);
        sw_uart_put32(&client_uart, i);
    }
    for (unsigned i = 0; i < n; i++)
        printk("%u: client read %u\n", i, bytes[i]);

    printk("client done\n");
}


static void ping_pong(unsigned tx, unsigned rx, unsigned n) {
    if(!gpio_read(rx))
        server(tx,rx,n);
    else
        client(tx,rx,n);
}


void notmain(void) {
    enable_cache();
    cycle_cnt_init();
    ping_pong(21,20,4096);
    clean_reboot();
}
