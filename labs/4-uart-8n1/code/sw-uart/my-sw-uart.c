#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/sw-uart.h"
#include "my-sw-uart.h"
#include "../fast-gpio.h"

#define BAUD 115200
#define CYCLES_PER_BIT (700*10000*1000UL)/BAUD
#define MAX_PINS 8


my_sw_uart_t my_sw_uart_init(const unsigned *txs, const unsigned tx_cnt, 
        const unsigned *rxs, const unsigned rx_cnt) {

    // Is it okay to call this multiple times?
    cycle_cnt_init(); 
    assert(0);    
}


int my_sw_uart_serial_get8(my_sw_uart_t *uart) {
    // Assume that `cycle_cnt_init` has already been called.

    unsigned rx = uart->rxs[0];
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

void my_sw_uart_serial_put8(my_sw_uart_t *uart, unsigned char b) {
    // Assume that `cycle_cnt_init` has already been called.
    
    unsigned tx = uart->txs[0];

    // Write start bit for `CYCLES_PER_BIT` cycles.
    fast_gpio_write(tx, 0);
    delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);

    for (unsigned i = 0; i < 8; i++) {
        fast_gpio_write(tx, (b >> i) & 1);
        delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);
    }

    // Write stop bit for `CYCLES_PER_BIT` cycles.
    fast_gpio_write(tx, 1);
    delay_ncycles(cycle_cnt_read(), CYCLES_PER_BIT);
}


int my_sw_uart_serial_get32(my_sw_uart_t *uart) {
    return my_sw_uart_serial_get8(uart) 
        | my_sw_uart_serial_get8(uart) << 8
        | my_sw_uart_serial_get8(uart) << 16
        | my_sw_uart_serial_get8(uart) << 24;
}


void my_sw_uart_serial_put32(my_sw_uart_t *uart, unsigned w) {
    for (unsigned i = 0; i < 32; i += 8)
        my_sw_uart_serial_put8(uart, (w >> i) & 0xff);
}


int my_sw_uart_parallel_get8(my_sw_uart_t *uart) {
    // TODO
    return -1;
}


void sw_uart_parallel_put8(my_sw_uart_t *uart, unsigned char b) {
    // TODO
}


int sw_uart_parallel_get32(my_sw_uart_t *uart) {
    // TODO
    return -1;
}


void sw_uart_parallel_put32(my_sw_uart_t *uart, unsigned w) {
    // TODO
}
