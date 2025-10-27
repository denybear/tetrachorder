/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"

#include "audio.h"

struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .sample_freq = SAMPLE_RATE,
        .channel_count = 1,
    };

    static struct audio_buffer_format producer_format = {
        .format = &audio_format,
        .sample_stride = 2
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3, SAMPLES_PER_BUFFER);
    bool __unused ok;
    const struct audio_format *output_format;

    struct audio_i2s_config config = {
        .data_pin = PICO_AUDIO_I2S_DATA,
        .clock_pin_base = PICO_AUDIO_I2S_BCLK,
        .dma_channel = 0,
        .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);

    return producer_pool;
}


void update_buffer(struct audio_buffer_pool *ap, buffer_callback cb) {

    struct audio_buffer *buffer = take_audio_buffer(ap, true);
    int16_t *samples = (int16_t *) buffer->buffer->bytes;
    for (uint32_t i = 0; i < buffer->max_sample_count; i++) {
        samples[i] = cb();
    }
    buffer->sample_count = buffer->max_sample_count;
    give_audio_buffer(ap, buffer);
}
