// ----------------------------------------------------------------------------
// multipass implementation for monome eurorack modules
//
// implements functions that provide access to the hardware (inputs, outputs, 
// knobs, MIDI, grid, arc etc) as defined in interface.h
//
// sends hardware events to controller
//
// provides preset management for persistent memory (flash/USB)
//
// to support new devices define appropriate functions in interface.h and 
// implement them here
//
// based on monome eurorack code: https://github.com/monome
// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>

// asf
#include "delay.h"
#include "compiler.h"
#include "flashc.h"
#include "preprocessor.h"
#include "print_funcs.h"
#include "intc.h"
#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "string.h"
#include "sysclk.h"

// skeleton
#include "types.h"
#include "events.h"
#include "hid.h"
#include "i2c.h"
#include "init_common.h"
#include "midi.h"
#include "midi_common.h"
#include "monome.h"
#include "music.h"
#include "timers.h"
#include "adc.h"
#include "util.h"
#include "cdc.h"
#include "ftdi.h"
#include "twi.h"
#include "dac.h"
#include "interrupts.h"
#include "font.h"
#include "screen.h"
#include "region.h"

// this
#include "conf_board.h"
#include "ii.h"

#include "module.h"
#include "interface.h"
#include "control.h"


// ----------------------------------------------------------------------------
// defines

#define ADC_POLL_INTERVAL 100
#define INPUTS_POLL_INTERVAL 50
#define HID_POLL_INTERVAL 48
#define MIDI_POLL_INTERVAL 8
#define MONOME_POLL_INTERVAL 20
#define MONOME_REFRESH_INTERVAL 30
#define I2C_REFRESH_INTERVAL 50

#define FRONT_BUTTON_HOLD_TIME 1200
#define GRID_HOLD_TIME 750

#define ARC_MAX_ENCODER_COUNT 4
#define ARC_ENCODER_SENSITIVITY 20

#define SHNTH_BAR_COUNT 4
#define SHNTH_ANTENNA_COUNT 2

#define ADC_COUNT 4
#define MAX_CV_COUNT 4
#define MAX_GATE_COUNT 8

#define FIRSTRUN_KEY 0x22
#define PRESET_COUNT 16
#define TIMED_EVENT_COUNT 100
#define SCREEN_LINE_COUNT 8
#define MAX_EVENT_DATA_LENGTH 16

#define MAX_VOICES_COUNT 32 // max number of voices available for voice mapping
#define MAX_OUTPUT_COUNT 8 // max number of device outputs that can be assigned to the same voice

#define MAX_ER301_OUTPUT_COUNT 16
#define MAX_JF_OUTPUT_COUNT 6
#define MAX_TXO_OUTPUT_COUNT 16 // up to 4 devices x 4 outputs each
#define MAX_DISTING_EX_OUTPUT_COUNT 32 // up to 4 devices x 8 voices each
#define MAX_EX_MIDI_1_OUTPUT_COUNT 16 // up to 16 "outputs" for 1 channel MIDI mode
#define MAX_EX_MIDI_CH_OUTPUT_COUNT 16 // up to 16 MIDI channels for multi channel MIDI mode
#define MAX_I2C2MIDI_1_OUTPUT_COUNT 16 // up to 16 "outputs" for 1 channel MIDI mode
#define MAX_I2C2MIDI_CH_OUTPUT_COUNT 16 // up to 16 MIDI channels for multi channel MIDI mode

#define MAX_ER301_COUNT 100
#define MAX_TXI_COUNT 16

#define TO_TR 0x00
#define TO_CV_SET 0x11
#define TO_OSC_SET 0x41
#define TO_ENV_ACT 0x60
#define TO_ENV 0x6D
#define TO_ENV_ATT 0x61
#define TO_ENV_DEC 0x64
#define TO_OSC_WAVE 0x4A

#define I2C2MIDI 0x3F


// ----------------------------------------------------------------------------
// variables

static struct {
    softTimer_t timer;
    u8 repeat;
} event_timers[TIMED_EVENT_COUNT];

static struct grid_data_t {
    u8 connected;
    u8 column_count;
    u8 row_count;
    u8 is_vb;
    u8 held_x;
    u8 held_y;
} grid;

static struct arc_data_t {
    u8 connected;
    u8 encoder_count;
    s16 delta[ARC_MAX_ENCODER_COUNT];
} arc;

typedef enum {
    hid_keyboard,
    hid_shnth,
    hid_ps3
} hid_devices;

static struct hid_data_t {
    u8 connected;
    hid_devices device;
    u8 frame[HID_FRAME_MAX_BYTES];
    u8 mod_key;
    u8 key;
    u8 shnth_init_bars;
    u8 shnth_init_antennas;
    s8 shnth_bars[SHNTH_BAR_COUNT];
    s8 shnth_antennas[SHNTH_ANTENNA_COUNT];
} hid;

static struct txo_refresh_t {
    u8 attack_dirty;
    u8 decay_dirty;
    u8 waveform_dirty;
    u16 attack;
    u16 decay;
    u16 waveform;
} txo_refresh[MAX_TXO_OUTPUT_COUNT];

static u8 _debug = 0;
static u8 control_initialized;
static u8 event_data[MAX_EVENT_DATA_LENGTH];

static u8 adc_timer, hid_poll_timer, inputs_poll_timer, i2c_refresh_timer, midi_poll_timer;

static region screen_lines[SCREEN_LINE_COUNT];

static u8 monome_dirty;
static softTimer_t monome_poll_timer, monome_refresh_timer;

static u8 midi_connected;
static midi_behavior_t midi_behavior;

static softTimer_t grid_hold_timer;
static softTimer_t front_button_hold_timer;

static u8 external_clock_connected;
static u16 adc_values[ADC_COUNT];

static u8 front_button_pressed;
static u8 button_pressed[_HARDWARE_BUTTON_COUNT];

static u8 gate_input_values[_HARDWARE_GATE_INPUT_COUNT];

static s16 cv_values[MAX_CV_COUNT];
static u8 voice_maps[MAX_VOICES_COUNT][MAX_DEVICE_COUNT][MAX_OUTPUT_COUNT >> 3];
static u16 device_on[MAX_DEVICE_COUNT];

static u8 txo_mode[MAX_TXO_OUTPUT_COUNT];
static u16 er301_max_volume[MAX_ER301_OUTPUT_COUNT];
static u16 jf_max_volume[MAX_JF_OUTPUT_COUNT];
static u16 txo_max_volume[MAX_TXO_OUTPUT_COUNT];
static u16 disting_ex_max_volume[MAX_DISTING_EX_OUTPUT_COUNT];
static u16 ex_midi_1_max_volume[MAX_EX_MIDI_1_OUTPUT_COUNT];
static u16 ex_midi_ch_max_volume[MAX_EX_MIDI_CH_OUTPUT_COUNT];
static u16 i2c2midi_1_max_volume[MAX_I2C2MIDI_1_OUTPUT_COUNT];
static u16 i2c2midi_ch_max_volume[MAX_I2C2MIDI_CH_OUTPUT_COUNT];

static s16 cv_transpose[MAX_CV_COUNT];
static s16 er301_transpose[MAX_ER301_OUTPUT_COUNT];
static s16 jf_transpose[MAX_JF_OUTPUT_COUNT];
static s16 txo_transpose[MAX_TXO_OUTPUT_COUNT];
static s16 disting_ex_transpose[MAX_DISTING_EX_OUTPUT_COUNT];
static s16 ex_midi_1_transpose[MAX_EX_MIDI_1_OUTPUT_COUNT];
static s16 ex_midi_ch_transpose[MAX_EX_MIDI_CH_OUTPUT_COUNT];
static s16 i2c2midi_1_transpose[MAX_I2C2MIDI_1_OUTPUT_COUNT];
static s16 i2c2midi_ch_transpose[MAX_I2C2MIDI_CH_OUTPUT_COUNT];

static u8 is_i2c_leader;
static u8 i2c_follower_address;
static u8 jf_mode;

