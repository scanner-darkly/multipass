// ----------------------------------------------------------------------------
// hardware configuration for ansible
// ----------------------------------------------------------------------------

#pragma once
#include "init_teletype.h"
#include "init_teletype.c"


// ----------------------------------------------------------------------------
// knobs

#define _HARDWARE_KNOB_COUNT 1
const u8 _hardware_knob_ids[_HARDWARE_KNOB_COUNT] = { 1 };


// ----------------------------------------------------------------------------
// buttons (not including the main button, reserved for presets/global config)

#define _HARDWARE_POLL_FRONT_BUTTON 0
#define _HARDWARE_BUTTON_COUNT 0
const u8 _hardware_button_pins[_HARDWARE_BUTTON_COUNT] = { };


// ----------------------------------------------------------------------------
// inputs

#define _POLL_INPUTS 1

#define _HARDWARE_CV_INPUT_COUNT 1
const u8 _hardware_cv_input_ids[_HARDWARE_CV_INPUT_COUNT] = { 0 };

#define _HARDWARE_GATE_INPUT_COUNT 8

#define _HARDWARE_CLOCK_INPUT 0
const u8 _hardware_clock_detect_pin;


// ----------------------------------------------------------------------------
// outputs

#define _HARDWARE_CLOCK_OUTPUT 0
const u8 _hardware_clock_output_pin;

#define _HARDWARE_CV_OUTPUT_COUNT 4
#define _HARDWARE_CV_DAISY_CHAINED 1

#define _HARDWARE_GATE_OUTPUT_COUNT 4
#define _HARDWARE_GATE_OUTPUT_PIN 1
const u8 _hardware_gate_output_pins[_HARDWARE_GATE_OUTPUT_COUNT] =
    { B08, B09, B10, B11 };


// ----------------------------------------------------------------------------
// other

#define _HARDWARE_LED_COUNT 0
#define _HARDWARE_SCREEN 1
