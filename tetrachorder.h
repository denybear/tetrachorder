#ifndef TETRACHORDER_H
#define TETRACHORDER_H

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "synth.h"
#include "chord.h"

/****************************************************/
/* Define GLOBALS MACRO CONSTANT TYPEDEF PROTOTYPES */
/****************************************************/
// MIDI constants
#define MIDI_NOTEON		0x90
#define MIDI_NOTEOFF	0x80
#define MIDI_PGMCHANGE	0xC0
#define CIN_NOTEON		0x9
#define CIN_NOTEOFF		0x8
#define CIN_PGMCHANGE	0xC
#define CHANNEL			0	 // midi channel 1


/***************************/
/* Define global variables */
/***************************/

// Init main global variables

queue_t synth_queue;					// define communication queue between UI and synth
AudioChannel channels[CHANNEL_COUNT];	// audio channels
//chord_t *chord [12];					// current chords to be played; let's assume 12 chords as we have 12 keys on chromatic keyboard
chord_t *chord;							// current chord to be played
uint8_t midi_notes [256];				// buffer containing the midi notes of the current chord
int midi_notes_size;
uint8_t former_midi_notes [256];		// buffer containing the midi notes of the former chord
int former_midi_notes_size = 0;
uint8_t midi_notes_common [256];		// buffer for midi notes that are common between former and new chord
int midi_notes_common_size;
uint8_t midi_notes_on [256];			// buffer for midi note_on to be played
int midi_notes_on_size;
uint8_t midi_notes_off [256];			// buffer for midi note_off to be played
int midi_notes_off_size;
uint8_t former_instrument = 0;			// number of instrument selected
uint8_t instrument;						// number of instrument selected
bool force_instrument = true;			// force sending program change at start of the program
int voicing = 60;						// C3: voicing for the chord
int voicing_bass = 36;					// C1: voicing for the bass
bool no_bass = false;					// true if we should play no bass
bool is_bass_voicing = false;			// true if encoder drives bass voicing, else encoder drives regular chord voicing

#endif