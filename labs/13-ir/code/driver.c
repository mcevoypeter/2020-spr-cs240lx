#include "rpi.h"
#include "tsop322.h"


void notmain(void) {
#   define DATA 21
    tsop322_init(DATA);
    while (1) {
        button_t button;
        tsop322_get_button_press(&button);
        printk("Button = %s\n", button.name);
        if (strcmp(button.name, "POWER") == 0)
            break;
    }
}
