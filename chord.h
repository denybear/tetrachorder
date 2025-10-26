#ifndef CHORD_H
#define CHORD_H

#include "pico/stdlib.h"

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

chord_t *create_chord();
void reset_rootnote (void *);
bool set_rootnote (uint8_t , void *);
uint8_t get_rootnote (void *);
void reset_bass (void *);
bool set_bass (uint8_t , void *);
bool set_bass_from_root (void *);
void reset_3 (void *);
void set_3 (void *);
void set_b3 (void *);
void reset_4 (void *);
void set_4 (void *);
void reset_5 (void *);
void set_5 (void *);
void set_b5 (void *);
void reset_7 (void *);
void set_7 (void *);
void set_b7 (void *);
void reset_9 (void *);
void set_9 (void *);
void reset_11 (void *);
void set_11 (void *);
int get_midi_notes (uint8_t *, void *, int , int );
int cmp_midi_notes (uint8_t *, int , uint8_t *, int , bool , uint8_t *);

#endif