static s16 last_pitch[MAX_VOICES_COUNT];

// NVRAM data structure located in the flash array
// preset_data is defined in control.h
typedef const struct {
    u8 initialized;
    u8 preset_index;
    preset_meta_t meta[PRESET_COUNT];
    preset_data_t presets[PRESET_COUNT];
    shared_data_t shared;
} flash_data_t;

__attribute__((__section__(".flash_nvram")))
static flash_data_t flash;


// ----------------------------------------------------------------------------
// prototypes

static void _print_str(const char *str);
static void _print_s16_var(const char *str, s16 var);

static void control_event(u8 event, u8 length);
static void event_timer_callback(void* o);
static void grid_hold_callback(void* o);
static void front_button_hold_callback(void* o);
static void handler_none(s32 data) { ;; }

static u8 _is_voice_mapped(u8 voice, u8 device, u8 output);

static void _send_note(u8 output, s16 pitch, u16 volume);
static void _set_cv(u8 output, s16 value);
static void _set_gate(u8 output, u8 on);

static void _send_er301_note(u8 output, s16 pitch, u16 volume);
static void _set_er301_cv(u8 output, s16 value);
static void _set_er301_gate(u8 output, u8 on);

static void _set_jf_mode(u8 mode);
static void _set_jf_gate(u8 output, u8 on);
static void _send_jf_note(u8 output, s16 pitch, u16 volume);

static void _send_txo_command(u8 output, u8 command, s16 value);
static void _set_txo_mode(u8 output, u8 mode);
static void _set_txo_cv(u8 output, s16 value);
static void _set_txo_gate(u8 output, u8 on);
static void _send_txo_note(u8 output, s16 pitch, u16 volume);
static int16_t _get_txi_value(u8 index, bool shift);

static void _send_disting_ex_note(u8 output, s16 pitch, u16 volume);
static void _send_ex_midi_1_note(u8 output, s16 pitch, u16 volume);
static void _send_ex_midi_ch_note(u8 output, s16 pitch, u16 volume);

static void _send_i2c2midi_1_note(u8 output, s16 pitch, u16 volume);
static void _send_i2c2midi_ch_note(u8 output, s16 pitch, u16 volume);

static int _i2c_leader_tx(uint8_t addr, uint8_t *data, uint8_t length);
static int _i2c_leader_rx(uint8_t addr, uint8_t *data, uint8_t length);
static void _set_i2c_mode(u8 leader);


// ----------------------------------------------------------------------------
// helper functions

void _print_str(const char *str) {
    if (!_debug) return;

    print_dbg("\r\n");
    print_dbg(str);
}

void _print_s16_var(const char *str, s16 var) {
    if (!_debug) return;

    print_dbg("\r\n");
    print_dbg(str);
    print_dbg(" [");
    s16 value;
    if (var < 0) {
        print_dbg("-");
        value = -var;
    } else {
        value = var;
    }
    print_dbg_char_hex(value >> 8);
    print_dbg_char_hex(value & 0xff);
    print_dbg("]");
}


// ----------------------------------------------------------------------------
// implementation for interface.h

// timers

void add_timed_event(u8 index, u16 ms, u8 repeat) {
    if (index > TIMED_EVENT_COUNT) return;
    
    timer_remove(&event_timers[index].timer);
    event_timers[index].repeat = repeat;
    timer_add(&event_timers[index].timer, ms, &event_timer_callback, (void *)(uintptr_t)index);
}

void stop_timed_event(u8 index) {
    if (index > TIMED_EVENT_COUNT) return;

    timer_remove(&event_timers[index].timer);
}

void update_timer_interval(uint8_t index, uint16_t ms) {
    if (index > TIMED_EVENT_COUNT) return;

    event_timers[index].timer.ticks = ms;
}

// clock

u64 get_global_time() {
    return get_ticks();
}

uint8_t has_clock_input() {
    return _HARDWARE_CLOCK_INPUT;
}

u8 is_external_clock_connected() {
    return external_clock_connected;
}

void set_clock_output(u8 on) {
    if (!_HARDWARE_CLOCK_OUTPUT) return;
    if (on)
        gpio_set_gpio_pin(_hardware_clock_output_pin);
    else
        gpio_clr_gpio_pin(_hardware_clock_output_pin);
}

// inputs

u8 get_cv_input_count() {
    return _HARDWARE_CV_INPUT_COUNT;
}

s16 get_cv(u8 index) {
    if (index >= _HARDWARE_CV_INPUT_COUNT || index >= ADC_COUNT) return 0;
    return adc_values[_hardware_cv_input_ids[index]] << 2;
}

u8 get_gate_input_count() {
    return _HARDWARE_GATE_INPUT_COUNT;
}

u8 get_gate(u8 index) {
    if (index >= _HARDWARE_GATE_INPUT_COUNT) return 0;
    return gate_input_values[index];
}

// outputs

u8 get_cv_output_count(void) {
    return _HARDWARE_CV_OUTPUT_COUNT;   
}

void set_cv(u8 output, s16 value) {
    _set_cv(output, value);
}

u8 get_gate_output_count(void) {
    return _HARDWARE_GATE_OUTPUT_COUNT;
}

void set_gate(u8 output, u8 on) {
    _set_gate(output, on);
}

// controls

u8 get_button_count() {
    return _HARDWARE_BUTTON_COUNT;
}

u8 is_button_pressed(u8 index) {
    if (index >= _HARDWARE_BUTTON_COUNT) return 0;
    return button_pressed[index];
}

u8 get_knob_count() {
    return _HARDWARE_KNOB_COUNT;
}

u16 get_knob_value(u8 index) {
    if (index >= _HARDWARE_KNOB_COUNT || index >= ADC_COUNT) return 0;
    return adc_values[_hardware_knob_ids[index]] << 4;
}

// grid

u8 is_grid_connected() {
    return grid.connected;
}

u8 get_grid_column_count() {
    return grid.column_count;
}

u8 get_grid_row_count() {
    return grid.row_count;
}

u8 is_grid_vb() {
    return grid.is_vb;
}

void clear_all_grid_leds() {
    for (u16 i = 0; i < MONOME_MAX_LED_BYTES; i++)
        monomeLedBuffer[i] = 0;
}

u8 get_grid_led(u8 x, u8 y) {
    u16 index = (y << 4) + x;
    return index < MONOME_MAX_LED_BYTES ? monomeLedBuffer[index] : 0;
}

void set_grid_led(u8 x, u8 y, u8 level) {
    u16 index = (y << 4) + x;
    if (index < MONOME_MAX_LED_BYTES)
        monomeLedBuffer[index] = level;
}

void set_grid_led_i(u16 index, u8 level) {
    if (index < MONOME_MAX_LED_BYTES)
        monomeLedBuffer[index] = level;
}

void refresh_grid() {
    monome_dirty = 1;
}

// arc

u8 is_arc_connected() {
    return arc.connected;
}

u8 get_arc_encoder_count() {
    return arc.encoder_count;
}

void clear_all_arc_leds() {
    for (u16 i = 0; i < MONOME_MAX_LED_BYTES; i++)
        monomeLedBuffer[i] = 0;

}

u8 get_arc_led(u8 enc, u8 led) {
    u16 index = (enc << 6) + led;
    return index < MONOME_MAX_LED_BYTES ? monomeLedBuffer[index] : 0;
}

void set_arc_led(u8 enc, u8 led, u8 level) {
    u16 index = (enc << 6) + led;
    if (index < MONOME_MAX_LED_BYTES)
        monomeLedBuffer[index] = level;
}

void refresh_arc() {
    monome_dirty = 1;
}

// midi

u8 is_midi_connected() {
    return midi_connected;
}

// notes

u16 note_to_pitch(u16 note) {
    u32 pitch = ((u32)note * 16384) / 60;
    pitch = (pitch > 1) + (pitch & 1);
    return pitch;
}

