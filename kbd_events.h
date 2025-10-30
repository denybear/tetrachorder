#ifndef KBD_EVENTS_H
#define KBD_EVENTS_H

#include "pico/stdlib.h"
#include "keypad.h"


bool build_full_chord (uint8_t , void *);
uint8_t parse_keyboard (void *, KeypadMatrix *);

#endif
