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

    int b = 0;

    unsigned cum_cycles = CYCLES_PER_BIT + CYCLES_PER_BIT/2;
    unsigned start = cycle_cnt_read();

    // Wait for start bit.
    while (fast_gpio_read(uart->rx) != 0);
    
    // Delay `CYCLES_PER_BIT + CYCLES_PER_BIT/2` cycles.
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 1;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 2;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 3;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 4;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 5;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 6;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    b |= fast_gpio_read(uart->rx) << 7;
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    // Wait for stop bit.
    /*while (fast_gpio_read(uart->rx) != 1);*/

    return b;
}


int sw_uart_put8(my_sw_uart_t *uart, uint8_t b) {
    // Assume that `cycle_cnt_init` has already been called.

    unsigned cum_cycles = CYCLES_PER_BIT;
    unsigned tx = uart->tx;
    unsigned start = cycle_cnt_read();

    // Write start bit for `CYCLES_PER_BIT` cycles.
    fast_gpio_write(0, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write(b & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 1) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 2) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 3) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles); 
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 4) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 5) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 6) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    fast_gpio_write((b >> 7) & 1, tx);
    while (cycle_cnt_read() - start < cum_cycles);
    cum_cycles += CYCLES_PER_BIT;

    // Write stop bit for `CYCLES_PER_BIT` cycles.
    fast_gpio_write(1, tx);
    while (cycle_cnt_read() - start < cum_cycles);

    unsigned end = cycle_cnt_read();
    return 0;
}

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
        sw_uart_put8(&server_uart, i);
        bytes[i] = sw_uart_get8(&server_uart);
#if 0
        sw_uart_put8(&server_uart, i & 0xff);
        sw_uart_put8(&server_uart, (i >> 8) & 0xff);
        sw_uart_put8(&server_uart, (i >> 16) & 0xff);
        sw_uart_put8(&server_uart, (i >> 24) & 0xff);
        bytes[i] = sw_uart_get8(&server_uart);
        bytes[i] |= sw_uart_get8(&server_uart) << 8;
        bytes[i] |= sw_uart_get8(&server_uart) << 16;
        bytes[i] |= sw_uart_get8(&server_uart) << 24;
#endif
    }
    for (unsigned i = 0; i < n; i++) {
        printk("%u: server read %u\n", i, bytes[i]);
    }
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
        bytes[i] = sw_uart_get8(&client_uart);
        sw_uart_put8(&client_uart, i);
#if 0
        bytes[i] = sw_uart_get8(&client_uart);
        bytes[i] |= sw_uart_get8(&client_uart) << 8;
        bytes[i] |= sw_uart_get8(&client_uart) << 16;
        bytes[i] |= sw_uart_get8(&client_uart) << 24;
        sw_uart_put8(&client_uart, i & 0xff);
        sw_uart_put8(&client_uart, (i >> 8) & 0xff);
        sw_uart_put8(&client_uart, (i >> 16) & 0xff);
        sw_uart_put8(&client_uart, (i >> 24) & 0xff);
#endif
    }
    for (unsigned i = 0; i < n; i++)
        printk("%u: client read %u\n", i, bytes[i]);

    printk("client done\n");
}


// send N samples at <ncycle> cycles each in a simple way.
static void ping_pong(unsigned tx, unsigned rx, unsigned n) {
    if(!gpio_read(rx))
        server(tx,rx,n);
    else
        client(tx,rx,n);
}


void notmain(void) {
    enable_cache();
    cycle_cnt_init();
    ping_pong(21,20,256);
    clean_reboot();
}
