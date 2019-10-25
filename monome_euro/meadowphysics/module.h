// ----------------------------------------------------------------------------
// hardware configuration for meadowphysics
// ----------------------------------------------------------------------------

#pragma once
#include "init_trilogy.h"
#include "init_trilogy.c"


// ----------------------------------------------------------------------------
// knobs

#define _HARDWARE_KNOB_COUNT 1
const u8 _hardware_knob_ids[_HARDWARE_KNOB_COUNT] = { 0 };


// ----------------------------------------------------------------------------
// buttons (not including the main button, reserved for presets/global config)

#define _HARDWARE_BUTTON_COUNT 0
const u8 _hardware_button_pins[_HARDWARE_BUTTON_COUNT];


// ----------------------------------------------------------------------------
// inputs

#define _POLL_INPUTS 0

#define _HARDWARE_CV_INPUT_COUNT 0
const u8 _hardware_cv_input_ids[_HARDWARE_CV_INPUT_COUNT] = { };

#define _HARDWARE_GATE_INPUT_COUNT 0

#define _HARDWARE_CLOCK_INPUT 1
const u8 _hardware_clock_detect_pin = B09;


// ----------------------------------------------------------------------------
// outputs

#define _HARDWARE_CLOCK_OUTPUT 1
const u8 _hardware_clock_output_pin = B10;

#define _HARDWARE_CV_OUTPUT_COUNT 0
#define _HARDWARE_CV_DAISY_CHAINED 0

#define _HARDWARE_GATE_OUTPUT_COUNT 8
#define _HARDWARE_GATE_OUTPUT_PIN 0
const u8 _hardware_gate_output_pins[_HARDWARE_GATE_OUTPUT_COUNT] =
    { B00, B01, B02, B03, B04, B05, B06, B07 };


// ----------------------------------------------------------------------------
// other

#define _HARDWARE_LED_COUNT 0
#define _HARDWARE_SCREEN 0
