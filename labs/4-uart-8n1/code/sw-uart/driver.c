#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/sw-uart.h"
#include "../fast-gpio.h"
#include "my-sw-uart.h"

#define PARALLEL 0

static const unsigned txs[] = {
    26,
    21
};
static const unsigned rxs[] = {
    19,
    20
};
static const unsigned n = 4096;

#if PARALLEL
static const unsigned tx_cnt = sizeof(txs)/sizeof(txs[0]);
static const unsigned rx_cnt = sizeof(rxs)/sizeof(rxs[0]);
static const uart_type_t type = parallel;
#else
static const unsigned tx_cnt = 1;
static const unsigned rx_cnt = 1;
static const uart_type_t type = serial;
#endif

// 5 second timeout
unsigned timeout = 5 * 1000 * 1000;
static void server() {
    printk("am a server\n");
    my_sw_uart_t server_uart = my_sw_uart_init(txs, tx_cnt, rxs, rx_cnt, type);
    fast_gpio_write(server_uart.txs[0], 1);

    // Wait for client acknowledgement.
    unsigned expected = 1;
    if(!wait_until_usec(server_uart.rxs[0], expected, timeout)) 
        panic("timeout waiting for %d!\n", expected);

    unsigned bytes[n];

    for(unsigned i = 0; i < n; i++) {
        my_sw_uart_put32(&server_uart, i);
        bytes[i] = my_sw_uart_get32(&server_uart);
    }
    for (unsigned i = 0; i < n; i++) 
        printk("%u: server read %u\n", i, bytes[i]);
    printk("server done\n");
}

static void client() {
    printk("am a client\n");
    my_sw_uart_t client_uart = my_sw_uart_init(txs, tx_cnt, rxs, rx_cnt, type);

    // Acknowledge server.
    fast_gpio_write(client_uart.txs[0], 1);

    unsigned bytes[n];

    for (unsigned i = 0; i < n; i++) {
        bytes[i] = my_sw_uart_get32(&client_uart);
        my_sw_uart_put32(&client_uart, i);
    }
    for (unsigned i = 0; i < n; i++)
        printk("%u: client read %u\n", i, bytes[i]);

    printk("client done\n");
}

static void ping_pong() {
    printk("rxs[0] = %u\n", rxs[0]);
    if(!gpio_read(rxs[0]))
        server();
    else
        client();
}


void notmain(void) {
    enable_cache();
    cycle_cnt_init();
    ping_pong();
    clean_reboot();
}
