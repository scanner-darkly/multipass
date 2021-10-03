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

/**
 * @brief Adds a new timed event. An existing event timer at the provided index
 *        identifier will be replaced.
 *
 * @param index Used to register and identify the timed event. Valid values are
 *        between 0 and TIMED_EVENT_COUNT - 1
 * @param ms How often the timed event should be triggered in milliseconds
 * @param repeat 0 or 1 value indicating if the timed event should repeat
 *        indefinitely
 */
void add_timed_event(uint8_t index, uint16_t ms, uint8_t repeat);

/**
 * @brief Stops and removes a previously added timed event at the index
 *        identifier.
 * 
 * @param index Index identifier of the timed event to stop
 */
void stop_timed_event(uint8_t index);

/**
 * @brief Updates a previously registed timed event trigger interval in
 *        milliseconds
 * 
 * @param index Index identifier of the timed event to update
 * @param ms How often the timed event should be triggered in milliseconds
 */
void update_timer_interval(uint8_t index, uint16_t ms);


// ----------------------------------------------------------------------------
// clock

/**
 * @brief Get the global time value of milliseconds since start
 * 
 * @return uint64_t Returns milliseconds since start
 */
uint64_t get_global_time(void);

/**
 * @brief Check if hardware has a CV clock input
 * 
 * @return uint8_t Returns 0 if hardware does not have CV clock input,
 *         uint8_t Returns 1 if hardware has CV clock input
 */
uint8_t has_clock_input(void);

/**
 * @brief Check if external CV clock input is connected
 * 
 * @return uint8_t Returns 0 if external CV clock is not connected,
 *         uint8_t Returns 1 if external CV clock is connected
 */
uint8_t is_external_clock_connected(void);

/**
 * @brief Set the clock output on or off
 * 
 * @param on 0 if clock output should be off,
 *           1 if clock output should be on
 */
void set_clock_output(uint8_t on);


// ----------------------------------------------------------------------------
// inputs

/**
 * @brief Get the number of hardware CV inputs available
 * 
 * @return uint8_t Returns number of hardware CV inputs
 */
uint8_t get_cv_input_count(void);

/**
 * @brief Get the CV value for the provided input index. Teletype is currently
 *        the only Monome module that has a CV input.
 * 
 * @param index Hardware CV input number (first input is index 0)
 * @return int16_t Returns CV value represented as a signed integer with the
 *         range of -16,384 (-10v) and 16,383 (+10v).
 */
int16_t get_cv(uint8_t index);

/**
 * @brief Get the number of hardware CV gate inputs available
 * 
 * @return uint8_t Returns number of hardware CV gate inputs
 */
uint8_t get_gate_input_count(void);

/**
 * @brief Get the CV gate value for the provided input index.
 * 
 * @param index Hardware CV gate input number (first input is index 0)
 * @return uint8_t Returns 0 when CV gate input is low,
 *         uint8_t Returns 1 when CV gate input is high
 */
uint8_t get_gate(uint8_t index);


// ----------------------------------------------------------------------------
// outputs

/**
 * @brief Get the number of hardware CV outputs available
 * 
 * @return uint8_t Returns number of hardware CV outputs
 */
uint8_t get_cv_output_count(void);

/**
 * @brief Set the value of the indicated CV output
 * 
 * @param output index value of the CV output
 * @param value CV value represented as a signed integer with a range of
 *        -16,384 (-10v) and 16,383 (+10v)
 */
void set_cv(uint8_t output, int16_t value);

/**
 * @brief Get the number of hardware CV gate outputs available
 * 
 * @return uint8_t Returns number of hardware CV gate outputs
 */
uint8_t get_gate_output_count(void);

/**
 * @brief Set the value of the indicated CV gate output
 * 
 * @param output index value of the CV gate output
 * @param on 0 CV gate output is low,
 *           1 CV gate output is high
 */
void set_gate(uint8_t output, uint8_t on);


// ----------------------------------------------------------------------------
// controls

/**
 * @brief Get the number of hardware buttons available
 * 
 * @return uint8_t Returns number of hardware buttons
 */
uint8_t get_button_count(void);

/**
 * @brief Get the button press status for the button at the indicated index
 * 
 * @param index Button number (first button is index 0)
 * @return uint8_t Returns 0 button not pressed,
 *         uint8_t Returns 1 button pressed
 */
