#include "pico/stdlib.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "globals.h"
#include "audio.h"
#include "synth.h"
#include "waveforms.h"

uint32_t prng_xorshift_state = 0x32B71700;

uint32_t prng_xorshift_next() {
    uint32_t x = prng_xorshift_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_xorshift_state = x;
    return x;
}

int32_t prng_normal() {
    // Rough approximation of a normal distribution
    uint32_t r0 = prng_xorshift_next();
    uint32_t r1 = prng_xorshift_next();
    uint32_t n = ((r0 & 0xffff) + (r1 & 0xffff) + (r0 >> 16) + (r1 >> 16)) / 2;
    return n - 0xffff;
}

uint32_t sample_rate;   // Sample rate definition
uint16_t volume;        // Global volume

void set_audio_rate_and_volume (uint32_t rate, uint16_t vol) {
    sample_rate = rate;
    volume = vol;
}

bool is_audio_playing() {
    if (volume == 0) {
        return false;
    }

    for (int c = 0; c < CHANNEL_COUNT; c++) {
        if (channels[c].volume > 0 && channels[c].adsr_phase != ADSR_OFF) {
            return true;
        }
    }

    return false;
}

int16_t get_audio_frame() {
    int32_t sample = 0; // Used to combine channel output
	int index = 0;

    for (int c = 0; c < CHANNEL_COUNT; c++) {
        AudioChannel* channel = &channels[c];

        // Increment the waveform position counter
        // we do over-sampling, ie. instead of 256 samples per waveform, we consider to have 256 >> 8 = 65536 (16-bits)
        channel->waveform_offset += ((channel->frequency * SAMPLES_PER_BUFFER) << 8) / sample_rate;

        if (channel->adsr_phase == ADSR_OFF) {
            continue;
        }

        // Check ADSR phase transitions
        if (channel->adsr_frame >= channel->adsr_end_frame) {
            switch (channel->adsr_phase) {
                case ADSR_ATTACK:
                    trigger_decay(channel);
                    break;
                case ADSR_DECAY:
                    trigger_sustain(channel);
                    break;
                case ADSR_SUSTAIN:
                    trigger_release(channel);
                    break;
                case ADSR_RELEASE:
                    off(channel);
                    break;
                default:
                    break;
            }
        }

        channel->adsr += channel->adsr_step;
        channel->adsr_frame++;
        channel->waveform_offset &= 0xffff;

        // Check if any waveforms are active for this channel
        if (channel->active) {
            int32_t channel_sample;

            // check if channel frequency is 0; if so, then sample shall be 0
            if (channel->frequency == 0) channel_sample =0;
            // if channel frequency is not 0, then process sample
            else {
				// get the value of sample from the waveform
				// select the waveform from value of midi_note
				// there are different waveforms so that higher notes get let's harmonics than lower range notes (waveforms are simpler)
				// this is a design characteristic of Korg DW8000 synthetizer
				if ((channel->midi_note <= 35)) index = 0;										// notes between C-2 and B0
				if ((channel->midi_note >= 36) && (channel->midi_note <= 47)) index = 1;		// notes between C1 and B1
				if ((channel->midi_note >= 48) && (channel->midi_note <= 59)) index = 2;		// notes between C2 and B2
				if ((channel->midi_note >= 60) && (channel->midi_note <= 71)) index = 3;		// notes between C3 and B3
				if ((channel->midi_note >= 72) && (channel->midi_note <= 83)) index = 4;		// notes between C4 and B4
				if ((channel->midi_note >= 84) && (channel->midi_note <= 95)) index = 5;		// notes between C5 and B5
				if ((channel->midi_note >= 96) && (channel->midi_note <= 107)) index = 6;		// notes between C6 and B6
				if ((channel->midi_note >= 108)) index = 7;										// notes between C7 and G8

				channel_sample = (int32_t)(waveforms [channel->waveforms][index][(channel->waveform_offset) >> 8]);		// get sample from sample array                  
            }

            // Scale by ADSR and volume
            channel_sample = ((int64_t)(channel_sample) * (int32_t)(channel->adsr >> 8)) >> 16;
            channel_sample = ((int64_t)(channel_sample) * (int32_t)(channel->volume)) >> 16;

            // Combine channel sample into the final sample
            sample += channel_sample;
        }
    }
    sample = ((int64_t)(sample) * (int32_t)(volume)) >> 16;

    // Clip result to 16-bit
    sample = (sample <= -0x8000) ? -0x8000 : ((sample > 0x7fff) ? 0x7fff : sample);
    return sample;
}


void trigger_attack(AudioChannel* channel)  {
	channel->waveform_offset = 0;
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_ATTACK;
    channel->adsr_end_frame = (channel->attack_ms * sample_rate) / 1000;
    channel->adsr_step = ((int32_t)(0xffffff) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame);
    channel->active = true;
}

void trigger_decay(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_DECAY;
    channel->adsr_end_frame = (channel->decay_ms * sample_rate) / 1000;
    channel->adsr_step = ((int32_t)(channel->sustain << 8) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame);
    channel->active = true;
}

void trigger_sustain(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_SUSTAIN;
    channel->adsr_end_frame = (channel->sustain_ms * sample_rate) / 1000;;
    channel->adsr_step = 0;
    channel->active = true;
}

void trigger_release(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_RELEASE;
    channel->adsr_end_frame = (channel->release_ms * sample_rate) / 1000;
    channel->adsr_step = ((int32_t)(0) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame);
    channel->active = true;
}

void off(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_OFF;
    channel->adsr_step = 0;
    channel->active = false;
}
