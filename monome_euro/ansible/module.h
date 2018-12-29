// ----------------------------------------------------------------------------
// hardware configuration for ansible
// ----------------------------------------------------------------------------

#pragma once
#include "init_ansible.h"
#include "init_ansible.c"


// ----------------------------------------------------------------------------
// knobs

#define _HARDWARE_KNOB_COUNT 0
const u8 _hardware_knob_ids[_HARDWARE_KNOB_COUNT] = { };


// ----------------------------------------------------------------------------
// buttons (not including the main button, reserved for presets/global config)

#define _HARDWARE_BUTTON_COUNT 2
const u8 _hardware_button_pins[_HARDWARE_BUTTON_COUNT] = { B07, B06 };


// ----------------------------------------------------------------------------
// inputs

#define _POLL_INPUTS 1

#define _HARDWARE_CV_INPUT_COUNT 0
#define _HARDWARE_GATE_INPUT_COUNT 1

#define _HARDWARE_CLOCK_INPUT 1
const u8 _hardware_clock_detect_pin = B10;


// ----------------------------------------------------------------------------
// outputs

#define _HARDWARE_CLOCK_OUTPUT 0
const u8 _hardware_clock_output_pin;

#define _HARDWARE_CV_OUTPUT_COUNT 4
#define _HARDWARE_CV_DAISY_CHAINED 1

#define _HARDWARE_GATE_OUTPUT_COUNT 4
const u8 _hardware_gate_output_pins[_HARDWARE_GATE_OUTPUT_COUNT] =
    { B02, B03, B04, B05 };


// ----------------------------------------------------------------------------
// other

#define _HARDWARE_LED_COUNT 1