uint8_t is_button_pressed(uint8_t index);

/**
 * @brief Get the number of hardware knobs available
 * 
 * @return uint8_t Returns number of hardware knobs
 */
uint8_t get_knob_count(void);

/**
 * @brief Get the knob value for the provided knob index.
 * 
 * @param index Hardware knob number (first know is index 0)
 * @return uint16_t Returns 
 */
uint16_t get_knob_value(uint8_t index);


// ----------------------------------------------------------------------------
// grid/arc

/**
 * @brief Check if a Monome grid is connected
 * 
 * @return uint8_t Returns 0 if Monome grid is not connected,
 *         uint8_t Returns 1 if Monome grid is connected
 */
uint8_t is_grid_connected(void);

/**
 * @brief Get the number of columns for the connected Monome grid
 * 
 * @return uint8_t Returns the number of columns
 */
uint8_t get_grid_column_count(void);

/**
 * @brief Get the number of rows for the connected Monome grid
 * 
 * @return uint8_t Returns the number of rows
 */
uint8_t get_grid_row_count(void);

/**
 * @brief Check if the connected Monome grid is capable of variable brightness
 * 
 * @return uint8_t Returns 0 if connected Monome grid is not capable of
 *         variable brightness, return 1 if connected Monome grid is capable of
 *         variable brightness
 */
uint8_t is_grid_vb(void);

/**
 * @brief Clears all LEDs of the connected Monome grid
 * 
 */
void clear_all_grid_leds(void);

/**
 * @brief Get an LEDs level for the connected Monome grid
 * 
 * @param x The x-coordinate of the LED
 * @param y The y-coordinate of the LED
 * @return uint8_t Returns the level of the LED at the provided coordinates.
 *         Valid values between 0 and 15, 0 = off, 15 = full level
 */
uint8_t get_grid_led(uint8_t x, uint8_t y);

/**
 * @brief Set an LEDs level for the connected Monome grid
 * 
 * @param x The x-coordinate of the LED to update
 * @param y The y-coordinate of the LED to update
 * @param level The level of the LED to update. Valid values between 0 and 15,
 *        0 = off, 15 = full level
 */
void set_grid_led(uint8_t x, uint8_t y, uint8_t level);

/**
 * @brief Set an LEDs level for the connected Monome grid based on the LEDs
 *        index
 * 
 * @param index Index of the LED to update
 * @param level The lvel of the LED to update. Valid values between 0 and 15,
 *        0 = off, 15 = full level
 */
void set_grid_led_i(uint16_t index, uint8_t level);

/**
 * @brief Marks the connected Monome grid for update
 * 
 */
void refresh_grid(void);

/**
 * @brief Check if a Monome arc is connected
 * 
 * @return uint8_t Returns 0 if Monome arc is not connected,
 *         uint8_t Returns 1 if Monome arc is connected
 */
uint8_t is_arc_connected(void);

/**
 * @brief Get the number of available encoders for the connected Monome arc
 * 
 * @return uint8_t Returns the number of available encoders
 */
uint8_t get_arc_encoder_count(void);

/**
 * @brief Clear all LEDs on the connected Monome arc
 * 
 */
void clear_all_arc_leds(void);

/**
 * @brief Get an LEDs level for the connected Monome arc
 * 
 * @param enc The encoder number
 * @param led The LED number
 * @return uint8_t Return the level of the LED at the provided location. Valid
 *         values between 0 and 15, 0 = off, 15 = full level
 */
uint8_t get_arc_led(uint8_t enc, uint8_t led);

/**
 * @brief Set an LEDs level for the connected Monome arc
 * 
 * @param enc The encoder number to update
 * @param led The LED number to update
 * @param level The level of the LED to update. Valid values between 0 and 15,
 *        0 = off, 15 = full level
 */
void set_arc_led(uint8_t enc, uint8_t led, uint8_t level);

/**
 * @brief Marks the connected Monome arc for update
 * 
 */
void refresh_arc(void);


// ----------------------------------------------------------------------------
// midi

/**
 * @brief Check if a MIDI device is connected
 * 
 * @return uint8_t Returns 0 if a MIDI device is not connected,
 *         uint8_t Returns 1 if a MIDI device is connected
 */
