#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "globals.h"
#include "audio.h"
#include "synth.h"
#include "play.h"

/*************************************/
/* MACRO CONSTANT TYPEDEF PROTOTYPES */
/*************************************/
// table of frequencies : this is the note frequency based on its midi number (0 to 127)
const float frequencies[] = {
    8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.250,
    12.978, 13.750, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445,
    20.602, 21.827, 23.125, 24.500, 25.957, 27.500, 29.135, 30.868,
    32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999,
    51.913, 55.000, 58.270, 61.735, 65.406, 69.296, 73.416, 77.782,
    82.407, 87.307, 92.499, 97.999, 103.826, 110.000, 116.541, 123.471,
    130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998,
    207.652, 220.000, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127,
    329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883,
    523.251, 554.365, 587.330, 622.254, 659.255, 698.456, 739.989, 783.991,
    830.609, 880.000, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508,
    1318.510, 1396.913, 1479.978, 1567.982, 1661.219, 1760.000, 1864.655, 1975.533,
    2093.005, 2217.461, 2349.318, 2489.016, 2637.020, 2793.826, 2959.955, 3135.963,
    3322.438, 3520.000, 3729.310, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032,
    5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040.000, 7458.620, 7902.133,
    8372.018, 8869.844, 9397.273, 9956.063, 10548.080, 11175.300, 11839.820, 12543.850
};


/**********************************/
/* Pico audio i2s board functions */
/**********************************/

// 0: piano
// 1: piano2
// 2: reed
// 3: guitar
// 4: pluckedguitar
// 5: bass
// 6: violin
// 7: horn
// 8: oboe
// 9: clarinette
// 10: flute

// waveform, attack in ms, decay in ms, sustain volume (0xafff = 70% of max volume), sustain in ms,
// release in ms, channel volume (set at 10000 to avoid saturation; it can be up to 0xffff)
const uint32_t instruments[NB_INSTRUMENTS][7] = {
	PIANO, 30, 20, 0xafff, 2000, 1000, 10000,
	PIANO2, 30, 20, 0xafff, 2000, 1000, 10000,
	REED, 30, 20, 0xafff, 2000, 1000, 10000,
	GUITAR, 10, 10, 0xafff, 1000, 500, 10000,
	PLUCKEDGUITAR, 10, 10, 0xafff, 1000, 500, 10000,
	SQUARE, 10, 10, 0xafff, 1000, 500, 5000,
	VIOLIN, 50, 200, 0xafff, 500, 5000, 10000,
	HORN, 120, 50, 0xafff, 2000, 100, 10000,
	OBOE, 120, 50, 0xafff, 2000, 100, 10000,
	CLARINETTE, 120, 50, 0xafff, 2000, 100, 10000,
	FLUTE, 120, 50, 0xafff, 2000, 100, 10000
};


// plays a note on a channel 
void update_playback (int chan, uint8_t note) {

	// get notes data from the structure, and pass it to synthetizer
	channels[chan].midi_note = note;
	channels[chan].frequency = (uint16_t) roundf (frequencies [note]);
	trigger_attack (&channels[chan]);
}


// release an active channel
void stop_playback (int chan) {

	// we must update the playback with release on a channel

	// if channel is in OFF state, then do nothing
	// if channel is already in release state, then do nothing
	// if channel is in another state, then go to release state
	if ((channels[chan].adsr_phase != ADSR_OFF) && (channels[chan].adsr_phase != ADSR_RELEASE)) {
		trigger_release (&channels[chan]);
	}
}


// shut down a channel
void reset_playback (int chan) {

	// we must stop a channel
	off (&channels[chan]);	// shut down channel and set it as inactive
}


// shut down all the channels
void reset_playback_all () {

	// we must stop all channels
	for (int i = 0; i < CHANNEL_COUNT; i++) {
		reset_playback (i);		// shut down channel and set it as inactive
	}
}


// load an instrument of a song into a channel
bool load_instrument(int instr, int chan) {

	// check boundaries
	if ((instr <0) || (instr >= NB_INSTRUMENTS)) return false;
	if ((chan <0) || (chan >= CHANNEL_COUNT)) return false;

	// assign instrument parameters to the channel

	channels[chan].waveforms   = instruments [instr][0];
	channels[chan].attack_ms   = instruments [instr][1];
	channels[chan].decay_ms    = instruments [instr][2];
	channels[chan].sustain     = instruments [instr][3];
	channels[chan].sustain_ms  = instruments [instr][4];
	channels[chan].release_ms  = instruments [instr][5];
	channels[chan].volume      = instruments [instr][6];

	return true;
}


// go through the notes to be played, muted, etc and set the audio channels accordingly
// send this to synthetizer so it is playde by i2s pico audio board
void song_task() {

	int i, j;
	
	// go through the list of notes to be kept untouched, and do nothing to the channel
	for (i=0; i < midi_notes_common_size; i++) {
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if (channels[j].midi_note == midi_notes_common[i]);		// do nothing
		}
	}

	// go through the list of midi notes off, and stop corresponding channel, put the channel as inactive;
	for (i=0; i < midi_notes_off_size; i++) {
printf ("number of note off: %d\n", midi_notes_off_size);
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if (channels[j].midi_note == midi_notes_off[i]) {
				// stop channel, set inactive
printf ("channel:%d, midi note off: %d\n", j, midi_notes_off[i]);
				stop_playback (j);
				// channels[j].off();
			}
		}
	}

	// go through the list of midi notes on, and start corresponding channel, by: 1- making sure the note is not played already (should not happen as in this case, the note should // be in the "untouched" list), and 2- we assign note to an inactive channel
	for (i=0; i < midi_notes_on_size; i++) {
printf ("number of note on: %d\n", midi_notes_on_size);
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if (channels[j].active == false) {
				// empty channel: let's use it and play!
printf ("channel:%d, midi note on: %d\n", j, midi_notes_on[i]);			
				update_playback (j, midi_notes_on[i]);
				break;			// assign note to a single channel, then move to next note
			}
		}
	}

	// BEWARE, but it should not happen:
	// 1- We should recalculate the chord if voicing changes (this should be done already, in theory)
	// 2- From a sound perspective, should we adapt the master volume to the number of channels? Or assume same fixed volume for each channel, and master is just the sum of the
	// individual channels? --> master volume applies to all channels (sum of individual channels, as seen in synth.cpp);
	// this ensures volume stays the same regardless the number of active playing channels 
}


void instrument_task() {
	int i;
	
	for (i = 0; i < CHANNEL_COUNT; i++) load_instrument (instrument, i);	// we load the same instrument for all the channels
}