u16 pitch_to_note(u16 pitch) {
    u32 note = ((s32)pitch * 240) / 16384;
    note = (note > 1) + (note & 1);
    return note;
}

static u8 _is_voice_mapped(u8 voice, u8 device, u8 output) {
    return (voice_maps[voice][device][output >> 3] & (1 << (output & 7))) && device_on[device];
}

void note(u8 voice, u16 note, u16 volume, u8 on) {
    if (on)
        note_on(voice, note, volume);
    else
        note_off(voice);
}

void note_v(u8 voice, s16 pitch, u16 volume, u8 on) {
    if (on)
        note_on_v(voice, pitch, volume);
    else
        note_off(voice);
}

void note_on(u8 voice, u16 note, u16 volume) {
    note_on_v(voice, note_to_pitch(note), volume);
}

void note_on_v(u8 voice, s16 pitch, u16 volume) {
    if (voice >= MAX_VOICES_COUNT) return;
    
    last_pitch[voice] = pitch;

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_CV_GATE, output))
            _send_note(output, pitch, volume);

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_ER301, output))
            _send_er301_note(output, pitch, volume);
            
    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_JF, output))
            _send_jf_note(output, pitch, volume);
            
    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_TXO_NOTE, output)) {
            _send_txo_note(output, pitch, volume);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_TXO_CV_GATE, output)) {
            _set_txo_cv(output, pitch);
            _set_txo_gate(output, 1);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_DISTING_EX, output)) {
            _send_disting_ex_note(output, pitch, volume);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_EX_MIDI_1, output)) {
            _send_ex_midi_1_note(output, pitch, volume);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_EX_MIDI_CH, output)) {
            _send_ex_midi_ch_note(output, pitch, volume);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_I2C2MIDI_1, output)) {
            _send_i2c2midi_1_note(output, pitch, volume);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_I2C2MIDI_CH, output)) {
            _send_i2c2midi_ch_note(output, pitch, volume);
        }
}

void note_off(u8 voice) {
    if (voice >= MAX_VOICES_COUNT) return;

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_CV_GATE, output))
            _send_note(output, last_pitch[voice], 0);
        
    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_ER301, output))
            _send_er301_note(output, last_pitch[voice], 0);
        
    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_JF, output))
            _send_jf_note(output, last_pitch[voice], 0);
        
    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_TXO_NOTE, output))
            _send_txo_note(output, last_pitch[voice], 0);
        
    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_TXO_CV_GATE, output))
            _set_txo_gate(output, 0);

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_DISTING_EX, output)) {
            _send_disting_ex_note(output, last_pitch[voice], 0);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_EX_MIDI_1, output)) {
            _send_ex_midi_1_note(output, last_pitch[voice], 0);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_EX_MIDI_CH, output)) {
            _send_ex_midi_ch_note(output, last_pitch[voice], 0);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_I2C2MIDI_1, output)) {
            _send_i2c2midi_1_note(output, last_pitch[voice], 0);
        }

    for (u8 output = 0; output < MAX_OUTPUT_COUNT; output++)
        if (_is_voice_mapped(voice, VOICE_I2C2MIDI_CH, output)) {
            _send_i2c2midi_ch_note(output, last_pitch[voice], 0);
        }
}

void map_voice(u8 voice, u8 device, u8 output, u8 on) {
    if (voice >= MAX_VOICES_COUNT || device >= MAX_DEVICE_COUNT || output > MAX_OUTPUT_COUNT) return;
    if (on)
        voice_maps[voice][device][output >> 3] |= 1 << (output & 7);
    else
        voice_maps[voice][device][output >> 3] &= ~(1 << (output & 7));
}

void set_output_transpose(u8 device, u16 output, u16 note) {
    set_output_transpose_v(device, output, note_to_pitch(note));
}

void set_output_transpose_v(u8 device, u16 output, s16 pitch) {
    if (device == VOICE_CV_GATE) {
        if (output < MAX_CV_COUNT) cv_transpose[output] = pitch;
    } else if (device == VOICE_ER301) {
        if (output < MAX_ER301_OUTPUT_COUNT) er301_transpose[output] = pitch;
    } else if (device == VOICE_JF) {
        if (output < MAX_JF_OUTPUT_COUNT) jf_transpose[output] = pitch;
    } else if (device == VOICE_TXO_CV_GATE || device == VOICE_TXO_NOTE) {
        if (output < MAX_TXO_OUTPUT_COUNT) txo_transpose[output] = pitch;
    } else if (device == VOICE_DISTING_EX) {
        if (output < MAX_DISTING_EX_OUTPUT_COUNT) disting_ex_transpose[output] = pitch;
    } else if (device == VOICE_EX_MIDI_1) {
        if (output < MAX_EX_MIDI_1_OUTPUT_COUNT) ex_midi_1_transpose[output] = pitch;
    } else if (device == VOICE_EX_MIDI_CH) {
        if (output < MAX_EX_MIDI_CH_OUTPUT_COUNT) ex_midi_ch_transpose[output] = pitch;
    } else if (device == VOICE_I2C2MIDI_1) {
        if (output < MAX_I2C2MIDI_1_OUTPUT_COUNT) i2c2midi_1_transpose[output] = pitch;
    } else if (device == VOICE_I2C2MIDI_CH) {
        if (output < MAX_I2C2MIDI_CH_OUTPUT_COUNT) i2c2midi_ch_transpose[output] = pitch;
    }
}

void set_output_max_volume(u8 device, u16 output, u16 volume) {
    if (device == VOICE_ER301) {
        if (output < MAX_ER301_OUTPUT_COUNT) er301_max_volume[output] = volume;
    } else if (device == VOICE_JF) {
        if (output < MAX_JF_OUTPUT_COUNT) jf_max_volume[output] = volume;
    } else if (device == VOICE_TXO_NOTE) {
        if (output < MAX_TXO_OUTPUT_COUNT) txo_max_volume[output] = volume;
    } else if (device == VOICE_DISTING_EX) {
        if (output < MAX_DISTING_EX_OUTPUT_COUNT) disting_ex_max_volume[output] = volume;
    } else if (device == VOICE_EX_MIDI_1) {
        if (output < MAX_EX_MIDI_1_OUTPUT_COUNT) ex_midi_1_max_volume[output] = volume;
    } else if (device == VOICE_EX_MIDI_CH) {
        if (output < MAX_EX_MIDI_CH_OUTPUT_COUNT) ex_midi_ch_max_volume[output] = volume;
    } else if (device == VOICE_I2C2MIDI_1) {
        if (output < MAX_I2C2MIDI_1_OUTPUT_COUNT) i2c2midi_1_max_volume[output] = volume;
    } else if (device == VOICE_I2C2MIDI_CH) {
        if (output < MAX_I2C2MIDI_CH_OUTPUT_COUNT) i2c2midi_ch_max_volume[output] = volume;
    }
}

// i2c

void mute_device(u8 device, u8 mute) {
    if (device >= MAX_DEVICE_COUNT) return;
    device_on[device] = !mute;
}

void set_as_i2c_leader() {
    _set_i2c_mode(1);
}

void set_as_i2c_follower(u8 address) {
    i2c_follower_address = address;
    _set_i2c_mode(0);
}

void set_er301_cv(u8 output, s16 value) {
    _set_er301_cv(output, value);
}

void set_er301_gate(u8 output, u8 on) {
    _set_er301_gate(output, on);
}

void set_jf_mode(u8 mode) {
    _set_jf_mode(mode & 1);
}

void set_jf_gate(u8 output, u8 on) {
    _set_jf_gate(output, on);
}

void set_txo_mode(u8 output, u8 mode) {
    _set_txo_mode(output, mode);
}

void set_txo_cv(u8 output, s16 value) {
    _set_txo_cv(output, value);
}

void set_txo_gate(u8 output, u8 on) {
    _set_txo_gate(output, on);
}

void set_txo_attack(u8 output, u16 attack) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    txo_refresh[output].attack = attack;
    txo_refresh[output].attack_dirty = 1;
}

