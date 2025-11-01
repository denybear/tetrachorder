#include "pico/stdlib.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "globals.h"
#include "audio.h"
#include "synth.h"

extern const int16_t waveforms[64][8][256];


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
    int32_t channel_sample;
	int index = 0;

    for (int c = 0; c < CHANNEL_COUNT; c++) {
        AudioChannel* channel = &channels[c];
        channel_sample = 0;

        // Increment the waveform position counter
        // we do over-sampling, ie. instead of 256 samples per waveform, we consider to have 256 >> 8 = 65536 (16-bits)
        channel->waveform_offset += ((channel->frequency * SAMPLES_PER_BUFFER) << 8) / sample_rate;

        if (channel->adsr_phase == ADSR_OFF) {      // in case channel is inactive (not playing), then leave
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
        channel->adsr_frame++;                  // number of frames into the current ADSR phase
        channel->waveform_offset &= 0xffff;

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
        // channel sample at this stage is signed 16-bits
        // it looks as if channel->adsr is actually unsigned 24-bit, but expressed on 32-bit
        // channel->adsr is the real-time volume at which the sample should be played (0x0000-0xffffff)
        // given we shift channel->adsr of 8-bit (>>8), then it is 16-bit
        // signed 16-bit * 16-bit = signed 32-bit (but we store it in a 64-bit variable); then we make it signed 16-bit again
        // We do the same for channel volume, except that channel volume is on unsigned 16-bit, so no need to >>8.
        // this is fine to shift >>8 and >>16 because C compiler propagates the sign bit, ie. incoming bits to the left will
        // be 1 to keep the sign bit.

        channel_sample = ((int64_t)(channel_sample) * (int32_t)(channel->adsr >> 8)) >> 16;
        channel_sample = ((int64_t)(channel_sample) * (int32_t)(channel->volume)) >> 16;


        // Combine channel sample into the final sample
        // here, we have say 16 channels. Suppose all the channel samples are up to the max,
        // this makes 16*0x7fff = 0x80008 (=20 bits if positive); but in 32-bit (sample is 32-bit)
        // this makes 0x00080008 for all samples positive to the max, and 0xFFFFFF80 for all samples negative to the min
        sample += channel_sample;
    }
    // given signed 20-bit (sample) * unsigned 16-bit (volume) requires a result on 37-bit, we need a 64-bit temp variable
    // then shift to take only the MSB; no problem with signed operation, the C compiler keeps the sign when shifting bits.
    // we want a 16-bit result from a 37-bit value, ie. we have to shift 21 bits
    // if number of channels is between 9 and 31, sample will be coded in 20-bit (result = 37-bit); requiring in the end a shift >>21.
    // if number of channels is lower or equal to 8, sample will be coded in 19-bit (result = 35-bit); requiring in the end a shift >>19.
    // in the end, sample is on 16-bit signed.
//    sample = ((int64_t)(sample) * (int32_t)(volume)) >> 21;
    sample = ((int64_t)(sample) * (int32_t)(volume)) >> 20; // we increase volume and accuracy, and will tolerate a bit of clipping


/* debug only
    // Clip result to 16-bit
    if (sample < -0x8000) {
        printf ("clipping low\n");
        sample = -0x8000;
    }
    if (sample > 0x7fff) {
        printf ("clipping high\n");
        sample = 0x7fff;
    }
*/
    sample = (sample <= -0x8000) ? -0x8000 : ((sample > 0x7fff) ? 0x7fff : sample);
    return sample;
}

void retrigger_attack(AudioChannel* channel)  {     // re-trigger attack from a note that was already playing, in an ADSR phase already
                                                    // in this case, ADSR volume should not start from 0 but from current volume
    int i=0;
    uint32_t frame = 0;
    uint32_t adsr = 0;

    channel->adsr_phase = ADSR_ATTACK;
    channel->adsr_end_frame = (channel->attack_ms * sample_rate) / 1000;    // frame target at which the ADSR changes to the next phase
//    channel->adsr_step = ((int32_t)(0xffffff) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame); // volume increment of current sample
    channel->adsr_step = (int32_t)(0xffffff) / (int32_t)(channel->adsr_end_frame); // volume increment of current sample
    // based on current adsr (volume of current sample, compute the current adsr_frame (ie. frame number into the current ADSR phase))
    while (adsr < channel->adsr) {
        frame++;
        adsr +=channel->adsr_step;
    }
    channel->adsr_frame = frame;                // number of frames into the current ADSR phase
    channel->adsr = adsr;
}

void trigger_attack(AudioChannel* channel)  {   // trigger attack from 0 (note was not playing already)
	channel->waveform_offset = 0;
    channel->adsr_frame = 0;                // number of frames into the current ADSR phase
    channel->adsr = 0;                      // volume of the curent sample, based on ADSR
    channel->adsr_phase = ADSR_ATTACK;
    channel->adsr_end_frame = (channel->attack_ms * sample_rate) / 1000;    // frame target at which the ADSR changes to the next phase
//    channel->adsr_step = ((int32_t)(0xffffff) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame); // volume increment of current sample
    channel->adsr_step = (int32_t)(0xffffff) / (int32_t)(channel->adsr_end_frame); // volume increment of current sample
}

void trigger_decay(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_DECAY;
    channel->adsr_end_frame = (channel->decay_ms * sample_rate) / 1000;
    channel->adsr_step = ((int32_t)(channel->sustain << 8) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame);
}

void trigger_sustain(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_SUSTAIN;
    channel->adsr_end_frame = (channel->sustain_ms * sample_rate) / 1000;;
    channel->adsr_step = 0;
}

void trigger_release(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr_phase = ADSR_RELEASE;
    channel->adsr_end_frame = (channel->release_ms * sample_rate) / 1000;
    channel->adsr_step = ((int32_t)(0) - (int32_t)(channel->adsr)) / (int32_t)(channel->adsr_end_frame);
}

void off(AudioChannel* channel) {
    channel->adsr_frame = 0;
    channel->adsr = 0;
    channel->adsr_phase = ADSR_OFF;
    channel->adsr_step = 0;
}
