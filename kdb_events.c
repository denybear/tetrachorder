#include <stdlib.h>
#include <stdio.h>


/*************/
/* Functions */
/*************/


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

bool build_full_chord (uint8_t root, void *pointer) {

	chord_t *chord = (chord_t *)pointer;

	if (set_rootnote (root, chord) == false) return false;
	set_3b (chord);
	set_5 (chord);
	set_7b (chord);
	// here we set the bass all the time; if bass if off, then we will clear the bass from the main
	set_bass_from_root (chord);
	return true;
}


// parse keyboard and based on which key is pressed, build chord
// kbd is a pointer to an array of bools; this array indicates whether the key is pressed or not
// this allows to have an instant photograph of the keyboard at regular times, and use this to build chord
// as inputs, it uses pointer to chord (which will be populated based on which key is pressed) and pointer to keyboard array (gotten from bool* keypad_read() function)
// it returns the pointer to chord fully populated, as well as instrument number
uint8_t parse_keyboard (void *pointer, bool * kbd) {

	chord_t *chord = (chord_t *)pointer;

	// define which key corresponds to which byte in the keypad array; yes this is tedious, but this is better for readibility and quick changes
	bool C     = kdb [0][0];		// column, rows
	bool Cs    = kdb [1][0];
	bool D     = kdb [2][0];
	bool Ds    = kdb [3][0];
	bool E     = kdb [0][1];		// column, rows
	bool F     = kdb [1][1];
	bool Fs    = kdb [2][1];
	bool G     = kdb [3][2];
	bool Gs    = kdb [0][2];		// column, rows
	bool A     = kdb [1][2];
	bool As    = kdb [2][2];
	bool B     = kdb [3][2];
	bool add11 = kdb [0][2];		// column, rows
	bool no3   = kdb [1][2];
	bool no5   = kdb [2][2];
	bool no7   = kdb [3][2];
	bool add9  = kdb [0][3];		// column, rows
	bool maj3  = kdb [1][3];
	bool b5    = kdb [2][3];
	bool maj7  = kdb [3][3];
	bool sw0   = kdb [0][4];		// column, rows
	bool sw1   = kdb [1][4];
	bool sw2   = kdb [2][4];
	bool sw3   = kdb [3][4];
	bool sw4   = kdb [0][5];		// column, rows
	bool sw5   = kdb [1][5];
	bool sw6   = kdb [2][5];
	bool sw7   = kdb [3][5];
	
	// analyse the keypad, key by key
	reset_rootnote (chord);			// start with empty chord and bass

	// analyse instrument, and return it as a byte
	int i;
	bool *pt = &sw0;
	uint8_t instrument = 0;
	for (i=0; i<8; i++) {
		instrument = instrument << 1;
		if (pt [i] == true) instrument |= 1;
	}

	// analyse chromatic keyboard (we don't check for errors, we assume the code is correct)
	if C build_full_chord (1, chord);
	if Cs build_full_chord (2, chord);
	if D build_full_chord (3, chord);
	if Ds build_full_chord (4, chord);
	if E build_full_chord (5, chord);
	if F build_full_chord (6, chord);
	if F# build_full_chord (7, chord);
	if G build_full_chord (8, chord);
	if G# build_full_chord (9, chord);
	if A build_full_chord (10, chord);
	if A# build_full_chord (11, chord);
	if B build_full_chord (12, chord);

	// analyse modulation keyboard
	if add9 set_9 (chord);
	if maj3 set_3 (chord);
	if b5 set_5b (chord);
	if maj7 set_7 (chord);
	if add11 set_11 (chord);
	if no3 reset_3 (chord);
	if no5 reset_5 (chord);
	if no7 reset_7 (chord);

	return instrument;
}


/***********************************/
/* definition of a chord structure */
/***********************************/

typedef struct chord_t {
	uint32_t bitmap;
	uint8_t rootnote;
	uint8_t bass;
} chord_t;

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
	if (root < 1) || (root > 12) {
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
	if (root < 1) || (root > 12) {
		chord->bass = 0;
		return false;
	}
	chord->bass = root;
	return true;
}

// Following functions are to set/reset some of the notes in the chord: major3rd or minor3rd, 5th or flat 5th, 7th or major7th, 9th, 11th
void reset_3 (void *pointer) {
	chord_t *chord = (chord_t *)pointer;
	chord->bitmap &= 0b111001111111111111111111;
//                     R 2334 5 677R 9  1   1
//                        mM     mM     1   3
}

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

void set_3b (void *pointer) {	// minor 3rd
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

void set_5b (void *pointer) {	// flat 5th
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

void set_7b (void *pointer) {	// flat 7th
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
// the table of bytes "result" shall be declared outside the function. It should be at least: R + 3 + 5 + 7 + 9 + 11 + bass = 7 bytes for a full chord + bass 
// function returns the number of midi notes to be sent (size of result to be considered)
int get_midi_notes (uint8_t *result, void *pointer, int voicing, int voicing_bass) {	// result should be allocated outside the function

	chord_t *chord = (chord_t *)pointer;
	int nb = 0;											// number of midi notes to return

	// start with main chord first
	if (chord->rootnote == 0) return 0;					// no chord available : exit
	start_voicing = voicing % 12;
	end_voicing = start_voicing + 11;
	voicing = ((int) (voicing / 12)) * 12;				// voicing to be multiple of 12
	
	uint32_t ch = chord->bitmap;
	int index = 0;
		
	// go through chord and retrieve all the notes to be played; C=1 C#=2 D=3 D#=4 E=5 F=6 F#=7 G=8 G#=9 A=10 A#=11 B=12
	while (ch > 0) {
		if ((ch & 0b100000000000000000000000) != 0) {			// test MSB
			int elt = (chord->rootnote - 1) + index;			// note to be played (C being 1)

			# now, make sure the chord is in the voicing
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
	start_voicing_bass = voicing_bass % 12;
	end_voicing_bass = start_voicing_bass + 11;
	voicing_bass = ((int) (voicing_bass / 12)) * 12;			// voicing_bass to be multiple of 12
	
	int elt = (chord->bass - 1);								// note to be played (C being 1)
	# now, make sure the chord is in the voicing
	if (elt < start_voicing_bass) elt += 12;
	if (elt > end_voicing_bass) elt -=12;	
	elt = elt + voicing_bass;									// add to final voicing: this is the "note" in the midi message

	while (elt > 127) elt -= 12;								// final test to make sure we are within midi range
	while (elt < 0) elt += 12;									// final test to make sure we are within midi range
	result [nb++] = (uint8_t) elt;								// add midi note to the list of midi notes to be played

	return nb;
}

