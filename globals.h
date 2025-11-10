#ifndef GLOBALS_H
#define GLOBALS_H

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
#define CHANNEL			0		// midi channel 1
#define KBD_ROW			7		// number of rows defined on matrix keypad
#define KBD_COL			4		// number of columns defined on matrix keypad


/************************************/
/* definition of a global variables */
/************************************/

extern queue_t synth_queue;						// define communication queue between UI and synth
extern AudioChannel channels[CHANNEL_COUNT];	// audio channels
//extern chord_t *chord [12];						// current chords to be played; let's assume 12 chords as we have 12 keys on chromatic keyboard
extern chord_t *chord;							// current chord to be played
extern uint8_t midi_notes [256];				// buffer containing the midi notes of the current chord
extern int midi_notes_size;
extern uint8_t former_midi_notes [256];			// buffer containing the midi notes of the former chord
extern int former_midi_notes_size;
extern uint8_t midi_notes_common [256];			// buffer for midi notes that are common between former and new chord
extern int midi_notes_common_size;
extern uint8_t midi_notes_on [256];				// buffer for midi note_on to be played
extern int midi_notes_on_size;
extern uint8_t midi_notes_off [256];			// buffer for midi note_off to be played
extern int midi_notes_off_size;
extern uint8_t former_instrument;				// number of instrument selected
extern uint8_t instrument;						// number of instrument selected
extern bool force_instrument;					// force sending program change at start of the program
extern int voicing;								// C3: voicing for the chord
extern int voicing_bass;						// C1: voicing for the bass
extern bool no_bass;							// true if we should play no bass
extern bool is_bass_voicing;					// true if encoder drives bass voicing, else encoder drives regular chord voicing

#endif