void set_txo_decay(u8 output, u16 decay) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    txo_refresh[output].decay = decay;
    txo_refresh[output].decay_dirty = 1;
}

void set_txo_waveform(u8 output, u16 waveform) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    txo_refresh[output].waveform = waveform;
    txo_refresh[output].waveform_dirty = 1;
}

// flash

u8 is_flash_new() {
    return flash.initialized != FIRSTRUN_KEY;
}

u8 get_preset_index() {
    return flash.preset_index;
}

u8 get_preset_count() {
    return PRESET_COUNT;
}

void store_preset_to_flash(u8 index, preset_meta_t *meta, preset_data_t *preset) {
    flashc_memset8((void*)&(flash.initialized), FIRSTRUN_KEY, 1, true);
    flashc_memcpy((void *)&flash.meta[index], meta, sizeof(preset_meta_t), true);
    flashc_memcpy((void *)&flash.presets[index], preset, sizeof(preset_data_t), true);
}

void store_preset_index(u8 index) {
    flashc_memset8((void*)&(flash.preset_index), index, 1, true);
}

void store_shared_data_to_flash(shared_data_t *shared) {
    flashc_memcpy((void *)&flash.shared, shared, sizeof(shared_data_t), true);
}

void load_preset_from_flash(u8 index, preset_data_t *preset) {
    memcpy(preset, &flash.presets[index], sizeof(preset_data_t));
}

void load_preset_meta_from_flash(u8 index, preset_meta_t *meta) {
    memcpy(meta, &flash.meta[index], sizeof(preset_meta_t));
}

void load_shared_data_from_flash(shared_data_t *shared) {
    memcpy(shared, &flash.shared, sizeof(shared_data_t));
}

// screen

void clear_screen() {
    if (_HARDWARE_SCREEN == 0) return;
    for (u8 i = 0; i < SCREEN_LINE_COUNT; i++) region_fill(&screen_lines[i], 0);
}

void fill_line(uint8_t line, uint8_t colour) {
    if (_HARDWARE_SCREEN == 0) return;
    if (line >= SCREEN_LINE_COUNT) return;
    region_fill(&screen_lines[line], colour);
}

void draw_str(const char* str, uint8_t line, uint8_t colour, uint8_t background) {
    if (_HARDWARE_SCREEN == 0) return;
    if (line >= SCREEN_LINE_COUNT) return;
    region_fill(&screen_lines[line], background);
    font_string_region_clip(&screen_lines[line], str, 0, 0, colour, background);
}

void refresh_screen() {
    if (_HARDWARE_SCREEN == 0) return;
    for (u8 i = 0; i < SCREEN_LINE_COUNT; i++) region_draw(&screen_lines[i]);
}

// other

void set_led(u8 index, u8 level) {
    if (index >= _HARDWARE_LED_COUNT) return;

    // ansible only, so hardcoded
    if (level == 0) {
        gpio_clr_gpio_pin(B00);
        gpio_clr_gpio_pin(B01);
    } else if (level == 1) {
        gpio_set_gpio_pin(B00);
        gpio_clr_gpio_pin(B01);
    } else if (level == 2) {
        gpio_clr_gpio_pin(B00);
        gpio_set_gpio_pin(B01);
    } else {
        gpio_set_gpio_pin(B00);
        gpio_set_gpio_pin(B01);
    }
}

void set_debug(u8 on) {
    _debug = on;
}

void print_debug(const char *str) {
    _print_str(str);
}

void print_int(const char *str, s16 value) {
    _print_s16_var(str, value);
}


// ----------------------------------------------------------------------------
// control events

void event_timer_callback(void* o) {
    u16 index = (uintptr_t)o;
    if (!event_timers[index].repeat) timer_remove(&event_timers[index].timer);
    
    event_data[0] = index;
    control_event(TIMED_EVENT, 1);
}

void control_event(u8 event, u8 length) {
    if (!control_initialized) return;
    process_event(event, event_data, length);
}


// ----------------------------------------------------------------------------
// i2c device helpers

int _i2c_leader_tx(u8 addr, u8 *data, u8 length) {
    if (!is_i2c_leader) return 0;
    return i2c_leader_tx(addr, data, length);
}

int _i2c_leader_rx(uint8_t addr, uint8_t *data, uint8_t l) {
    if (!is_i2c_leader) return 0;
    return i2c_leader_rx(addr, data, l);
}

void _send_note(u8 output, s16 pitch, u16 volume) {
    // boundaries will be enforced by _set_cv and _set_gate
    if (volume) {
        pitch += cv_transpose[output];
        _set_cv(output, pitch);
        _set_gate(output, 1);
    } else {
        _set_gate(output, 0);
    }
}

void _set_cv(u8 output, s16 value) {
    if (output >= _HARDWARE_CV_OUTPUT_COUNT || output >= MAX_CV_COUNT) return;
    cv_values[output] = value;
    u16 norm = (value < 0 ? 0 : value) >> 2;
    
    if (_HARDWARE_CV_DAISY_CHAINED) {
        dac_set_value_noslew(output, value);
        dac_update_now(); // will send all 4!
    } else {
        if (output == 0) {
            u8 irq_flags = irqs_pause();
            spi_selectChip(DAC_SPI, DAC_SPI_NPCS);
            spi_write(DAC_SPI, 0x31);
            spi_write(DAC_SPI, norm >> 4);
            spi_write(DAC_SPI, norm << 4);
            spi_unselectChip(DAC_SPI, DAC_SPI_NPCS);
            irqs_resume(irq_flags);
        } else if (output == 1) {
            u8 irq_flags = irqs_pause();
            spi_selectChip(DAC_SPI, DAC_SPI_NPCS);
            spi_write(DAC_SPI, 0x38);
            spi_write(DAC_SPI, norm >> 4);
            spi_write(DAC_SPI, norm << 4);
            spi_unselectChip(SPI,DAC_SPI_NPCS);
            irqs_resume(irq_flags);
        }
    }
}

void _set_gate(u8 output, u8 on) {
    if (output >= _HARDWARE_GATE_OUTPUT_COUNT || output >= MAX_GATE_COUNT) return;
    if (on) {
        if (_HARDWARE_GATE_OUTPUT_PIN)
            gpio_set_pin_high(_hardware_gate_output_pins[output]);
        else
            gpio_set_gpio_pin(_hardware_gate_output_pins[output]);
    } else {
        if (_HARDWARE_GATE_OUTPUT_PIN)
            gpio_set_pin_low(_hardware_gate_output_pins[output]);
        else
            gpio_clr_gpio_pin(_hardware_gate_output_pins[output]);
    }
}

void _set_i2c_mode(u8 leader) {
    if (leader && !is_i2c_leader) {
        init_i2c_leader();
        is_i2c_leader = 1;
    } else if (!leader && is_i2c_leader) {
        is_i2c_leader = 0;
        _set_jf_mode(0);
        if (i2c_follower_address) init_i2c_follower(i2c_follower_address);
    }
}

void _send_er301_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_ER301_OUTPUT_COUNT) return;
    if (volume) {
        u32 vol = (u32)volume * (u32)er301_max_volume[output] / MAX_LEVEL;
        pitch += er301_transpose[output] - 3277;
        _set_er301_cv(output, pitch);
         // using 2nd set for volume
        _set_er301_cv(output + MAX_ER301_OUTPUT_COUNT, vol);
        _set_er301_gate(output, 1);
    } else {
        _set_er301_gate(output, 0);
    }
}

void _set_er301_cv(u8 output, s16 value) {
    if (output >= MAX_ER301_COUNT) return;
    
    u8 d[] = { TO_CV_SET, output, (u16)value >> 8, value & 0xff };
    _i2c_leader_tx(ER301_1, d, 4);
}

void _set_er301_gate(u8 output, u8 on) {
    if (output >= MAX_ER301_COUNT) return;
    
    u8 d[] = { TO_TR, output, 0, on & 1 };
    _i2c_leader_tx(ER301_1, d, 4);
    _i2c_leader_tx(ER301_1, d, 4);
}

