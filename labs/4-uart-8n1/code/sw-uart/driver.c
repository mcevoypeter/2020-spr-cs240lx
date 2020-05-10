#include "rpi.h"
#include "cs140e-src/cycle-count.h"
#include "cs140e-src/sw-uart.h"
#include "../fast-gpio.h"
#include "my-sw-uart.h"

#define WORD 1
#define PARALLEL 1

static const unsigned txs[] = {
    16,
    17,
    18,
    19
};
static const unsigned rxs[] = {
    20,
    21,
    22,
    23
};

#if WORD
static const unsigned n = 4096;
int (*my_sw_uart_get)(my_sw_uart_t *uart) = my_sw_uart_get32;
void (*my_sw_uart_put)(my_sw_uart_t *uart, unsigned w) = my_sw_uart_put32;
#else
static const unsigned n = 256;
int (*my_sw_uart_get)(my_sw_uart_t *uart) = my_sw_uart_get8;
void (*my_sw_uart_put)(my_sw_uart_t *uart, unsigned char b) = my_sw_uart_put8;
#endif

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

    unsigned start = cycle_cnt_read();
    for(unsigned i = 0; i < n; i++) {
        my_sw_uart_put(&server_uart, i);
        bytes[i] = my_sw_uart_get(&server_uart);
    }
    unsigned end = cycle_cnt_read();
    for (unsigned i = 0; i < n; i++) {
        assert(i == bytes[i]);
        printk("%u: server read %u\n", i, bytes[i]);
    }

    printk("server done\n");
    printk("exchanged %u %s in %s\n", n, WORD ? "words" : "bytes", 
            PARALLEL ? "parallel" : "series");
    printk("cycles consumed = %u\n", end-start);
}

static void client() {
    printk("am a client\n");
    my_sw_uart_t client_uart = my_sw_uart_init(txs, tx_cnt, rxs, rx_cnt, type);

    // Acknowledge server.
    fast_gpio_write(client_uart.txs[0], 1);

    unsigned bytes[n];

    unsigned start = cycle_cnt_read();
    for (unsigned i = 0; i < n; i++) {
        bytes[i] = my_sw_uart_get(&client_uart);
        my_sw_uart_put(&client_uart, i);
    }
    unsigned end = cycle_cnt_read();
    for (unsigned i = 0; i < n; i++) {
        assert(i == bytes[i]);
        printk("%u: client read %u\n", i, bytes[i]);
    }

    printk("client done\n");
    printk("exchanged %u %s in %s\n", n, WORD ? "words" : "bytes", 
            PARALLEL ? "parallel" : "series");
    printk("cycles consumed = %u\n", end-start);
}

void notmain(void) {
    enable_cache();
    cycle_cnt_init();
    printk("rxs[0] = %u\n", rxs[0]);
    if(!gpio_read(rxs[0]))
        server();
    else
        client();
    clean_reboot();
}
