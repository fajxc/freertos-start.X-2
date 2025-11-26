/*
 * File:   buttons.h
 * Author: ENCM 511
 * 
 * Button Handling Module Header
 * 
 * Description: Provides button initialization, debouncing, and event detection.
 *              Supports short click and long press detection.
 *              Uses polling-based approach integrated with FreeRTOS task.
 * 
 * Created on Nov 2025
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>
#include "app.h"

/*============================================================================
 * BUTTON STATE STRUCTURE
 * 
 * Tracks the state of each button for debouncing and event detection.
 *============================================================================*/

typedef struct {
    bool current_state;         /* Current debounced state (true = pressed) */
    bool last_state;            /* Previous debounced state */
    bool raw_state;             /* Raw reading from GPIO */
    uint16_t debounce_counter;  /* Debounce timing counter */
    uint16_t press_duration;    /* How long button has been held (ms) */
    bool click_pending;         /* Click event waiting to be sent */
    bool long_press_sent;       /* Long press already sent for this press */
} ButtonState_t;

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Initialize the button handling module
 * 
 * Configures button GPIO pins as inputs and initializes internal state.
 * Call this before using any other button functions.
 */
void Buttons_Init(void);

/**
 * @brief Update button states - call periodically (every ~10ms)
 * 
 * Reads all button GPIO pins, performs debouncing, and detects events.
 * This should be called from a FreeRTOS task at regular intervals.
 * 
 * @param elapsed_ms Time since last update in milliseconds
 */
void Buttons_Update(uint16_t elapsed_ms);

/**
 * @brief Check if PB1 was clicked (short press and release)
 * 
 * @return true if PB1 click detected, false otherwise
 * @note Calling this clears the click flag
 */
bool Buttons_IsPB1Clicked(void);

/**
 * @brief Check if PB2 was clicked
 * 
 * @return true if PB2 click detected, false otherwise
 */
bool Buttons_IsPB2Clicked(void);

/**
 * @brief Check if PB3 was clicked
 * 
 * @return true if PB3 click detected, false otherwise
 */
bool Buttons_IsPB3Clicked(void);

/**
 * @brief Check if both PB2 and PB3 are currently pressed together
 * 
 * @return true if both buttons are currently pressed
 */
bool Buttons_ArePB2AndPB3Pressed(void);

/**
 * @brief Check if PB3 long press detected
 * 
 * @return true if PB3 held for >1 second
 * @note Only returns true once per long press
 */
bool Buttons_IsPB3LongPress(void);

/**
 * @brief Check if PB2+PB3 long press detected
 * 
 * @return true if both buttons held for >1 second
 */
bool Buttons_IsPB2AndPB3LongPress(void);

/**
 * @brief Check if PB2+PB3 click detected (both pressed and released together)
 * 
 * @return true if both buttons clicked together (within time window)
 */
bool Buttons_IsPB2AndPB3Click(void);

/**
 * @brief Get the current pressed state of PB1
 * 
 * @return true if PB1 is currently pressed (debounced)
 */
bool Buttons_IsPB1Pressed(void);

/**
 * @brief Get the current pressed state of PB2
 * 
 * @return true if PB2 is currently pressed (debounced)
 */
bool Buttons_IsPB2Pressed(void);

/**
 * @brief Get the current pressed state of PB3
 * 
 * @return true if PB3 is currently pressed (debounced)
 */
bool Buttons_IsPB3Pressed(void);

/**
 * @brief Clear all pending button events
 * 
 * Use this when transitioning states to avoid spurious events.
 */
void Buttons_ClearEvents(void);

/**
 * @brief Get and send the next button event to the queue
 * 
 * Checks for any pending button events and sends them to xButtonQueue.
 * 
 * @return true if an event was sent, false otherwise
 */
bool Buttons_SendPendingEvents(void);

#endif /* BUTTONS_H */