void _set_jf_mode(u8 mode) {
    if (mode && !jf_mode) {
        jf_mode = 1;
        u8 d[] = { JF_MODE, 1 };
        _i2c_leader_tx(JF_ADDR, d, 2);
    } else if (!mode && jf_mode) {
        jf_mode = 0;
        u8 d[] = { JF_MODE, 0 };
        _i2c_leader_tx(JF_ADDR, d, 2);
    }
}

void _send_jf_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_JF_OUTPUT_COUNT) return;

    u32 vol = (u32)volume * (u32)jf_max_volume[output] / MAX_LEVEL;
    pitch += jf_transpose[output] - 3277;
    u8 d[] = { JF_VOX, output + 1, (u16)pitch >> 8, pitch & 0xff, (u16)vol >> 8, vol & 0xff };
    _i2c_leader_tx(JF_ADDR, d, 6);
    
    // this should only be needed if volume is 0
    // but for some reason it works better if it's done note on as well
    _set_jf_gate(output, vol > 0);
}

void _set_jf_gate(u8 output, u8 on) {
    if (output >= MAX_JF_OUTPUT_COUNT) return;
    
    uint8_t d[] = { JF_TR, output + 1, on & 1 };
    _i2c_leader_tx(JF_ADDR, d, 3);
}

// all txo comm should be done through this as it safeguards
void _send_txo_command(u8 output, u8 command, s16 value) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    
    u8 address = TELEXO + (output >> 2);
    u8 port = output & 0b11;

    u8 d[] = { command, port, (u16)value >> 8, value & 0xff };
    _i2c_leader_tx(address, d, 4);
}

void _set_txo_mode(u8 output, u8 mode) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    // if (txo_mode[output] == mode) return;
    
    if (mode) {
        _send_txo_command(output, TO_ENV_ACT, 1);
    } else {
        _send_txo_command(output, TO_ENV_ACT, 0);
        _send_txo_command(output, TO_OSC_SET, 0);
    }
    txo_mode[output] = mode;
}

void _send_txo_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    _set_txo_mode(output, 1);
    
    if (volume) {
        u32 vol = (u32)volume * (u32)txo_max_volume[output] / MAX_LEVEL;
        pitch += txo_transpose[output] + 4915;
        _send_txo_command(output, TO_OSC_SET, pitch);
        _send_txo_command(output, TO_CV_SET, vol);
        _send_txo_command(output, TO_ENV, 1);
    } else {
        // _send_txo_command(output, TO_OSC_SET, 0);
        // _send_txo_command(output, TO_CV_SET, 0);
        _send_txo_command(output, TO_ENV, 0);
    }
}

void _set_txo_cv(u8 output, s16 value) {
    _set_txo_mode(output, 0);
    _send_txo_command(output, TO_CV_SET, value);
}

void _set_txo_gate(u8 output, u8 on) {
    if (output >= MAX_TXO_OUTPUT_COUNT) return;
    
    _send_txo_command(output, TO_ENV, 0);
    _send_txo_command(output, TO_TR, on & 1);
}

int16_t _get_txi_value(uint8_t index, bool shift) {
    // send request to read
    uint8_t port = index & 3;
    uint8_t device = index >> 2;
    uint8_t address = TELEXI + device;
    port += shift ? 4 : 0;
    uint8_t buffer[2];
    buffer[0] = port;
    _i2c_leader_tx(address, buffer, 1);
    
    // now read
    buffer[0] = 0;
    buffer[1] = 0;
    _i2c_leader_rx(address, buffer, 2);
    return (buffer[0] << 8) + buffer[1];
}

int16_t get_txi_input(uint8_t input) {
    if (input >= MAX_TXI_COUNT) return 0;
    return _get_txi_value(input, true);
}

uint16_t get_txi_param(uint8_t param) {
    if (param >= MAX_TXI_COUNT) return 0;
    // shift to bring it to same range as get_knob_value
    return _get_txi_value(param, false) << 2;
}

void _send_disting_ex_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_DISTING_EX_OUTPUT_COUNT) return;

    u32 vol = (u32)volume * (u32)disting_ex_max_volume[output] / MAX_LEVEL;
    pitch += disting_ex_transpose[output] - 3277;
    u8 note = pitch_to_note(pitch) + 48;
    if (note > 127) note = 127;
    
    // 8 channels per disting device
    u8 address = DISTING_EX_1 + (output >> 3);
    u8 channel = output & 7;
    
    u8 d_note_off[] = { 0x6A, channel, note };
    _i2c_leader_tx(address, d_note_off, 3);
    
    if (vol) {
        u8 d_pitch[] = { 0x68, channel, note, (u16)pitch >> 8, pitch & 0xff };
        _i2c_leader_tx(address, d_pitch, 5);
        
        u8 d_note_on[] = { 0x69, channel, note, (u16)vol >> 8, vol & 0xff };
        _i2c_leader_tx(address, d_note_on, 5);
    }
}

void _send_ex_midi_1_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_EX_MIDI_1_OUTPUT_COUNT) return;

    u32 vol = (u32)volume * (u32)ex_midi_1_max_volume[output] / MAX_LEVEL;
    pitch += ex_midi_1_transpose[output];
    u8 note = pitch_to_note(pitch);
    
    if (vol) {
        u8 d_note[] = { 0x4F, 0x90, note, (u16)vol >> 7 };
        _i2c_leader_tx(DISTING_EX_1, d_note, 4);
    } else {
        u8 d_note[] = { 0x4F, 0x80, note, 0 };
        _i2c_leader_tx(DISTING_EX_1, d_note, 4);
    }
}

void _send_ex_midi_ch_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_EX_MIDI_CH_OUTPUT_COUNT) return;

    u32 vol = (u32)volume * (u32)ex_midi_ch_max_volume[output] / MAX_LEVEL;
    pitch += ex_midi_ch_transpose[output];
    u8 note = pitch_to_note(pitch);
    
    if (vol) {
        u8 d_note[] = { 0x4F, 0x90 + output, note, (u16)vol >> 7 };
        _i2c_leader_tx(DISTING_EX_1, d_note, 4);
    } else {
        u8 d_note[] = { 0x4F, 0x80 + output, note, 0 };
        _i2c_leader_tx(DISTING_EX_1, d_note, 4);
    }
}

void _send_i2c2midi_1_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_EX_MIDI_1_OUTPUT_COUNT) return;

    u32 vol = (u32)volume * (u32)ex_midi_1_max_volume[output] / MAX_LEVEL;
    pitch += i2c2midi_1_transpose[output];
    u8 note = pitch_to_note(pitch);
    
    if (vol) {
        u8 d_note[] = { 20, 0, output + 10, (u16)vol >> 7 };
        _i2c_leader_tx(I2C2MIDI, d_note, 4);
    } else {
        u8 d_note[] = { 21, 0, output + 20 };
        _i2c_leader_tx(I2C2MIDI, d_note, 3);
    }
}

void _send_i2c2midi_ch_note(u8 output, s16 pitch, u16 volume) {
    if (output >= MAX_EX_MIDI_CH_OUTPUT_COUNT) return;

    u32 vol = (u32)volume * (u32)ex_midi_ch_max_volume[output] / MAX_LEVEL;
    pitch += i2c2midi_ch_transpose[output];
    u8 note = pitch_to_note(pitch);
    
    if (vol) {
        u8 d_note[] = { 20, output, note, (u16)vol >> 7 };
        _i2c_leader_tx(I2C2MIDI, d_note, 4);
    } else {
        u8 d_note[] = { 21, output, note };
        _i2c_leader_tx(I2C2MIDI, d_note, 3);
    }
}

// ----------------------------------------------------------------------------
// input handlers

static void handler_clock_ext(s32 data) {
    event_data[0] = 1;
    event_data[1] = data;
    control_event(MAIN_CLOCK_RECEIVED, 2);
}

