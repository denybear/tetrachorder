/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"


  // The duration a note is played is determined by the amount of attack,
  // decay, and release, combined with the length of the note as defined by
  // the user.
  //
  // - Attack:  number of milliseconds it takes for a note to hit full volume
  // - Decay:   number of milliseconds it takes for a note to settle to sustain volume
  // - Sustain: percentage of full volume that the note sustains at, and duration in ms
  // - Release: number of milliseconds it takes for a note to reduce to zero volume after it has ended
  //
  // Attack (750ms) - Decay (500ms) -------- Sustain ----- Release (250ms)
  //
  //                +         +                                  +    +
  //                |         |                                  |    |
  //                |         |                                  |    |
  //                |         |                                  |    |
  //                v         v                                  v    v
  // 0ms               1000ms              2000ms              3000ms              4000ms
  //
  // |              XXXX |                   |                   |                   |
  // |             X    X|XX                 |                   |                   |
  // |            X      |  XXX              |                   |                   |
  // |           X       |     XXXXXXXXXXXXXX|XXXXXXXXXXXXXXXXXXX|                   |
  // |          X        |                   |                   |X                  |
  // |         X         |                   |                   |X                  |
  // |        X          |                   |                   | X                 |
  // |       X           |                   |                   | X                 |
  // |      X            |                   |                   |  X                |
  // |     X             |                   |                   |  X                |
  // |    X              |                   |                   |   X               |
  // |   X               |                   |                   |   X               |
  // |  X +    +    +    |    +    +    +    |    +    +    +    |    +    +    +    |    +
  // | X  |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
  // |X   |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
  // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+--->



#define CHANNEL_COUNT 16          // 4 notes + bass + 9th + 11th = 7 channels * 2 = 14; let's make it 16

#define PI 3.14159265358979323846f

typedef enum {
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
    ADSR_OFF
} ADSRPhase;

typedef struct {
    uint8_t waveforms;                // # of waveform
    uint16_t frequency;               // Frequency of the voice (Hz)
    uint16_t volume;                  // Channel volume
    uint8_t midi_note;                // MIDI note played on the channel

    uint16_t attack_ms;               // Attack period
    uint16_t decay_ms;                // Decay period
    uint16_t sustain;                 // Sustain volume
    uint16_t sustain_ms;              // Sustain period
    uint16_t release_ms;              // Release period

    uint32_t waveform_offset;         // Voice offset (Q8)

    int32_t filter_last_sample;       // Last sample for filter
    bool filter_enable;               // Filter status
    uint16_t filter_cutoff_frequency; // Cutoff frequency for filter

    uint32_t adsr_frame;              // Number of frames in current ADSR phase
    uint32_t adsr_end_frame;          // Frame target for ADSR change
    uint32_t adsr;                    // Current ADSR value
    int32_t adsr_step;                // ADSR step value
    ADSRPhase adsr_phase;             // Current ADSR phase
} AudioChannel;

void set_audio_rate_and_volume (uint32_t, uint16_t);
int16_t get_audio_frame(void);
bool is_audio_playing(void);

void retrigger_attack(AudioChannel* channel);
void trigger_attack(AudioChannel* channel);
void trigger_decay(AudioChannel* channel);
void trigger_sustain(AudioChannel* channel);
void trigger_release(AudioChannel* channel);
void off(AudioChannel* channel);

#endif // SYNTH_H
