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
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"			// in pico_examples git

#include "bsp/board_api.h"
#include "tusb.h"
#include "tusb_config.h"

#include "encoder.c"
#include "keypad.c"
#include "keypad.h"


/*************************************/
/* Matrix Keypad callbacks & globals */
/*************************************/

/**
 * @brief Define the keypad matrix column GPIOs
 */
const uint8_t cols[] = {4, 5, 6, 7};

/**
 * @brief Define the keypad matrix row GPIOs
 */
const uint8_t rows[] = {0, 1, 2, 3};

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
	printf("Key %d pressed\n", key);
}

/**
 * @brief Callback function for key release event
 *
 * @param key Key number
 */
void key_released(uint8_t key){
	printf("Key %d released\n", key);
}

/**
 * @brief Callback function for long press event
 *
 * @param key Key number
 */
void key_long_pressed(uint8_t key){
	printf("Key %d long pressed\n", key);
}


/***************************/
/* Rotary Encoder callback */
/***************************/

void onchange(rotary_encoder_t *encoder) {
  printf("Position: %d\n", encoder->position);
  printf("State: %d%d\n", encoder->state&0b10 ? 1 : 0, encoder->state&0b01);
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



/* This MIDI example sends midi note-on and note-off commands at press of a switch.
 * There are 5 switches on my home-made pedal; a different note is assigned to each
 * - Linux (Ubuntu): install qsynth, qjackctl. Then connect TinyUSB output port to FLUID Synth input port
 * - Windows: install MIDI-OX
 * - MacOS: SimpleSynth
 */

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+

// MIDI constants
#define MIDI_NOTEON	0x90
#define CIN_NOTEON	0x9
#define CHANNEL		0	 // midi channel 1

// midi notes corresponding to foot pedal switches
#define NOTE_SW1	0


// function prototypes
void midi_task();


/*------------- MAIN -------------*/
int main(void)
{
	stdio_init_all();
	board_init();
	printf("Tetrachorder\r\n");

	// init device stack on configured roothub port
	tud_init(BOARD_TUD_RHPORT);

	if (board_init_after_tusb) {
		board_init_after_tusb();
	}

	// Rotary encoder inits
	rotary_encoder_t *encoder = create_encoder(2, 3, onchange);			// GPIO to be changed here
	printf("Rotary Encoder created and it's state is %d%d\n", encoder->state&0b10 ? 1 : 0, encoder->state&0b01);
	printf("Rotary Encoder created and it's position is %d\n", encoder->position);

	// Matrix keyboard inits
	// Apply the keypad configuration defined earlier and declare the number of columns and rows
	keypad_init(&keypad, cols, rows, 4, 4);
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
		tud_task(); 		// tinyusb device task
		midi_task();	// manage midi tasks

		// Poll the keypad
		keypad_read(&keypad);
		/* Alternatively, the output of keypad_read() can
		be stored as a pointer to the array containing
		the state of each key:
		bool *pressed = keypad_read(&keypad);
		*/
		sleep_ms(5);

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
	uint8_t packet[4];
	bool read = false;

	while ( tud_midi_available() ) {
		read = tud_midi_packet_read (packet);	// read midi EVENT
		
		// byte 0 = cable number | Code Index Number (CIN)
		// byte 1 = MIDI 0 
		// byte 2 = MIDI 1 
		// byte 3 = MIDI 2
		// CIN = 0x08 for note off, 0x09 for note on 

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


/*
TO UPDATE BASED ON MIDI NOTES TO BE SENT

	// check if state has changed, ie. pedal has just been pressed or unpressed
	if (pd->change_state) {

		uint8_t note_on[4] = { (cable_num << 4) | CIN_NOTEON, MIDI_NOTEON | CHANNEL, 0, 127 };

		if (pd->value & SW1) {
			// Send Note On for current switch at full velocity (127) on channel.
			note_on[2] = NOTE_SW1;
			tud_midi_packet_write (note_on);
		}
	}
*/

}

