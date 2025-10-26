#ifndef PICO_GPIO_INTERRUPT_H
#define PICO_GPIO_INTERRUPT_H

#include "pico/stdlib.h"

typedef void (*handler)(void *argument);

typedef struct {
  void * argument;
  handler fn;
} closure_t;


void handle_interupt(uint, uint32_t);
void listen(uint , int , handler , void *);

#endif
