#include <stdlib.h>
#include <stdio.h>


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
	if Fs build_full_chord (7, chord);
	if G build_full_chord (8, chord);
	if Gs build_full_chord (9, chord);
	if A build_full_chord (10, chord);
	if As build_full_chord (11, chord);
	if B build_full_chord (12, chord);

	// analyse modulation keyboard
	if add9 set_9 (chord);
	if maj3 set_3 (chord);
	if b5 set_b5 (chord);
	if maj7 set_7 (chord);
	if add11 set_11 (chord);
	if no3 reset_3 (chord);
	if no5 reset_5 (chord);
	if no7 reset_7 (chord);

	return instrument;
}

