#ifndef __MY_SW_UART_H__
#define __MY_SW_UART_H__

#define MAX_PINS 8

typedef enum {
    parallel = 0,
    serial
} uart_type_t;

typedef struct {
    unsigned txs[MAX_PINS], rxs[MAX_PINS];
    unsigned tx_cnt, rx_cnt;
    unsigned baud;
    unsigned cycle_per_bit;
} my_sw_uart_t;


my_sw_uart_t my_sw_uart_init(const unsigned *txs, const unsigned tx_cnt, 
        const unsigned *rxs, const unsigned rx_cnt, uart_type_t type);

int my_sw_uart_get8(my_sw_uart_t *uart);
void my_sw_uart_put8(my_sw_uart_t *uart, unsigned char b);
int my_sw_uart_get32(my_sw_uart_t *uart);
void my_sw_uart_put32(my_sw_uart_t *uart, unsigned w);

#endif