uint8_t is_midi_connected(void);


// ----------------------------------------------------------------------------
// notes

/**
 * @brief Convenience function used to get the hardware output CV pitch value
 *        used to represent a note (equal temper tuning) value from 0-127. This
 *        function is useful when converting a MIDI note number to the
 *        appropriate CV pitch value for Monome hardware CV outputs.
 * 
 * @param note The equal temper tuning value between 0-127. See a MIDI note
 *        value chart.
 * 
 * @return uint16_t Returns a number that maps to 1v/octave CV on a 14bit DAC
 *         with a 10v range.
 * 
 * @see music.c/h in libavr32 library for reference
 */
uint16_t note_to_pitch(uint16_t note);

/**
 * @brief Send a chromatic note, volume, and on/off state to the mapped voice
 *        number
 * 
 * @param voice Number of the mapped voice to send a note on/off to
 * @param note Number of the note to send. Standard MIDI note values apply
 * @param volume Volume level of the note. A volume of 0 is always
 *        considered the equivalent of a note off.
 *        Monome hardware, TELEXo CV/Gate: A value of 0 or  >= 1
 *        ER301, Just Friends, TELEXo note (sound source): A value between 0
 *        and MAX_LEVEL (16,383)
 * @param on 0 if note should be off,
 *           1 if note should be on
 */
void note(uint8_t voice, uint16_t note, uint16_t volume, uint8_t on);

/**
 * @brief Send pitch, volume, and on/off state to the mapped voice number.
 *        Useful for microtonal scales, where the value maps to output
 *        voltage directly, with a range of -16,384 (-10v) and 16,383 (+10v)
 * 
 * @param voice Number of the mapped voice to send a note on/off to
 * @param pitch Value of the pitch to send. A valid value is between -16,384
 *        and 16,383.
 * @param volume Volume level of the note. A volume of 0 is always
 *        considered the equivalent of a note off.
 *        Monome hardware, TELEXo CV/Gate: A value of 0 or  >= 1
 *        ER301, Just Friends, TELEXo note (sound source): A value between 0
 *        and MAX_LEVEL (16,383)
 * @param on 0 if note should be off,
 *           1 if note should be on
 */
void note_v(uint8_t voice, int16_t pitch, uint16_t volume, uint8_t on);

/**
 * @brief Send a chromatic note on, and volume to the mapped voice number
 * 
 * @param voice Number of the mapped voice to send a note on to
 * @param note Number of the note to send. Standard MIDI note values apply
 * @param volume Volume level of the note. A volume of 0 is always
 *        considered the equivalent of a note off.
 *        Monome hardware, TELEXo CV/Gate: A value of 0 or  >= 1
 *        ER301, Just Friends, TELEXo note (sound source): A value between 0
 *        and MAX_LEVEL (16,383)
 */
void note_on(uint8_t voice, uint16_t note, uint16_t volume);

/**
 * @brief Send pitch, and volume to the mapped voice number. Useful for
 *        microtonal scales, where the value maps to output voltage directly,
 *        with a range of -16,384 (-10v) and 16,383 (+10v)
 * 
 * @param voice Number of the mapped voice to send a note on to
 * @param pitch Value of the pitch to send. A valid value is between -16,384
 *        and 16,383.
 * @param volume Volume level of the note. A volume of 0 is always
 *        considered the equivalent of a note off.
 *        Monome hardware, TELEXo CV/Gate: A value of 0 or  >= 1
 *        ER301, Just Friends, TELEXo note (sound source): A value between 0
 *        and MAX_LEVEL (16,383)
 */
void note_on_v(uint8_t voice, int16_t pitch, uint16_t volume);

/**
 * @brief Send a note off to the specified voice
 * 
 * @param voice Number of the mapped voice to send a note off to
 */
void note_off(uint8_t voice);

