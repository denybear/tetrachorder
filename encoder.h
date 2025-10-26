#ifndef ENCODER_H
#define ENCODER_H

#include "pico/stdlib.h"

typedef struct rotary_encoder_t {
  int pin_a, pin_b;
  int state;
  long int position;
  int direction;
  void (*onchange)(struct rotary_encoder_t *encoder);
} rotary_encoder_t;

void handle_rotation(void *);
rotary_encoder_t *create_encoder(int, int, void (*)(rotary_encoder_t *));

#endif