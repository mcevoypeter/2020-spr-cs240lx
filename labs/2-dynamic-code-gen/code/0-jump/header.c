#include "rpi.h"
#include "../unix-side/armv6-insts.h"

void notmain(void) {
    // print out the string in the header.

    // figure out where it points to!
    const char *header_string = (const char *)0x8014;

    assert(header_string);
    printk("<%s>\n", header_string);
    printk("success!\n");
    clean_reboot();
}