/**
 * @brief Map a multipass voice to a device output. A multipass voice is a
 *        virtual output mapped to any hardware CV & Gate output, and/or I2C
 *        device output. Once mapped, a voice can then be trigger by calling
 *        convenient note/pitch based functions (e.g. note(), note_v(), etc.)
 *        and multipass will translate the calls into the appropriate device
 *        specific hardware actions. The discrete device specific hardware
 *        actions (such as set_cv(), set_er301_cv(), set_txo_cv(), etc.) remain
 *        available for use.
 * 
 * @param voice Value used to register and identify the voice. Valid values are
 *        between 0 and MAX_VOICE_COUNT - 1
 * @param device Device identifier value between 0 and MAX_DEVICE_COUNT - 1.
 *        For valid values, see the #define VOICE_… values (e.g. VOICE_CV_GATE)
 *        at the top of this file.
 * @param output Device output that the multipass voice should map to
 * @param on Value indicating if the mapping should be on or off. When set to
 *        off, the device output will not trigger when the voice is updated.
 *        0 mapped voice -> device output is off
 *        1 mapped voice -> device output is on
 * 
 * @see note(), note_v(), note_on(), note_on_v(), note_off(),
 *      set_output_transpose(), set_output_transpose_v(),
 *      set_output_max_volume()
 */
void map_voice(uint8_t voice, uint8_t device, uint8_t output, uint8_t on);

/**
 * @brief Set the output transpose note value for a specific device output, for
 *        use in conjuction with the multipass mapped voice note triggering
 *        functions.
 * 
 * @param device Device identifier value between 0 and MAX_DEVICE_COUNT - 1.
 *        For valid values, see the #define VOICE_… values (e.g. VOICE_CV_GATE)
 *        at the top of this file.
 * @param output The device output to transpose
 * @param note The chromatic note amount to transpose the output by. Standard
 *        MIDI note values apply.
 * 
 * @see map_voice(), note(), note_v(), note_on(), note_on_v(), note_off()
 */
void set_output_transpose(uint8_t device, uint16_t output, uint16_t note);

/**
 * @brief Set the output transpose pitch value for a specific device output, for
 *        use in conjuction with the multipass mapped voice note triggering
 *        functions.
 * 
 * @param device Device identifier value between 0 and MAX_DEVICE_COUNT - 1.
 *        For valid values, see the #define VOICE_… values (e.g. VOICE_CV_GATE)
 *        at the top of this file.
 * @param output Device output to transpose
 * @param pitch Pitch amount to transpose the output by. A valid value is
 *        between -16,384 and 16,383.
 * 
 * @see map_voice(), note(), note_v(), note_on(), note_on_v(), note_off()
 */
void set_output_transpose_v(uint8_t device, uint16_t output, int16_t pitch);

/**
 * @brief Set the output maximum volume for a specific device output, for
 *        use in conjuction with the multipass mapped voice note triggering
 *        functions. Maximum volume applies to devices that support a volume
 *        range: VOICE_ER301, VOICE_JF, and VOICE_TXO_NOTE.
 * 
 * @param device Device identifier value between 0 and MAX_DEVICE_COUNT - 1.
 *        For valid values, see the #define VOICE_… values (e.g. VOICE_CV_GATE)
 *        at the top of this file.
 * @param output Device output to update
 * @param volume Maximum volume value between 0 and MAX_LEVEL
 * 
 * @see map_voice(), note(), note_v(), note_on(), note_on_v(), note_off()
 */
void set_output_max_volume(uint8_t device, uint16_t output, uint16_t volume);


// ----------------------------------------------------------------------------
// i2c / devices

/**
 * @brief Mute or unmute all outputs on a device, for use in conjuction with the
 *        multipass mapped voice note triggering functions.
 * 
 * @param device Device identifier value between 0 and MAX_DEVICE_COUNT - 1.
 *        For valid values, see the #define VOICE_… values (e.g. VOICE_CV_GATE)
 *        at the top of this file.
 * @param mute Value indicating if the mapped voice should be muted or not.
 *        0 device is not muted
 *        1 device is muted
 * 
 * @see map_voice(), note(), note_v(), note_on(), note_on_v(), note_off()
 */
void mute_device(uint8_t device, uint8_t mute);

/**
 * @brief Sets this Monome hardware to be the I2C leader.
 * 
 */
void set_as_i2c_leader(void);

/**
 * @brief Sets this Monome hardware to be an I2C follower with the provided
 *        address.
 * 
 * @param address I2C address used to communicate with this Monome hardware via
 *        I2C
 */
void set_as_i2c_follower(uint8_t address);

