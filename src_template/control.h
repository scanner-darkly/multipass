/**
 * @file control.h
 * 
 * @brief Defines functions for Multipass to send events to the controller (grid
 *        presses etc). Defines functions for engine to send updates (note on,
 *        etc.). Defines data structures for Multipass preset management.
 * 
 */

#pragma once
#include "types.h"


// ----------------------------------------------------------------------------
// firmware dependent stuff starts here


// ----------------------------------------------------------------------------
// shared types

/**
 * @brief Preset meta data which is associated with a preset. This structure is
 *        useful for storing things like a glyph or a note that will help
 *        describe or display a preset, but is not the preset itself.
 * 
 * @see preset_data_t
 */
typedef struct {
} preset_meta_t;

/**
 * @brief Shared application data, useful for storing things like settings and
 *        modes of the hardware, global application state, and things that
 *        should not change when switching presets. For example I2C
 *        leader/follower mode, application operating modes, or maybe optional
 *        override settings like a global transpose/scale. 
 * 
 */
typedef struct {
} shared_data_t;

/**
 * @brief Preset data used to store values describing the current state of the
 *        application and capable of being stored and loaded to/from flash. Used
 *        along with `preset_meta_t` and `shared_data_t`, these data structures
 *        encapsulate the majority of statefulness of the application. Useful
 *        for storing things like patterns/sequences, voices, mappings, volume,
 *        timings, pitch, etc.
 * 
 * @see preset_meta_t
 * @see shared_data_t
 */
typedef struct {
} preset_data_t;


// ----------------------------------------------------------------------------
// firmware settings/variables main.c needs to know


// ----------------------------------------------------------------------------
// functions control.c needs to implement (will be called from main.c)

/**
 * @brief Called implicitly by Multipass at startup if the hardware flash is
 *        new and there are no presets stored to flash. This function is the
 *        applications opportunity to initialize the flash with application
 *        appropriate preset structs and shared data in default state. Once
 *        initialized, Multipass will no longer call this function at startup.
 * 
 * @see is_flash_new()
 * @see store_preset_to_flash()
 * @see store_shared_data_to_flash()
 * @see store_preset_index()
 * @see preset_data_t
 * @see preset_meta_t
 * @see shared_data_t
 */
void init_presets(void);

/**
 * @brief Called implicitly by Multipass at application startup. This function
 *        is the applications opportunity to load shared data, load a preset and
 *        meta data, set up any initial application values, and set up timers.
 * 
 * @see load_shared_data_from_flash()
 * @see load_preset_from_flash()
 * @see load_preset_meta_from_flash()
 */
void init_control(void);

/**
 * @brief Implement event handling code for your application here. This function
 *        is called by Multipass when an event happens. Here your application
 *        has the opportunity to respond to these events. See `interface.h`
 *        events macro definition section for a list of event identifier macros,
 *        data, and length documentation.
 * 
 * @param event Event identifier
 * @param data Pointer to event data
 * @param length Length of event data
 */
void process_event(u8 event, u8 *data, u8 length);

/**
 * @brief Implement Monome grid LED rendering code for your application here. If
 *        your application does not use the Monome grid, you can leave this
 *        function blank. This function is called by Multipass at a rate of
 *        MONOME_REFRESH_INTERVAL if a Monome grid is connected to the hardware.
 * 
 */
void render_grid(void);

/**
 * @brief Implement Monome arc LED rendering code for your application here. If
 *        your application does not use the Monome arc, you can leave this
 *        function blank. This function is called by Multipass at a rate of
 *        MONOME_REFRESH_INTERVAL if a Monome arc is connected to the hardware.
 * 
 */
void render_arc(void);


// ----------------------------------------------------------------------------
// functions engine needs to call
