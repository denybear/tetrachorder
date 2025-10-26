#ifndef ENCODER_BUTTON_H
#define ENCODER_BUTTON_H

#include "pico/stdlib.h"

typedef struct rotary_encoder_t {
  int pin_a, pin_b;
  int state;
  long int position;
  int direction;
  void (*onchange)(struct rotary_encoder_t *encoder);
} rotary_encoder_t;

typedef struct button_t {
  int pin;
  bool state;
  void (*onchange)(struct button_t *button);
} button_t;

void handle_rotation(void *);
rotary_encoder_t *create_encoder(int, int, void (*)(rotary_encoder_t *));
long long int handle_button_alarm(long int, void *);
void handle_button_interrupt(void *);
button_t *create_button(int , void (*)(button_t *));

#endif