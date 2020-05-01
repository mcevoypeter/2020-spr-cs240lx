// simple driver for hc-sr04
#include "rpi.h"
#include "hc-sr04.h"

void notmain(void) {
    // use this timeout(in usec) so everyone is consistent.
    unsigned timeout = 55000;

    uart_init();

	printk("starting sonar!\n");
    hc_sr04_t h = hc_sr04_init(20, 21);
	printk("sonar ready!\n");

    for(int dist, i = 0; i < 10; i++) {
        // read until no timeout.
        while((dist = hc_sr04_get_distance(&h, timeout)) < 0)
            ;
        printk("distance = %d inches\n", dist);
        // wait a second
        delay_ms(1000);
    }
	printk("stopping sonar !\n");
    clean_reboot();
}