static void handler_clock_normal(s32 data) {
    external_clock_connected = !gpio_get_pin_value(_hardware_clock_detect_pin); 
    event_data[0] = external_clock_connected;
    control_event(MAIN_CLOCK_SWITCHED, 1);
}

static void handler_tr(s32 data) {
    if (data < 2) {
        event_data[0] = 1;
        event_data[1] = data;
        control_event(MAIN_CLOCK_RECEIVED, 2);
    } else {
        // gate input, only one on ansible so hardcoding
        if (_HARDWARE_GATE_INPUT_COUNT) gate_input_values[0] = data & 1;
        event_data[0] = 0;
        event_data[1] = data & 1;
        control_event(GATE_RECEIVED, 2);
    }
}

static void poll_inputs(void) {
    if (!_POLL_INPUTS) return;
    
    u8 pressed;
    for (u8 i = 0; i < _HARDWARE_BUTTON_COUNT; i++) {
        pressed = !gpio_get_pin_value(_hardware_button_pins[i]);
        if (button_pressed[i] != pressed) {
            button_pressed[i] = pressed;
            event_data[0] = i;
            event_data[1] = pressed;
            control_event(BUTTON_PRESSED, 2);
        }
    }
    
    if (_HARDWARE_CLOCK_INPUT && external_clock_connected != !gpio_get_pin_value(_hardware_clock_detect_pin)) {
        event_t e = { .type = kEventClockNormal };
        event_post(&e);
    }
    
    if (_HARDWARE_POLL_FRONT_BUTTON) {
        if (front_button_pressed != !gpio_get_pin_value(NMI)) {
            event_t e = { .type = kEventFront, .data = gpio_get_pin_value(NMI) };
            event_post(&e);
        }
    }
}


// ----------------------------------------------------------------------------
// front button handlers

static void handler_front(s32 data) {
    front_button_pressed = !data;
    
    timer_remove(&front_button_hold_timer);
    if (front_button_pressed)
        timer_add(
            &front_button_hold_timer, 
            FRONT_BUTTON_HOLD_TIME, 
            front_button_hold_callback, 
            NULL);
    
    event_data[0] = front_button_pressed;
    control_event(FRONT_BUTTON_PRESSED, 1);
}

void front_button_hold_callback(void* o) {
    timer_remove(&front_button_hold_timer);
    if (!front_button_pressed) return;
    
    control_event(FRONT_BUTTON_HELD, 0);
}


// ----------------------------------------------------------------------------
// monome/ftdi handlers

static void monome_poll_callback(void* obj) {
    serial_read();
}

static void monome_refresh_callback(void* obj) {
    if (monome_dirty) {
        static event_t e;
        e.type = kEventMonomeRefresh;
        event_post(&e);
    }
}

static void handler_ftdi_connect(s32 data) {
    ftdi_setup();
}


static void handler_serial_connect(s32 data) {
    monome_setup_mext();
}

static void handler_ftdi_disconnect(s32 data) {
    timer_remove(&monome_poll_timer );
    timer_remove(&monome_refresh_timer );
    timer_remove(&grid_hold_timer);
    
    u8 is_grid = grid.connected;
    grid.connected = arc.connected = 0;

    event_data[0] = 0;
    control_event(is_grid ? GRID_CONNECTED : ARC_CONNECTED, 1);
}

static void handler_monome_connect(s32 data) {
    if (monome_device() == eDeviceArc) {
        arc.encoder_count = monome_encs();
        for (u8 i = 0; i < ARC_MAX_ENCODER_COUNT; i++) arc.delta[i] = 0;
        arc.connected = 1;
        event_data[0] = 1;
        control_event(ARC_CONNECTED, 1);
    } else {
        timer_remove(&grid_hold_timer);
        grid.column_count = monome_size_x();
        grid.row_count = monome_size_y();
        grid.is_vb = monome_is_vari();
        grid.connected = 1;
        event_data[0] = 1;
        control_event(GRID_CONNECTED, 1);
    }
    monome_dirty = 1;
    timer_add(&monome_poll_timer, MONOME_POLL_INTERVAL, &monome_poll_callback, NULL );
    timer_add(&monome_refresh_timer, MONOME_REFRESH_INTERVAL, &monome_refresh_callback, NULL );
}

static void handler_monome_poll(s32 data) {
    monome_read_serial();
}

static void handler_monome_refresh(s32 data) {
    if (grid.connected)
        render_grid();
    else if (arc.connected)
        render_arc();
    monome_dirty = 0;
    monomeFrameDirty = 0b1111;
    (*monome_refresh)();
}

static void handler_monome_grid_key(s32 data) {
    u8 x, y, z;
    monome_grid_key_parse_event_data(data, &x, &y, &z);
    
    if (z) {
        grid.held_x = x;
        grid.held_y = y;
        timer_remove(&grid_hold_timer);
        timer_add(&grid_hold_timer, GRID_HOLD_TIME, grid_hold_callback, NULL);
    } else if (grid.held_x == x && grid.held_y == y) {
        timer_remove(&grid_hold_timer);
    }

    event_data[0] = x;
    event_data[1] = y;
    event_data[2] = z;
    control_event(GRID_KEY_PRESSED, 3);
}

void grid_hold_callback(void* o) {
    timer_remove(&grid_hold_timer);
    
    event_data[0] = grid.held_x;
    event_data[1] = grid.held_y;
    control_event(GRID_KEY_HELD, 2);
}

static void handler_monome_ring_enc(s32 data) {
    u8 n;
    s8 delta;
    monome_ring_enc_parse_event_data(data, &n, &delta);
    
    if (n >= ARC_MAX_ENCODER_COUNT) return;
    
    event_data[0] = n;
    event_data[1] = delta;
    control_event(ARC_ENCODER_FINE, 2);
    
    if (delta > 0){
        if (arc.delta[n] > 0) arc.delta[n] += delta; else arc.delta[n] = delta;
    } else {
        if (arc.delta[n] < 0) arc.delta[n] += delta; else arc.delta[n] = delta;
    }
    
    if (abs(arc.delta[n]) > ARC_ENCODER_SENSITIVITY) {
        arc.delta[n] = 0;
        event_data[1] = delta > 0;
        control_event(ARC_ENCODER_COARSE, 2);
    }
}


// ----------------------------------------------------------------------------
// midi handlers

static void handler_midi_connect(s32 data) {
    event_data[0] = 1;
    control_event(MIDI_CONNECTED, 1);
    midi_connected = 1;
}

static void handler_midi_disconnect(s32 data) {
    midi_connected = 0;
    event_data[0] = 0;
    control_event(MIDI_CONNECTED, 1);
}

static void handler_standard_midi_packet(s32 data) {
    midi_packet_parse(&midi_behavior, (u32)data);
}

static void midi_note_on(u8 ch, u8 num, u8 vel) {
    event_data[0] = ch;
    event_data[1] = num;
    event_data[2] = vel;
    event_data[3] = 1;
    control_event(MIDI_NOTE, 4);
}

static void midi_note_off(u8 ch, u8 num, u8 vel) {
    event_data[0] = ch;
    event_data[1] = num;
    event_data[2] = vel;
    event_data[3] = 0;
    control_event(MIDI_NOTE, 4);
}

static void midi_control_change(u8 ch, u8 num, u8 val) {
    event_data[0] = ch;
    event_data[1] = num;
    event_data[2] = val;
    control_event(MIDI_CC, 3);
}

static void midi_aftertouch(u8 ch, u8 num, u8 val) {
    event_data[0] = ch;
    event_data[1] = num;
    event_data[2] = val;
    control_event(MIDI_AFTERTOUCH, 3);
}


// ----------------------------------------------------------------------------
// hid handlers