/**
 * @brief Set the value of the indicated CV output on a connected ER301 via I2C.
 * 
 * @param output ER301 CV output number
 * @param value CV value represented as a signed integer with a range of
 *        -16,384 (-10v) and 16,383 (+10v)
 */
void set_er301_cv(uint8_t output, int16_t value);

/**
 * @brief Set the value of the indicated CV gate output on a connected ER301 via
 *        I2C.
 * 
 * @param output ER301 CV gate output number
 * @param on 0 CV gate output is low,
 *           1 CV gate output is high
 */
void set_er301_gate(uint8_t output, uint8_t on);

/**
 * @brief Set the mode on a connected Just Friends module via I2C. Used to
 *        switch Just Friends between a default behaviour and an alternate
 *        operating mode that provides access to it's alternate personalities of
 *        Synthesis, a polyphonic synthesizer (while in mode 1, and JF is set to
 *        "sound"), and Geode a rhythm machine (while in mode 1, and JF is set
 *        to "shape").
 * 
 * @param mode 0 default behaviour
 *             1 activates alternate modes with any non-zero value treated as 1
 */
void set_jf_mode(uint8_t mode);

/**
 * @brief Set the value of the indicated CV gate output on a connected Just
 *        Friends via I2C.
 * 
 * @param output Just Friends CV gate output number
 * @param on 0 CV gate output is low,
 *           1 CV gate output is high
 */
void set_jf_gate(uint8_t output, uint8_t on);

/**
 * @brief Set the envelope mode of the indicated output on a connected TELEXo
 *        module via I2C.
 * 
 * @param output TELEXo CV output number
 * @param mode TELEXo CV output envelope mode
 *             0 sets output envelope off and initializes it
 *             1 sets output envelope on
 */
void set_txo_mode(uint8_t output, uint8_t mode);

/**
 * @brief Set the CV value of the indicated output on a connected TELEXo module
 *        via I2C.
 * 
 * @param output TELEXo CV output number
 * @param value CV value represented as a signed integer with a range of
 *        -16,384 (-10v) and 16,383 (+10v)
 */
void set_txo_cv(uint8_t output, int16_t value);

/**
 * @brief Set the CV gate value of the indicated output on a connected TELEXo
 *        module via I2C.
 * 
 * @param output TELEXo CV gate output number
 * @param on 0 CV gate output is low,
 *           1 CV gate output is high
 */
void set_txo_gate(uint8_t output, uint8_t on);

/**
 * @brief Set the envelope attack value of the indicated output on a connected
 *        TELEXo module via I2C.
 * 
 * @param output TELEXo output number
 * @param attack Output envelope attack rate in milliseconds. A valid value is
 *        between 1 and n
 */
void set_txo_attack(uint8_t output, uint16_t attack);

/**
 * @brief Set the envelope decay value of the indicated output on a connected
 *        TELEXo module via I2C.
 * 
 * @param output TELEXo output number
 * @param decay Output envelope decay rate in milliseconds. A valid value is
 *        between 1 and n
 */
void set_txo_decay(uint8_t output, uint16_t decay);

/**
 * @brief Set the waveform of the indicated output on a connected TELEXo module
 *        via I2C.
 * 
 * @param output TELEXo output number
 * @param waveform Desired output waveform. A valid value is between 0 and 325
 *        for TELEXo modules with a Teensy 3.6 and between 0 and 2 for TELEXo
 *        modules with a Teensy 3.2
 */
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

/**
 * @brief Set the debug state. When the debug state is set to on, the print
 *        functions become available to print logs to the
 *        [serial port](https://github.com/monome/libavr32#serial-port) of the
 *        Monome hardware. 
 * 
 * @param on 0 debug is off
 *           1 debug is on
 */
void set_debug(uint8_t on);

/**
 * @brief Print a string to the serial port. Requires the debug state to be set
 *        on.
 * 
 * @param str String value to print
 * 
 * @see set_debug()
 */
void print_debug(const char *str);

/**
 * @brief Print an integer value to the serial port. Requires the debug state to
 *        be set on.
 *        Example output of str = "CV 1", value = 100:
 *        ```
 *        
 *        CV 1 [100]
 *        ```
 * 
 * @param str A string that can be prepended when printing the integer value for
 *        example to describe the value
 * @param value Integer value to print
 * 
 * @see set_debug()
 */
void print_int(const char *str, int16_t value);
