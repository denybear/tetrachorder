/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

// project-wide includes
#include "tetrachorder.h"		// global variables init

// USB midi device
#include "bsp/board_api.h"
#include "tusb.h"
#include "tusb_config.h"
// rotary encoder + button
#include "encoder_button.h"
// matrix keypad
#include "keypad.h"
// neopixel LEDs
#include "ws2812.pio.h"			// in pico_examples git
// keyboard parsing and chords
#include "kbd_events.h"
// pico i2s audio board
#include "audio.h"
#include "synth.h"
#include "chord.h"
#include "play.h"


/***********************/
/* Midi USB prototypes */
/***********************/

void midi_task();


/*************************************/
/* Matrix Keypad callbacks & globals */
/*************************************/

/**
 * @brief Define the keypad matrix column GPIOs
 */
const uint8_t cols[] = {5, 6, 7, 8};

/**
 * @brief Define the keypad matrix row GPIOs
 */
const uint8_t rows[] = {22, 21, 20, 19, 18, 17, 16};

/**
 * @brief Create the keypad matrix structure
 */
KeypadMatrix keypad;

/**
 * @brief Callback function for key press event
 *
 * @param key Key number
 */
void key_pressed(uint8_t key){
	//printf("Key %d pressed\n", key);
}

/**
 * @brief Callback function for key release event
 *
 * @param key Key number
 */
void key_released(uint8_t key){
	//printf("Key %d released\n", key);
}

/**
 * @brief Callback function for long press event
 *
 * @param key Key number
 */
void key_long_pressed(uint8_t key){
//	printf("Key %d long pressed\n", key);
}


/**************************************/
/* Rotary Encoder and button callback */
/**************************************/

void onchange(rotary_encoder_t *encoder) {
	if (is_bass_voicing) {
		// we change bass voicing
		voicing_bass += encoder->direction;
		if (voicing_bass < 0) {
			voicing_bass = -1;
			no_bass = true;
			//printf("voicing_bass is off\n");
		}
		else {
			voicing_bass = MAX (0, voicing_bass);
			voicing_bass = MIN ((127-12), voicing_bass);			
			no_bass = false;
			//printf("voicing_bass is on, value is: %d\n", voicing_bass);
		}
	}
	else {
		// we change regular voicing
		voicing += encoder->direction;
		voicing = MAX (0, voicing);
		voicing = MIN ((127-12), voicing);
		//printf("voicing is on, value is: %d\n", voicing);		
	}
	//printf("Position: %d\n", encoder->position);
	//printf("State: %d%d\n", encoder->state&0b10 ? 1 : 0, encoder->state&0b01);
}

void onpress(button_t *button) {
	if (!button->state) {				// we do this only when the button is pressed, but not depressed
		is_bass_voicing = is_bass_voicing ? false : true;
		//printf("Button pressed, is_bass_voicing is: %s\n", is_bass_voicing ? "true (we select bass voicing)" : "false (we select regular voicing)");
	}
}


/**********************/
/* Neopixel functions */
/**********************/

/* This part is the Neopixel part. Upon receiving a MIDI note-on, we light the leds of Neopixel
 * and we keep these lit for a few ms
 */
 
 /*

#define LED_PIN		6	// GPIO 6 (pin #9) connected to NeoPixel data line
#define NUM_PIXELS	1	// Number of NeoPixels in the strip
// globals
PIO pio = pio0;
uint sm = 0;
uint64_t neoPixelOnTime = 0;			// current time when Neopixel has been lit 
int neoPixelState = 3;					// determine whether Neopixel strip should be set to on
										// 0: light in black
										// 1: light in red
										// 2: light in yellow
										// 3: do nothing (pass)
	
void set_pixel_color(uint32_t color, uint32_t pixel_index, uint32_t *pixels) {
	pixels [pixel_index] = color;
}

uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

void light_strip (uint32_t color) {

	uint32_t pixels[NUM_PIXELS] = {0};

	for (uint32_t i = 0; i < NUM_PIXELS; i++) {
		set_pixel_color (color, i, pixels);
		for (int j = 0; j < NUM_PIXELS; j++) {
			pio_sm_put_blocking (pio, sm, pixels [j] << 8u);	// Send data to NeoPixels
		}
	}
}
*/

/* End of neopixel part
*/


