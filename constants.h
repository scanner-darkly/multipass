/**
 * @file constants.h
 * 
 * @brief Constants shared by other files
 * 
 */

#pragma once

#define MAX_LEVEL 16383 /**< Maximum CV level value */


// ----------------------------------------------------------------------------
// voice mapping

#define VOICE_CV_GATE      0 /**< Voice mapping identifier for internal Monome hardware CV/Gate output pair */
#define VOICE_ER301        1 /**< Voice mapping identifier for the [Orthogonal Devices ER-301](http://www.orthogonaldevices.com/er-301) */
#define VOICE_JF           2 /**< Voice mapping identifier for the [Mannequins Just Friends](https://www.whimsicalraps.com/products/just-friends) */
#define VOICE_TXO_NOTE     3 /**< Voice mapping identifier for the [bpcmusic TELEXo](https://github.com/bpcmusic/telex) in sound mode */
#define VOICE_TXO_CV_GATE  4 /**< Voice mapping identifier for the [bpcmusic TELEXo](https://github.com/bpcmusic/telex) CV/Gate mode */
#define VOICE_DISTING_EX   5 /**< Voice mapping identifier for the [Expert Sleepers disting EX](https://www.expert-sleepers.co.uk/distingEX.html) note mode */
#define VOICE_EX_MIDI_1    6 /**< Voice mapping identifier for the [Expert Sleepers disting EX](https://www.expert-sleepers.co.uk/distingEX.html) MIDI mode */
#define VOICE_EX_MIDI_CH   7 /**< Voice mapping identifier for the [Expert Sleepers disting EX](https://www.expert-sleepers.co.uk/distingEX.html) multi channel MIDI mode */
#define VOICE_I2C2MIDI_1   8 /**< Voice mapping identifier for the [attowatt i2c2midi](https://github.com/attowatt/i2c2midi) MIDI mode */
#define VOICE_I2C2MIDI_CH  9 /**< Voice mapping identifier for the [attowatt i2c2midi](https://github.com/attowatt/i2c2midi) multi channel MIDI mode */
#define MAX_DEVICE_COUNT  10 /**< Number of voice mappable devices */


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
