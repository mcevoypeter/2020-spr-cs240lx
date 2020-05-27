#ifndef TSOP_322_H
#define TSOP_322_H

typedef enum { 
    quiet = 0, 
    verbose 
} verbosity_t;

typedef struct {
    unsigned hex_encoding;
    const char *name;
} button_t;

void tsop322_init(unsigned data_pin);

void tsop322_poll(void);

unsigned tsop322_reverse_engineer(verbosity_t verbose);

void tsop322_get_button_press(button_t *button);


#endif