/*------------- MAIN -------------*/
int main(void)
{
	// Overclock
	set_sys_clock_khz(200000, 1);		// 200MHz = 200000kHz

	stdio_init_all();
	board_init();
	printf("Tetrachorder\r\n");

	// init multicore and queue for communication
	queue_init(&synth_queue, sizeof(uint32_t), 256);	// Initialize a queue for 256 items of 4 bytes (uint32)
	multicore_launch_core1 (core1_main);				// Reset core1 for synth and and enter the core1_main function

	// init device stack on configured roothub port
	tud_init(BOARD_TUD_RHPORT);

	if (board_init_after_tusb) {
		board_init_after_tusb();
	}

	// Globals init
	chord = create_chord ();	// create current chord to be played
	reset_playback_all ();		// reset all synth channels to off

	// Rotary encoder inits
	rotary_encoder_t *encoder = create_encoder(2, 3, onchange);			// GPIO to be changed here
	printf("Rotary Encoder created and it's state is %d%d\n", encoder->state&0b10 ? 1 : 0, encoder->state&0b01);
	printf("Rotary Encoder created and it's position is %d\n", encoder->position);
	button_t *button = create_button(4, onpress);						// GPIO to be changed here
	printf("Button created and it's state is %d\n", button->state);
	
	// Matrix keyboard inits
	// Apply the keypad configuration defined earlier and declare the number of columns and rows
	printf ("keypad init\n");
	keypad_init(&keypad, cols, rows, KBD_COL, KBD_ROW);
	// Assign the callbacks for each event
	keypad_on_press(&keypad, key_pressed);
	keypad_on_release(&keypad, key_released);
	keypad_on_long_press(&keypad, key_long_pressed);
	// Adjust the hold threshold to two seconds. Default is 1500ms
	keypad_set_hold_threshold(&keypad, 2000);


	// Neopixels inits
	// Initialize PIO and load the WS2812 program
	//uint offset = (uint) pio_add_program (pio, &ws2812_program);
	//ws2812_program_init (pio, sm, offset, LED_PIN, 800000, false);
	// End of NeoPixel inits


	// main
	while (true) {
		// Poll the keypad
		// keypad_read(&keypad);
		// Alternatively, the output of keypad_read() can be stored as a pointer to the array containing the state of each key.
		//
		// how to deal with several chords being pressed at the same time? For example C chord and D chord pressed at the same time?
		// 2 options:
		// a- we make a 12 chord table, and manage 12 chords instead of 1; drawback is managing 12 chords and having 12 chords pressed at the same time
		// will end up in a too noisy situation
		//
		// b- we use a timer to detect when chord keys have been pressed, and take only into account the latest chord key pressed (based on "when" value);
		// this is what we will do, and hopefully the timer is implemented in keypad.c already

		keypad_read (&keypad);
		sleep_ms(20);

		instrument = parse_keyboard (chord, &keypad);	// analyse key presses to get which chords has been selected

		if (no_bass) reset_bass (chord);				// remove bass note in case we don't want to play it
		// midi_notes that are contained in the chord
		midi_notes_size = get_midi_notes (midi_notes, chord, voicing, voicing_bass);
		// determine lists of notes which should be on / off, and list of notes that are common
		midi_notes_common_size = cmp_midi_notes (midi_notes, midi_notes_size, former_midi_notes, former_midi_notes_size, true, midi_notes_common);
		midi_notes_on_size = cmp_midi_notes (midi_notes, midi_notes_size, former_midi_notes, former_midi_notes_size, false, midi_notes_on);
		midi_notes_off_size = cmp_midi_notes (former_midi_notes, former_midi_notes_size, midi_notes, midi_notes_size,  false, midi_notes_off);

		tud_task(); 												// tinyusb device task
		midi_task();												// manage midi tasks, send notes, send program select

// the 2 below calls are useless as we now communicate with core1 (synth) through midi_task()
//		if (former_instrument != instrument) instrument_task (instrument);	// load new instrument if required
//		song_task ();												// send to pico audio i2s board

		// make new chord & instrument become former chord & instrument
		former_instrument = instrument;
		memcpy (former_midi_notes, midi_notes, midi_notes_size);
		former_midi_notes_size = midi_notes_size;


/* Neopixel part
		// manage neopixel led strip: light the strip with the right color; in case of black, wait 150ms before unlighting
		switch (neoPixelState) {
			case 0:
				if (((to_us_since_boot (get_absolute_time()) - neoPixelOnTime)) > 150000) {	// paint to black after 150ms
					light_strip (rgb_to_color (0, 0, 0));	// black
					neoPixelState = 3;						// next time do nothing
				}
				break;
			case 1:
				neoPixelOnTime = to_us_since_boot (get_absolute_time());
				light_strip (rgb_to_color (255, 0, 0));		// red
				neoPixelState = 0;							// next state is black
				break;
			case 2:
				neoPixelOnTime = to_us_since_boot (get_absolute_time());
				light_strip (rgb_to_color (255, 255, 0));	// yellow
				neoPixelState = 0;							// next state is black
				break;
			default:
				break;
		}
		// end of management of neopixel led strip	
*/
	}
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
	//blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
	//blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
	(void) remote_wakeup_en;
	//blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
	//blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+

void midi_task()
{
	uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint

	// note that we are using USB MIDI EVENTS: https://www.usb.org/sites/default/files/midi10.pdf
	// these are 4-bytes messages supposed to describe any standard MIDI message

	// The MIDI interface always creates input and output port/jack descriptors
	// regardless of these being used or not. Therefore incoming traffic should be read
	// (possibly just discarded) to avoid the sender blocking in IO
	// here, we check for note_on event, and if received, then we light the Neopixel strip
	uint32_t *data;
	uint8_t packet[4];
	bool read = false;
	int i;

	while ( tud_midi_available() ) {
		read = tud_midi_packet_read (packet);	// read midi EVENT (and we don't care)
		
		// byte 0 = cable number | Code Index Number (CIN)
		// byte 1 = MIDI 0 
		// byte 2 = MIDI 1 
		// byte 3 = MIDI 2
		// CIN = 0x08 for note off, 0x09 for note on, 0x0B for control change, 0x0C for program change, etc

/*
		// NeoPixel part
		// test if note on, and velocity not null: in this case, lite the leds ON (in the while loop)
		if (read) {
			if ((packet [1] == (MIDI_NOTEON | CHANNEL)) && (packet [3] != 0)) {
				if (packet [3] == 127) neoPixelState = 1;	// first beat (velocity == 127) is red
				else neoPixelState = 2;						// other beats (other velocities) are yellow
			}
		}
		// End of Neopixel part
*/

	}

	// check for program change
	uint8_t pgm_change[4] = { (cable_num << 4) | CIN_PGMCHANGE, MIDI_PGMCHANGE | CHANNEL, 0, 0};
	data = (uint32_t *) pgm_change;			// assign 32-bit data as 4 bytes of 8-bit


	// Send program change on channel in case instrument has changed, or at the very start of the program
	if ((force_instrument) || (instrument != former_instrument)) {
		force_instrument = false;
		pgm_change[2] = (uint8_t) (instrument & 0x7F);
		tud_midi_packet_write (pgm_change);			// send to USB

		if (!queue_try_add(&synth_queue, data)) {	// send to synth
			printf("Queue is full.\n");
		}
	}

	// send notes off events
	uint8_t note_off[4] = { (cable_num << 4) | CIN_NOTEOFF, MIDI_NOTEOFF | CHANNEL, 0, 0 };
	data = (uint32_t *) note_off;			// assign 32-bit data as 4 bytes of 8-bit

	// Send Note Off at no velocity (0) on channel.
	for (i=0; i<midi_notes_off_size; i++) {
		note_off[2] = midi_notes_off [i];
		note_off[3] = 0x00;
		tud_midi_packet_write (note_off);			// send to USB

		if (!queue_try_add(&synth_queue, data)) {	// send to synth
			printf("Queue is full.\n");
		}
	}

	// send notes off events
	uint8_t note_on[4] = { (cable_num << 4) | CIN_NOTEON, MIDI_NOTEON | CHANNEL, 0, 127 };
	data = (uint32_t *) note_on;			// assign 32-bit data as 4 bytes of 8-bit

	// Send Note On at full velocity (127) on channel.
	for (i=0; i<midi_notes_on_size; i++) {
		note_on[2] = midi_notes_on [i];
		note_on[3] = 0x7F;
		tud_midi_packet_write (note_on);			// send to USB

		if (!queue_try_add(&synth_queue, data)) {	// send to synth
			printf("Queue is full.\n");
		}
	}
}

