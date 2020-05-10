#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/sw-uart.h"
#include "my-sw-uart.h"
#include "../fast-gpio.h"

#define BAUD 115200
#define CYCLES_PER_BIT (700*10000*1000UL)/BAUD
#define MAX_PINS 8

static uart_type_t uart_type;
my_sw_uart_t my_sw_uart_init(const unsigned *txs, const unsigned tx_cnt, 
        const unsigned *rxs, const unsigned rx_cnt, uart_type_t type) {

    // Assume that `cycle_cnt_init` has already been called.
    uart_type = type;

    if (uart_type == serial) 
        assert(tx_cnt == 1 && rx_cnt == 1);
    else if (uart_type == parallel)
        assert(tx_cnt < MAX_PINS && rx_cnt < MAX_PINS);
    else
        panic("invalid uart_type_t type\n");
    
    my_sw_uart_t uart = { .baud = BAUD, .cycle_per_bit = CYCLES_PER_BIT };
    for (unsigned i = 0; i < tx_cnt; i++) {
        uart.txs[i] = txs[i];
        gpio_set_output(uart.txs[i]);
    }
    for (unsigned i = 0; i < rx_cnt; i++) {
        uart.rxs[i] = rxs[i];
        gpio_set_input(uart.rxs[i]);
    }
    return uart;
}


static int my_sw_uart_serial_get8(my_sw_uart_t *uart) {
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

static int my_sw_uart_parallel_get8(my_sw_uart_t *uart) {
    // TODO
    return -1;
}

int my_sw_uart_get8(my_sw_uart_t *uart) {
    if (uart_type == serial)
        return my_sw_uart_serial_get8(uart);
    else if (uart_type == parallel)
        return my_sw_uart_parallel_get8(uart);
    else
        return -1;
}


static void my_sw_uart_serial_put8(my_sw_uart_t *uart, unsigned char b) {
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

static void my_sw_uart_parallel_put8(my_sw_uart_t *uart, unsigned char b) {
    // TODO
}

void my_sw_uart_put8(my_sw_uart_t *uart, unsigned char b) {
    if (uart_type == serial)
        my_sw_uart_serial_put8(uart, b);
    else if (uart_type == parallel)
        my_sw_uart_parallel_put8(uart, b);
}


static int my_sw_uart_serial_get32(my_sw_uart_t *uart) {
    return my_sw_uart_serial_get8(uart) 
        | my_sw_uart_serial_get8(uart) << 8
        | my_sw_uart_serial_get8(uart) << 16
        | my_sw_uart_serial_get8(uart) << 24;
}

static int my_sw_uart_parallel_get32(my_sw_uart_t *uart) {
    // TODO
    return -1;
}

int my_sw_uart_get32(my_sw_uart_t *uart) {
    if (uart_type == serial)
        return my_sw_uart_serial_get32(uart);
    else if (uart_type == parallel)
        return my_sw_uart_parallel_get32(uart);
    else
        return -1;
}


static void my_sw_uart_serial_put32(my_sw_uart_t *uart, unsigned w) {
    for (unsigned i = 0; i < 32; i += 8)
        my_sw_uart_serial_put8(uart, (w >> i) & 0xff);
}

static void my_sw_uart_parallel_put32(my_sw_uart_t *uart, unsigned w) {
    // TODO
}

void my_sw_uart_put32(my_sw_uart_t *uart, unsigned w) {
    if (uart_type == serial)
        my_sw_uart_serial_put32(uart, w);
    else if (uart_type == parallel)
        my_sw_uart_parallel_put32(uart, w);
}
