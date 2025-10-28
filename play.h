#ifndef PLAY_H
#define PLAY_H

#include "pico/stdlib.h"

#define NB_INSTRUMENTS 11

void update_playback (int, uint8_t);
void stop_playback (int);
void reset_playback (int);
void reset_playback_all ();
bool load_instrument(int, int);
void song_task();
void instrument_task(int);
void core1_main();

#endif