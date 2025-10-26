#ifndef BUTTON_H
#define BUTTON_H

#include "pico/stdlib.h"

typedef struct button_t {
  uint8_t pin;
  bool state;
  void (*onchange)(struct button_t *button);
} button_t;

long long int handle_button_alarm(long int, void *);
void handle_button_interrupt(void *);
button_t *create_button(int , void (*)(button_t *));

#endif
