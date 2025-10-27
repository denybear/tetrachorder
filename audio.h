/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"


#define PICO_AUDIO_I2S_DATA		9
#define PICO_AUDIO_I2S_BCLK		10
#define PICO_AUDIO_I2S_LRCLK	11
#define SAMPLES_PER_BUFFER		256
#define SAMPLE_RATE				44100
#define VOLUME					0xFFFF

typedef int16_t (*buffer_callback)(void);

struct audio_buffer_pool *init_audio();
void update_buffer(struct audio_buffer_pool *ap, buffer_callback cb);

#endif // AUDIO_H
