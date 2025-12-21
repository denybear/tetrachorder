#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"

#include "globals.h"
#include "keypad.h"
#include "chord.h"
#include "keypad.h"
#include "kbd_events.h"


/********************************************************/
/* Keyboad definition                                   */
/* need to adjust number of columns, rows to a minimum	*/
/*		C#		D#			F#		G#		A#			*/
/*	C		D		E	F		G		A		B		*/
/*                                                      */
/*	add11	no3		no5		no7							*/
/*	add9	Maj3	5b		Maj7						*/
/*                                                      */
/*	sw0	sw1	sw2	sw3	sw4	sw5	sw6	sw7 (instr. selection)  */
/*                                                      */
/*  Converted to columns/rows:                          */
/*                                                      */
/*	C		C#		D		D#							*/
/*	E		F		F#		G							*/
/*	G#		A		A#		B							*/
/*	add11	no3		no5		no7							*/
/*	add9	maj3	b5		maj7						*/
/*	sw0		sw1		sw2		sw3							*/
/*	sw4		sw5		sw6		sw7							*/
/*                                                      */
/*	4 columns, 7 rows, 28 keys total 11 GPIO            */
/********************************************************/


/*************/
/* Functions */
/*************/

// take the input chord, and set rootnote, minor 3rd, 5th, minor 7th and bass to come out with minor 7th full chord.
bool build_full_chord (uint8_t root, void *pointer) {

	chord_t *chord = (chord_t *)pointer;

	if (set_rootnote (root, chord) == false) return false;
	set_b3 (chord);
	set_5 (chord);
	set_b7 (chord);
	// here we set the bass all the time; if bass if off, then we will clear the bass from the main
	set_bass_from_root (chord);
	return true;
}


// parse keyboard and based on which key is pressed, build chord
// kbd is a pointer to an array of bools; this array indicates whether the key is pressed or not
// this allows to have an instant photograph of the keyboard at regular times, and use this to build chord
// as inputs, it uses pointer to chord (which will be populated based on which key is pressed) and pointer to KeypadMatrix (gotten from keypad_read() function)
// it returns the pointer to chord array fully populated, as well as instrument number
uint8_t parse_keyboard (void *pointer, KeypadMatrix *kbd) {

	chord_t *chord = (chord_t *)pointer;
	bool pressed [13];											// if the key has been pressed
	uint64_t when_pressed [13];									// when the key has been pressed
	int i, index;

	// define which key corresponds to which byte in the keypad array; yes this is tedious, but this is better for readibility and quick changes
	bool add11 = kbd->pressed [27];
	bool no3   = kbd->pressed [23];
	bool no5   = kbd->pressed [19];
	bool no7   = kbd->pressed [15];
	bool add9  = kbd->pressed [3];
	bool maj3  = kbd->pressed [7];
	bool b5    = kbd->pressed [11];
	bool maj7  = kbd->pressed [2];
	// instrument on 6-bit (64 instruments) instead of 8-bit (256 instruments)
	bool sw0   = false;
	bool sw1   = false;
//	bool sw0   = kbd->pressed [(5 * KBD_COL) + 0];
//	bool sw1   = kbd->pressed [(5 * KBD_COL) + 1];
	// we invert boolean to cope with HW soldering issue
	bool sw2   = !kbd->pressed [22];
	bool sw3   = !kbd->pressed [26];
	bool sw4   = !kbd->pressed [14];
	bool sw5   = !kbd->pressed [18];
	bool sw6   = !kbd->pressed [6];
	bool sw7   = !kbd->pressed [10];

	// chromatic keyboard
	when_pressed [0]  = 0;												// when the key has been pressed
	when_pressed [1]  = kbd->press_times [24];
	when_pressed [2]  = kbd->press_times [25];
	when_pressed [3]  = kbd->press_times [20];
	when_pressed [4]  = kbd->press_times [21];
	when_pressed [5]  = kbd->press_times [16];
	when_pressed [6]  = kbd->press_times [12];
	when_pressed [7]  = kbd->press_times [17];
	when_pressed [8]  = kbd->press_times [8];
	when_pressed [9]  = kbd->press_times [13];
	when_pressed [10] = kbd->press_times [4];
	when_pressed [11] = kbd->press_times [9];
	when_pressed [12] = kbd->press_times [0];

	pressed [0]  = false;										// if the key has been pressed
	pressed [1]  = kbd->pressed [24];
	pressed [2]  = kbd->pressed [25];
	pressed [3]  = kbd->pressed [20];
	pressed [4]  = kbd->pressed [21];
	pressed [5]  = kbd->pressed [16];
	pressed [6]  = kbd->pressed [12];
	pressed [7]  = kbd->pressed [17];
	pressed [8]  = kbd->pressed [8];
	pressed [9]  = kbd->pressed [13];
	pressed [10] = kbd->pressed [4];
	pressed [11] = kbd->pressed [9];
	pressed [12] = kbd->pressed [0];


	// analyse the keypad, key by key
	reset_rootnote (chord);			// start with empty chord and bass

	// analyse instrument, and return it as a byte
	uint8_t instrument = 0;
	instrument = sw0 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw1 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw2 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw3 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw4 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw5 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw6 ? ((instrument | 1)<< 1) : (instrument << 1);
	instrument = sw7 ? (instrument | 1) : instrument;

	// analyse chromatic keyboard (we don't check for errors, we assume the code is correct)
	// get the index of the key that has been pressed last
	index = 0;
	for (i=1; i<13; i++) {
		if ((pressed [i]) && (when_pressed [i] >= when_pressed [index])) index = i;
	}
	// index contains the chord whose key has been pressed last
	if (index != 0) {
		build_full_chord (index, chord);

		// analyse modulation keyboard
		if (add9) set_9 (chord);
		if (maj3) {
			reset_3 (chord);
			set_3 (chord);
		}
		if (b5) {
			reset_5 (chord);
			set_b5 (chord);
		}
		if (maj7) {
			reset_7 (chord);
			set_7 (chord);
		}
		if (add11) set_11 (chord);
		if (no3) reset_3 (chord);
		if (no5) reset_5 (chord);
		if (no7) reset_7 (chord);
	}
	
	return instrument;
}

