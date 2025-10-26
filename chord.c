#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"

#include "globals.h"
#include "chord.h"


/*************/
/* Functions */
/*************/

// create a chord object
chord_t *create_chord() {
	chord_t *chord = (chord_t *)malloc(sizeof(chord_t));
	chord->rootnote = 0;
	chord->bitmap = 0;
	chord->bass = 0;
	return chord;
}

// reset rootnote = clear the chord
void reset_rootnote (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->rootnote = 0;
	chord->bitmap = 0;	
	chord->bass = 0;
}

// set rootnote to a value C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
// returns true if set is done correctly, false if set is not done and chord is emptyu
bool set_rootnote (uint8_t root, void *pointer) {
	// C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
	chord_t *chord = (chord_t *)pointer;
	if ((root < 1) || (root > 12)) {
		chord->rootnote = 0;
		chord->bitmap = 0;	
		chord->bass = 0;
		return false;
	}
	chord->rootnote = root;
	chord->bitmap |= 0b100000000000000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
	return true;
}

// get rootnote from the chord: C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
uint8_t get_rootnote (void *pointer) {
	// C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
	chord_t *chord = (chord_t *)pointer;
	return chord->rootnote;
}

// reset bass of the chord
void reset_bass (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bass = 0;
}

// set bass of the chord to a value C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
bool set_bass (uint8_t root, void *pointer) {
	// C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
	chord_t *chord = (chord_t *)pointer;
	if ((root < 1) || (root > 12)) {
		chord->bass = 0;
		return false;
	}
	chord->bass = root;
	return true;
}

// set bass of the chord from the value of root note
bool set_bass_from_root (void *pointer) {
	// C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
	chord_t *chord = (chord_t *)pointer;
	if ((chord->rootnote < 1) || (chord->rootnote > 12)) {
		chord->bass = 0;
		return false;
	}
	chord->bass = chord->rootnote;
	return true;
}

// Following functions are to set/reset some of the notes in the chord: major3rd or minor3rd, 5th or flat 5th, 7th or major7th, 9th, 11th
void reset_3 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111001111111111111111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_3 (void *pointer) {	// major 3rd
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000010000000000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_b3 (void *pointer) {	// minor 3rd
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000010000000000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void reset_4 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111110111111111111111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_4 (void *pointer) {	// sus 4th : should be useless as the same as add9 without 3rd
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000001000000000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void reset_5 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111111101111111111111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_5 (void *pointer) {	// 5th
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000000010000000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_b5 (void *pointer) {	// flat 5th
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000000100000000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void reset_7 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111111111100111111111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_7 (void *pointer) {	// major 7th
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000000000001000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_b7 (void *pointer) {	// flat 7th
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000000000010000000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void reset_9 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111111111111110111111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_9 (void *pointer) {	// 9th
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000000000000001000000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void reset_11 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111111111111111110111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

void set_11 (void *pointer) {	// 11th
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap |= 0b000000000000000001000000;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

// This function reads a chord, and returns the midi notes to be played for both the chord, and the bass if it exists
// The midi notes that are returned are in line with the value of the voicing
// ie. the chord is played in a way that all the notes of the chord are contained within a range of 12 notes, this range being configurable. Same for bass.
// voicing and voicing_bass correspond to the start note of the voicing in midi, that is in (0,127) range
// function uses a pointer to result, a table of bytes. Each byte will contain midi value of one note of the chord to be played, plus bass.
// the table of bytes "result" shall be declared outside the function. It should be at least: R + 3 + 5 + 7 + 9 + 11 + bass = 7 bytes for a full (chord + bass)
// function returns the number of midi notes to be sent (size of result to be considered)
int get_midi_notes (uint8_t *result, void *pointer, int voicing, int voicing_bass) {	// result should be allocated outside the function

	chord_t *chord = (chord_t *)pointer;
	int nb = 0;											// number of midi notes to return

	// start with main chord first
	if (chord->rootnote == 0) return 0;					// no chord available : exit
	int start_voicing = voicing % 12;
	int end_voicing = start_voicing + 11;
	voicing = ((int) (voicing / 12)) * 12;				// voicing to be multiple of 12
	
	uint32_t ch = chord->bitmap;
	int index = 0;
		
	// go through chord and retrieve all the notes to be played; C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
	while (ch > 0) {
		if ((ch & 0b100000000000000000000000) != 0) {			// test MSB
			int elt = (chord->rootnote - 1) + index;			// note to be played (C being 1)

			// now, make sure the chord is in the voicing
			if (elt < start_voicing) elt += 12;
			if (elt > end_voicing) elt -=12;	
			elt = elt + voicing;								// add to final voicing: this is the "note" in the midi message

			while (elt > 127) elt -= 12;						// final test to make sure we are within midi range
			while (elt < 0) elt += 12;							// final test to make sure we are within midi range
			result [nb++] = (uint8_t) elt;						// add midi note to the list of midi notes to be played
		}

		ch = ch & 0b011111111111111111111111;					// shift to next degree
		ch = ch << 1;
		index += 1;
	}

	// manage bass
	if (chord->bass == 0) return nb;							// no bass available : exit
	int start_voicing_bass = voicing_bass % 12;
	int end_voicing_bass = start_voicing_bass + 11;
	voicing_bass = ((int) (voicing_bass / 12)) * 12;			// voicing_bass to be multiple of 12
	
	int elt = (chord->bass - 1);								// note to be played (C being 1)
	// now, make sure the chord is in the voicing
	if (elt < start_voicing_bass) elt += 12;
	if (elt > end_voicing_bass) elt -=12;	
	elt = elt + voicing_bass;									// add to final voicing: this is the "note" in the midi message

	while (elt > 127) elt -= 12;								// final test to make sure we are within midi range
	while (elt < 0) elt += 12;									// final test to make sure we are within midi range
	result [nb++] = (uint8_t) elt;								// add midi note to the list of midi notes to be played

	return nb;
}


// This function compares 2 lists of midi notes together (list A and B), and come out:
// in case equal == false --> with a list of notes (res) that are in list A of midi notes but not in list B of midi notes.
// in case equal == true --> with a list of notes (res) that are both in list A of midi notes and in list B of midi notes.
// the 2 lists should be memory-allocated outside the function; they should be at least 7 bytes
// It returns a new list of midi notes, as well the number of elements in this list 
int cmp_midi_notes (uint8_t *listA, int sizeA, uint8_t *listB, int sizeB, bool equal, uint8_t *res) {

	int i, j, nb = 0;											// nb = number of midi notes to return
	bool found;
	
	for (i=0; i<sizeA; i++) {
		found = false;
		for (j=0; j<sizeB; j++) {
			if (listA [i] == listB[j]) found = true;			// we have found the same element in the 2 lists
		}
		if (equal) {
			if (found) {											// we have found the element that is in list A in list B
				res [nb++] = listA [i];								// add this element to the result
			}
		}
		else {
			if (!found) {											// we haven't found the element that is in list A in list B
				res [nb++] = listA [i];								// add this element to the result
			}
		}			
	}
	
	return nb;													// return number of elements which are not common to both lists
}


