// ----------------------------------------------------------------------------
// multipass interface for interacting with hardware in a platform agnostic way
// each supported platform will have its own implementation
//
// any apps using multipass should only talk to hardware using functions 
// defined here
// 
// needs control.h for the data types used for preset management
// ----------------------------------------------------------------------------

#pragma once

#include "control.h"

#define MAX_LEVEL 16383


// ----------------------------------------------------------------------------
// voice mapping

#define VOICE_CV_GATE     0
#define VOICE_ER301       1
#define VOICE_JF          2
#define VOICE_TXO_NOTE    3
#define VOICE_TXO_CV_GATE 4
#define MAX_DEVICE_COUNT  5


// ----------------------------------------------------------------------------
// events (comments describe data[] passed to process_hardware_event)

#define TIMED_EVENT             0x00 // high value byte, low value byte
#define MAIN_CLOCK_RECEIVED     0x01 // external, phase
#define MAIN_CLOCK_SWITCHED     0x02 // external
#define GATE_RECEIVED           0x03 // index, phase

#define FRONT_BUTTON_PRESSED    0x10 // pressed/released
#define FRONT_BUTTON_HELD       0x11 // -
#define BUTTON_PRESSED          0x12 // index, pressed/released

#define I2C_RECEIVED            0x30 // i2c data

#define GRID_CONNECTED          0x40 // connected/disconnected
#define GRID_KEY_PRESSED        0x41 // x, y, pressed
#define GRID_KEY_HELD           0x42 // x, y
#define ARC_CONNECTED           0x43 // connected/disconnected
#define ARC_ENCODER_FINE        0x44 // encoder, delta
#define ARC_ENCODER_COARSE      0x45 // encoder, 1==cw/0==ccw

#define MIDI_CONNECTED          0x50 // connected/disconnected
#define MIDI_NOTE               0x51 // channel, note, velocity, on
#define MIDI_CC                 0x52 // channel, cc, value (0..127)
#define MIDI_AFTERTOUCH         0x53 // channel, note, value

#define KEYBOARD_CONNECTED      0x60 // connected/disconnected
#define KEYBOARD_KEY            0x61 // mod, key, pressed/released

#define SHNTH_CONNECTED         0x62 // connected/disconnected
#define SHNTH_BAR               0x63 // bar, pressure (0..255)
#define SHNTH_ANTENNA           0x64 // antenna, pressure (0..255)
#define SHNTH_BUTTON            0x65 // index, pressed/released


// ----------------------------------------------------------------------------
// timers

void add_timed_event(uint8_t index, uint16_t ms, uint8_t repeat);
void stop_timed_event(uint8_t index);
void update_timer_interval(uint8_t index, uint16_t ms);


// ----------------------------------------------------------------------------
// clock

uint64_t get_global_time(void);
uint8_t has_clock_input(void);
uint8_t is_external_clock_connected(void);
void set_clock_output(uint8_t on);


// ----------------------------------------------------------------------------
// inputs

uint8_t get_cv_input_count(void);
int16_t get_cv(uint8_t index);

uint8_t get_gate_input_count(void);
uint8_t get_gate(uint8_t index);


// ----------------------------------------------------------------------------
// outputs

uint8_t get_cv_output_count(void);
void set_cv(uint8_t output, int16_t value);

uint8_t get_gate_output_count(void);
void set_gate(uint8_t output, uint8_t on);


// ----------------------------------------------------------------------------
// controls

uint8_t get_button_count(void);
uint8_t is_button_pressed(uint8_t index);

uint8_t get_knob_count(void);
uint16_t get_knob_value(uint8_t index);


// ----------------------------------------------------------------------------
// grid/arc

uint8_t is_grid_connected(void);
uint8_t get_grid_column_count(void);
uint8_t get_grid_row_count(void);
uint8_t is_grid_vb(void);

void clear_all_grid_leds(void);
void set_grid_led(uint8_t x, uint8_t y, uint8_t level);
void set_grid_led_i(uint16_t index, uint8_t level);
void refresh_grid(void);

uint8_t is_arc_connected(void);
uint8_t get_arc_encoder_count(void);

void clear_all_arc_leds(void);
void set_arc_led(uint8_t enc, uint8_t led, uint8_t level);
void refresh_arc(void);


// ----------------------------------------------------------------------------
// midi

uint8_t is_midi_connected(void);


// ----------------------------------------------------------------------------
// notes

void note(uint8_t voice, uint16_t note, uint16_t volume, uint8_t on);
void note_v(uint8_t voice, int16_t pitch, uint16_t volume, uint8_t on);
void note_on(uint8_t voice, uint16_t note, uint16_t volume);
void note_on_v(uint8_t voice, int16_t pitch, uint16_t volume);
void note_off(uint8_t voice);

void map_voice(uint8_t voice, uint8_t device, uint8_t output, uint8_t on);
void set_output_transpose(uint8_t device, uint16_t output, uint16_t note);
void set_output_transpose_v(uint8_t device, uint16_t output, int16_t pitch);
void set_output_max_volume(uint8_t device, uint16_t output, uint16_t volume);


// ----------------------------------------------------------------------------
// i2c / devices

void mute_device(uint8_t device, uint8_t mute);

void set_as_i2c_leader(void);
void set_as_i2c_follower(uint8_t address);

void set_er301_cv(uint8_t output, int16_t value);
void set_er301_gate(uint8_t output, uint8_t on);

void set_jf_mode(uint8_t mode);
void set_jf_gate(uint8_t output, uint8_t on);

void set_txo_mode(uint8_t output, uint8_t mode);
void set_txo_cv(uint8_t output, int16_t value);
void set_txo_gate(uint8_t output, uint8_t on);

void set_txo_attack(uint8_t output, uint16_t attack);
void set_txo_decay(uint8_t output, uint16_t decay);
void set_txo_waveform(uint8_t output, uint16_t waveform);

int16_t get_txi_input(uint8_t input);
uint16_t get_txi_param(uint8_t param);


// ----------------------------------------------------------------------------
// flash storage

uint8_t is_flash_new(void);
uint8_t get_preset_index(void);
uint8_t get_preset_count(void);

void store_preset_to_flash(uint8_t index, preset_meta_t *meta, preset_data_t *preset);
void store_preset_index(uint8_t index);
void store_shared_data_to_flash(shared_data_t *shared);

void load_preset_from_flash(uint8_t index, preset_data_t *preset);
void load_preset_meta_from_flash(uint8_t index, preset_meta_t *meta);
void load_shared_data_from_flash(shared_data_t *shared);


// ----------------------------------------------------------------------------
// screen

void clear_screen(void);
void fill_line(uint8_t line, uint8_t colour);
void draw_str(const char* str, uint8_t line, uint8_t colour, uint8_t background);
void refresh_screen(void);


// ----------------------------------------------------------------------------
// other

void set_led(uint8_t index, uint8_t level);
void set_debug(uint8_t on);
void print_debug(const char *str);
void print_int(const char *str, int16_t value);