static void handler_hid_connect(int32_t data) {
    hid.connected = 1;
    event_data[0] = 1;

    uhc_device_t* dev = (uhc_device_t*)(data);
    
    if (dev->dev_desc.idProduct == 0x6666) {
        hid.device = hid_shnth;
        control_event(SHNTH_CONNECTED, 1);
        hid.shnth_init_bars = hid.shnth_init_antennas = 1;
    } else if (dev->dev_desc.idVendor == 0x4C05) {
        hid.device = hid_ps3;
    } else {
        hid.device = hid_keyboard;
        hid.mod_key = hid.key = 0;
        control_event(KEYBOARD_CONNECTED, 1);
    }
    
    if (_debug) {
        _print_str("\r\n");
        _print_s16_var("bcdDevice", dev->dev_desc.bcdDevice);
        _print_s16_var("bcdUSB", dev->dev_desc.bcdUSB);
        _print_s16_var("bDescriptorType", dev->dev_desc.bDescriptorType);
        _print_s16_var("bDeviceClass", dev->dev_desc.bDeviceClass);
        _print_s16_var("bDeviceProtocol", dev->dev_desc.bDeviceProtocol);
        _print_s16_var("bDeviceSubClass", dev->dev_desc.bDeviceSubClass);
        _print_s16_var("bLength", dev->dev_desc.bLength);
        _print_s16_var("bMaxPacketSize0", dev->dev_desc.bMaxPacketSize0);
        _print_s16_var("bNumConfigurations", dev->dev_desc.bNumConfigurations);
        _print_s16_var("idProduct", dev->dev_desc.idProduct);
        _print_s16_var("idVendor", dev->dev_desc.idVendor);
        _print_s16_var("iManufacturer", dev->dev_desc.iManufacturer);
        _print_s16_var("iProduct", dev->dev_desc.iProduct);
    }
}

static void handler_hid_disconnect(int32_t data) {
    hid.connected = 0;
    event_data[0] = 0;
    if (hid.device == hid_shnth) {
        control_event(SHNTH_CONNECTED, 1);
    } else {
        control_event(KEYBOARD_CONNECTED, 1);
    }
}

static void process_hid(void) {
    if (!hid.connected) return;
    
    const s8* frame = (const s8*)hid_get_frame_data();
    u16 value;
    s16 delta;
    
    if (hid.device == hid_shnth) {
        
        // bars start at 0, when pressed go up to 127
        // then down to -128 then back to 0
        for (u8 i = 0; i < SHNTH_BAR_COUNT; i++) {
            delta = abs(frame[i] - hid.shnth_bars[i]);
            if (hid.shnth_init_bars || (delta > 2 && delta < 0x30)) {
                hid.shnth_init_bars = 0;
                hid.shnth_bars[i] = frame[i];
                value = (u16)(128 + frame[i]);
                if (value > 255) value = 255;
                event_data[0] = i;
                event_data[1] = value & 0xff;
                control_event(SHNTH_BAR, 2);
            }
        }
        
        // if holding shnth with buttons facing you, main button on top
        // antenna 0 is on the left, antenna 1 is on the right
        // antenna range seems to be around 0 when away
        // and -128 with palm right on it
        for (u8 i = 0; i < SHNTH_ANTENNA_COUNT; i++) {
            delta = abs(frame[i + 4] - hid.shnth_antennas[i]);
            if (hid.shnth_init_antennas || (delta > 2 && delta < 0x30)) {
                hid.shnth_init_antennas = 0;
                hid.shnth_antennas[i] = frame[i + 4];
               // get it into 0..255 range
                value = abs(hid.shnth_antennas[i]) << 1;
                if (value > 255) value = 255;
                event_data[0] = i;
                event_data[1] = value;
                control_event(SHNTH_ANTENNA, 2);
            }
        }
        
        u8 bit;
        for (u8 i = 0; i < 8; i++) {
            bit = 1 << i;
            if ((frame[7] & bit) != (hid.frame[7] & bit)) {
                event_data[0] = i;
                event_data[1] = frame[7] & bit;
                control_event(SHNTH_BUTTON, 2);
            }
        }
        hid.frame[7] = frame[7];
    }
    
    else if (hid.device == hid_keyboard) {
        hid.mod_key = frame[0];
        for (u8 i = 2; i < 8; i++) {
            if (frame[i] == 0) {
                if (i == 2) {
                    event_data[0] = hid.mod_key;
                    event_data[1] = hid.key;
                    event_data[2] = 0;
                    control_event(KEYBOARD_KEY, 3);
                    hid.key = 0;
                }
            }
            else if (hid.frame[i] != frame[i]) {
                hid.key = frame[i];
                event_data[0] = hid.mod_key;
                event_data[1] = hid.key;
                event_data[2] = 1;
                control_event(KEYBOARD_KEY, 3);
            }
            hid.frame[i] = frame[i];
        }
    }
}


// ----------------------------------------------------------------------------
// i2c handlers

static void process_i2c(uint8_t *data, uint8_t length) {
    for (uint8_t i = 0; i < min(length, MAX_EVENT_DATA_LENGTH); i++)
        event_data[i] = data[i];
    control_event(I2C_RECEIVED, length);
}

static void refresh_i2c(void) {
    u8 updated = 0;
    for (u8 i = 0; i < MAX_TXO_OUTPUT_COUNT; i++) {
        if (txo_refresh[i].attack_dirty) {
            _send_txo_command(i, TO_ENV_ATT, txo_refresh[i].attack);
            txo_refresh[i].attack_dirty = 0;
            updated = 1;
        }
        if (txo_refresh[i].decay_dirty) {
            _send_txo_command(i, TO_ENV_DEC, txo_refresh[i].decay);
            txo_refresh[i].decay_dirty = 0;
            updated = 1;
        }
        if (txo_refresh[i].waveform_dirty) {
            _send_txo_command(i, TO_OSC_WAVE, txo_refresh[i].waveform);
            txo_refresh[i].waveform_dirty = 0;
            updated = 1;
        }
        // if (updated) break;
    }
}


// ----------------------------------------------------------------------------
// init / main

static inline void assign_main_event_handlers(void) {
    for (u16 i = 0; i < kNumEventTypes; i++)
        app_event_handlers[i] = &handler_none;
    
    app_event_handlers[kEventFront]            = &handler_front;
    app_event_handlers[kEventClockNormal]      = &handler_clock_normal;
    app_event_handlers[kEventClockExt]         = &handler_clock_ext;
    app_event_handlers[kEventTr]               = &handler_tr;
    app_event_handlers[kEventTrigger]          = &handler_tr;
    
    app_event_handlers[kEventFtdiConnect]      = &handler_ftdi_connect;
    app_event_handlers[kEventFtdiDisconnect]   = &handler_ftdi_disconnect;
    app_event_handlers[kEventSerialConnect]    = &handler_serial_connect;
    app_event_handlers[kEventSerialDisconnect] = &handler_ftdi_disconnect;

    app_event_handlers[kEventMonomeConnect]    = &handler_monome_connect;
    app_event_handlers[kEventMonomeDisconnect] = &handler_none;
    app_event_handlers[kEventMonomeRefresh]    = &handler_monome_refresh;
    app_event_handlers[kEventMonomePoll]       = &handler_monome_poll;
    app_event_handlers[kEventMonomeGridKey]    = &handler_monome_grid_key;
    app_event_handlers[kEventMonomeRingEnc]    = &handler_monome_ring_enc;
    
    app_event_handlers[kEventMidiConnect]      = &handler_midi_connect;
    app_event_handlers[kEventMidiDisconnect]   = &handler_midi_disconnect;
    app_event_handlers[kEventMidiPacket]       = &handler_standard_midi_packet;
    app_event_handlers[kEventHidConnect]       = &handler_hid_connect;
    app_event_handlers[kEventHidDisconnect]    = &handler_hid_disconnect;
}

static void setup_dacs(void) {
    if (_HARDWARE_CV_DAISY_CHAINED) init_dacs();
}

static void initialize_control(void) {
    if (is_flash_new()) init_presets();
    init_control();
    control_initialized = 1;
}

