#include <stdlib.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "globals.h"
#include "gpio-interrupt.h"

static closure_t handlers[28] = {NULL};

void handle_interupt(uint gpio, uint32_t events) {
  closure_t handler = handlers[gpio];
  handler.fn(handler.argument);
}

void listen(uint pin, int condition, handler fn, void *arg) {
  gpio_init(pin);
  gpio_pull_up(pin);
  gpio_set_irq_enabled_with_callback(pin, condition, true, (gpio_irq_callback_t) handle_interupt);
  closure_t *handler = malloc(sizeof(closure_t));
  handler->argument = arg;
  handler->fn = fn;
  handlers[pin] = *handler;
}
