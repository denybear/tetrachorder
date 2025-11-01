#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "globals.h"
#include "audio.h"
#include "synth.h"
#include "play.h"
#include "waveforms.h"

// plays a note on a channel 
void update_playback (int chan, uint8_t note, bool retrigger) {

	// get notes data from the structure, and pass it to synthetizer
	channels[chan].midi_note = note;
	channels[chan].frequency = (uint16_t) roundf (frequencies [note]);
	if (retrigger) retrigger_attack (&channels[chan]);		// retrigger attack while note is playing already
	else trigger_attack (&channels[chan]);					// tigger attack as note is not playing already
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

	channels[chan].waveforms   = instr;
	channels[chan].attack_ms   = instruments [instr][0];
	channels[chan].decay_ms    = instruments [instr][1];
	channels[chan].sustain     = instruments [instr][2];
	channels[chan].sustain_ms  = instruments [instr][3];
	channels[chan].release_ms  = instruments [instr][4];
	channels[chan].volume      = instruments [instr][5];

	return true;
}


// go through the notes to be played, muted, etc and set the audio channels accordingly
// send this to synthetizer so it is playde by i2s pico audio board
void song_task() {

	int i, j;
	bool found;
	
	// go through the list of notes to be kept untouched, and do nothing to the channel
	for (i=0; i < midi_notes_common_size; i++) {
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if (channels[j].midi_note == midi_notes_common[i]);		// do nothing
		}
	}

	// go through the list of midi notes off, and stop corresponding channel, put the channel as inactive;
	for (i=0; i < midi_notes_off_size; i++) {
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if (channels[j].midi_note == midi_notes_off[i]) {
				// stop channel, set inactive
				stop_playback (j);
				// channels[j].off();
			}
		}
	}

	// go through the list of midi notes on, and start corresponding channel, by: 1- making sure the note is not played already (should not happen as in this case, the note should // be in the "untouched" list), and 2- we assign note to an inactive channel
	for (i=0; i < midi_notes_on_size; i++) {
		found = false;
		// check if the same note is being played already (still in ADSR, eg. in release mode); if so, then trigger attack again
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if ((channels[j].adsr_phase != ADSR_OFF) && (channels[j].midi_note == midi_notes_on[i])) {
				// channel plays same note already: let's use it and attack again!
				update_playback (j, midi_notes_on[i], true);
				found = true;
				break;			// assign note to a single channel, then move to next note
			}
		}
		if (found) break;		// if we play the note, then leave

		// in case the same note is not being played already, find an empty channel to play note
		for (j = 0; j < CHANNEL_COUNT; j++) {
			if (channels[j].adsr_phase == ADSR_OFF) {
				// empty channel: let's use it and play!
				update_playback (j, midi_notes_on[i], false);
				break;			// assign note to a single channel, then move to next note
			}
		}
	}
}


void instrument_task(int instr) {
	int i;
	
	for (i = 0; i < CHANNEL_COUNT; i++) load_instrument (instr, i);		// we load the same instrument for all the channels
}


void core1_main() {		//The program running on core 1

	uint32_t data;
	uint8_t *midi;
	int i,j;
	bool found;

	// configure audio
	struct audio_buffer_pool *ap = init_audio();
	set_audio_rate_and_volume (SAMPLE_RATE, VOLUME);	// set audio rate & volume at synthetizer level
	reset_playback_all ();								// at start, stop all audio channels and set all channels to inactive

	while (true) {
		if (queue_try_remove(&synth_queue, &data)) {
			// Perform processing here
			midi = (uint8_t *) &data;		// read 32-bit data as 4 bytes of 8-bit

			// analyse midi data and play accordingly
			switch (midi[1] & 0xF0) {
				case MIDI_PGMCHANGE:
					instrument_task (midi[2]);
				break;

				case MIDI_NOTEOFF:
					for (i = 0; i < CHANNEL_COUNT; i++) {
						if (channels[i].midi_note == midi[2]) {
							// stop channel, set inactive
							stop_playback (i);
						}
					}
				break;

				case MIDI_NOTEON:
					found = false;
					// check if the same note is being played already (still in ADSR); if so, then trigger attack again
					for (i = 0; i < CHANNEL_COUNT; i++) {
						if ((channels[i].adsr_phase != ADSR_OFF) && (channels[i].midi_note == midi[2])) {
							// channel plays same note already: let's use it and attack again!
							update_playback (i, midi[2], true);
							found = true;
							break;			// assign note to a single channel, then move to next note
						}
					}
					if (found) break;		// if we play the note, then leave

					// in case the same note is not being played already, find an empty channel to play note
					for (i = 0; i < CHANNEL_COUNT; i++) {
						if (channels[i].adsr_phase == ADSR_OFF) {
							// empty channel: let's use it and play!
							update_playback (i, midi[2], false);
							break;			// assign note to a single channel, then move to next note
						}
					}
				break;
			}
		}
		else {
            // printf("Queue is empty.\n");
		}

		// update audio buffer : make sure we do this regularly (in while loop)
	   	update_buffer(ap, get_audio_frame);
	}
}