static void process_system_events(void) {
    u64 ticks = get_ticks();
    
    if (ticks - adc_timer > ADC_POLL_INTERVAL) {
        adc_timer = ticks;
        adc_convert(&adc_values);
    }
    
    if (_POLL_INPUTS && (ticks - inputs_poll_timer > INPUTS_POLL_INTERVAL)) {
        poll_inputs();
        inputs_poll_timer = ticks;
    }
    
    if (midi_connected && (ticks - midi_poll_timer > MIDI_POLL_INTERVAL)) {
        midi_read();
        midi_poll_timer = ticks;
    }
    
    if (hid.connected && (ticks - hid_poll_timer > HID_POLL_INTERVAL)) {
        process_hid();
        hid_poll_timer = ticks;
    }

    if (ticks - i2c_refresh_timer > I2C_REFRESH_INTERVAL) {
        refresh_i2c();
        i2c_refresh_timer = ticks;
    }
}

static void init_state(void) {
    control_initialized = 0;
    
    // timers
    
    adc_timer = hid_poll_timer = inputs_poll_timer = i2c_refresh_timer = midi_poll_timer = 0;
    
    // event timers
    
    for (u16 i = 0; i < TIMED_EVENT_COUNT; i++) {
        event_timers[i].timer.next = NULL;
        event_timers[i].timer.prev = NULL;
    }

    // grid
    
    grid.connected = 0;
    grid.column_count = 16;
    grid.row_count = 8;
    grid.is_vb = 1;
    grid.held_x = 0;
    grid.held_y = 0;
    
    // arc
    
    arc.connected = 0;
    arc.encoder_count = 4;
    for (u8 i = 0; i < ARC_MAX_ENCODER_COUNT; i++) arc.delta[i] = 0;

    // midi
    
    midi_connected = 0;
    
    midi_behavior.note_on = &midi_note_on;
    midi_behavior.note_off = &midi_note_off;
    midi_behavior.channel_pressure = NULL;
    midi_behavior.pitch_bend = NULL;
    midi_behavior.control_change = &midi_control_change;
    midi_behavior.clock_tick = NULL;
    midi_behavior.seq_start = NULL;
    midi_behavior.seq_stop = NULL;
    midi_behavior.seq_continue = NULL;
    midi_behavior.panic = NULL;
    midi_behavior.aftertouch = &midi_aftertouch;

    // hid

    hid.connected = 0;
    for (u8 i = 0; i < HID_FRAME_MAX_BYTES; i++) hid.frame[i] = 0;
    for (u8 i = 0; i < SHNTH_BAR_COUNT; i++) hid.shnth_bars[i] = 0x80;
    for (u8 i = 0; i < SHNTH_ANTENNA; i++) hid.shnth_antennas[i] = 0;
    hid.shnth_init_bars = hid.shnth_init_antennas = 1;

    // voices
    
    for (u8 i = 0; i < MAX_VOICES_COUNT; i++)
        for (u8 j = 0; j < MAX_DEVICE_COUNT; j++)
            for (u8 k = 0; k < MAX_OUTPUT_COUNT/8; k++)
                voice_maps[i][j][k] = 0;
                
    for (u8 i = 0; i < max(_HARDWARE_CV_OUTPUT_COUNT, _HARDWARE_GATE_OUTPUT_COUNT); i++)
        map_voice(i, VOICE_CV_GATE, i, 1);
    
    for (u8 i = 0; i < MAX_VOICES_COUNT; i++)
        last_pitch[i] = 0;

    // devices 
    
    for (u8 i = 0; i < MAX_DEVICE_COUNT; i++) device_on[i] = 1;
    
    for (u8 i = 0; i < MAX_TXO_OUTPUT_COUNT; i++) txo_mode[i] = 2;
    for (u8 i = 0; i < MAX_ER301_OUTPUT_COUNT; i++) er301_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_JF_OUTPUT_COUNT; i++) jf_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_TXO_OUTPUT_COUNT; i++) txo_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_DISTING_EX_OUTPUT_COUNT; i++) disting_ex_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_EX_MIDI_1_OUTPUT_COUNT; i++) ex_midi_1_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_EX_MIDI_CH_OUTPUT_COUNT; i++) ex_midi_ch_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_I2C2MIDI_1_OUTPUT_COUNT; i++) i2c2midi_1_max_volume[i] = MAX_LEVEL;
    for (u8 i = 0; i < MAX_I2C2MIDI_CH_OUTPUT_COUNT; i++) i2c2midi_ch_max_volume[i] = MAX_LEVEL;
    
    for (u8 i = 0; i < MAX_CV_COUNT; i++) cv_transpose[i] = 0;
    for (u8 i = 0; i < MAX_ER301_OUTPUT_COUNT; i++) er301_transpose[i] = 0;
    for (u8 i = 0; i < MAX_JF_OUTPUT_COUNT; i++) jf_transpose[i] = 0;
    for (u8 i = 0; i < MAX_TXO_OUTPUT_COUNT; i++) txo_transpose[i] = 0;
    for (u8 i = 0; i < MAX_DISTING_EX_OUTPUT_COUNT; i++) disting_ex_transpose[i] = 0;
    for (u8 i = 0; i < MAX_EX_MIDI_1_OUTPUT_COUNT; i++) ex_midi_1_transpose[i] = 0;
    for (u8 i = 0; i < MAX_EX_MIDI_CH_OUTPUT_COUNT; i++) ex_midi_ch_transpose[i] = 0;
    for (u8 i = 0; i < MAX_I2C2MIDI_1_OUTPUT_COUNT; i++) i2c2midi_1_transpose[i] = 0;
    for (u8 i = 0; i < MAX_I2C2MIDI_CH_OUTPUT_COUNT; i++) i2c2midi_ch_transpose[i] = 0;

    // txo refresh
    
    for (u8 i = 0; i < MAX_TXO_OUTPUT_COUNT; i++) {
        txo_refresh[i].attack_dirty = 0;
        txo_refresh[i].decay_dirty = 0;
        txo_refresh[i].waveform_dirty = 0;
    }
    
    // i2c
    
    is_i2c_leader = 0;
    i2c_follower_address = 0;
    jf_mode = 0;
}

static void init_hardware(void) {
    // inputs

    if (_HARDWARE_CLOCK_INPUT) 
        external_clock_connected = !gpio_get_pin_value(_hardware_clock_detect_pin);

    adc_convert(&adc_values);
    front_button_pressed = 0;

    for (u8 i = 0; i < _HARDWARE_BUTTON_COUNT; i++)
        button_pressed[i] = !gpio_get_pin_value(_hardware_button_pins[i]);
    
    for (u8 i = 0; i < _HARDWARE_GATE_INPUT_COUNT; i++)
        gate_input_values[i] = 0;
    
    // outputs
    
    for (u8 i = 0; i < min(_HARDWARE_CV_OUTPUT_COUNT, MAX_CV_COUNT); i++)
        _set_cv(i, 0);
    
    for (u8 i = 0; i < min(_HARDWARE_GATE_OUTPUT_COUNT, MAX_GATE_COUNT); i++)
        _set_gate(i, 0);
    
    set_clock_output(0);

    // screen
    
    if (_HARDWARE_SCREEN) {
        for (u8 i = 0; i < SCREEN_LINE_COUNT; i++) {
            screen_lines[i].w = 128;
            screen_lines[i].h = 8;
            screen_lines[i].x = 0;
            screen_lines[i].y = i << 3;
            region_alloc(&screen_lines[i]);
        }
    }
}

int main(void) {
    init_state();
    
    sysclk_init();
    init_dbg_rs232(FMCK_HZ);
    init_gpio();
    
    assign_main_event_handlers();
    init_events();
    init_tc();
    init_spi();
    init_adc();

    irq_initialize_vectors();
    register_interrupts();
    cpu_irq_enable();

    setup_dacs();    
    init_usb_host();
    init_monome();
    
    if (_HARDWARE_SCREEN) init_oled();
    process_ii = &process_i2c;

    init_hardware();
    initialize_control();

    event_t e;
    while (true) {
        process_system_events();
        if (event_next(&e)) (app_event_handlers)[e.type](e.data);
    };
}
