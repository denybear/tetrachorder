#include <stdlib.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "globals.h"
#include "encoder_button.h"

typedef void (*handler)(void *argument);

typedef struct {
  void * argument;
  handler fn;
} closure_t;

static closure_t handlers[28] = {NULL};

static void handle_interupt(uint gpio, uint32_t events) {
  closure_t handler = handlers[gpio];
  handler.fn(handler.argument);
}

static void listen(uint pin, int condition, handler fn, void *arg) {
  gpio_init(pin);
  gpio_pull_up(pin);
  gpio_set_irq_enabled_with_callback(pin, condition, true, (gpio_irq_callback_t) handle_interupt);
  closure_t *handler = malloc(sizeof(closure_t));
  handler->argument = arg;
  handler->fn = fn;
  handlers[pin] = *handler;
}

void handle_rotation(void *pointer) {
  rotary_encoder_t *encoder = (rotary_encoder_t *)pointer;
  int state = gpio_get(encoder->pin_a)<<1 | gpio_get(encoder->pin_b);
  encoder->direction = 0;
  switch ((encoder->state)<<2 | state) {
    case 0b1110:
      encoder->direction = -1;
      encoder->position++;
      encoder->onchange(encoder);
      break;
    case 0b0001: case 0b0111: case 0b1000:
      encoder->position++;
      encoder->onchange(encoder);
      break;
    case 0b1101:
      encoder->direction = 1;
      encoder->position--;
      encoder->onchange(encoder);
      break;
    case 0b0100: case 0b1011: case 0b0010:
      encoder->position--;
      encoder->onchange(encoder);
      break;
  }
  encoder->state = state;
}

rotary_encoder_t *create_encoder(int pin_a, int pin_b, void (*onchange)(rotary_encoder_t *encoder)) {
  rotary_encoder_t *encoder = (rotary_encoder_t *)malloc(sizeof(rotary_encoder_t));
  encoder->pin_a = pin_a;
  encoder->pin_b = pin_b;
  encoder->state = (gpio_get(pin_a)<<1 | gpio_get(pin_b));
  encoder->position = 0;
  encoder->direction = 0;
  encoder->onchange = onchange;
  listen(pin_a, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, handle_rotation, encoder);
  listen(pin_b, GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL, handle_rotation, encoder);
  return encoder;
}

long long int handle_button_alarm(long int a, void *p) {
  button_t *b = (button_t *)(p);
  bool state = gpio_get(b->pin);
  if (state != b->state) {
    b->state = state;
    b->onchange(b);
  }
  return 0;
}

void handle_button_interrupt(void *p) {
  button_t *b = (button_t *)(p);
  bool state = gpio_get(b->pin);
  add_alarm_in_us(200, handle_button_alarm, b, true);
}

button_t * create_button(int pin, void (*onchange)(button_t *)) {
  button_t *b = (button_t *)(malloc(sizeof(button_t)));
  listen(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,handle_button_interrupt, b);
  b->pin = pin;
  b->onchange = onchange;
  b->state = gpio_get(pin);
  return b;
}

