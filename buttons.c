/*
 * File:   buttons.c
 * Author: ENCM 511
 * 
 * Button Handling Module Implementation
 * 
 * Description: Implements button debouncing using a counter-based approach.
 *              Detects short clicks and long presses.
 *              Designed to be called periodically from a FreeRTOS task.
 * 
 * Debouncing Algorithm:
 *   - Sample button at regular intervals (~10ms)
 *   - Only change debounced state after DEBOUNCE_COUNT consistent readings
 *   - This filters out mechanical bounce noise
 * 
 * Event Detection:
 *   - Click: Button pressed then released (short press)
 *   - Long Press: Button held for > LONG_PRESS_THRESHOLD_MS
 * 
 * Created on Nov 2025
 */

#include "buttons.h"
#include "hw_config.h"
#include "app.h"

/*============================================================================
 * CONFIGURATION CONSTANTS
 *============================================================================*/

/* Number of consistent readings required for debounce (at 10ms update rate) */
#define DEBOUNCE_COUNT          5       /* 50ms debounce time */

/* Long press threshold in milliseconds */
#define LONG_PRESS_THRESHOLD_MS 1000

/* Time window for detecting simultaneous button press (ms) */
#define COMBO_WINDOW_MS         200

/*============================================================================
 * STATIC VARIABLES
 *============================================================================*/

/* Button state tracking */
static ButtonState_t pb1_state;
static ButtonState_t pb2_state;
static ButtonState_t pb3_state;

/* Combo button tracking */
static bool pb2_pb3_combo_click_pending = false;
static bool pb2_pb3_combo_long_press_sent = false;
static uint16_t pb2_pb3_combo_duration = 0;
static bool pb2_was_pressed_in_combo = false;
static bool pb3_was_pressed_in_combo = false;

/*============================================================================
 * STATIC HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Update a single button's debounced state
 * 
 * @param state Pointer to button state structure
 * @param raw_reading Current raw GPIO reading (true = pressed)
 * @param elapsed_ms Time since last update
 */
static void UpdateButtonState(ButtonState_t *state, bool raw_reading, uint16_t elapsed_ms)
{
    /* Store raw reading */
    state->raw_state = raw_reading;
    
    /* Debouncing logic */
    if (raw_reading == state->current_state) {
        /* Reading matches current state, reset debounce counter */
        state->debounce_counter = 0;
    } else {
        /* Reading differs, increment debounce counter */
        state->debounce_counter++;
        
        /* Check if we've had enough consistent different readings */
        if (state->debounce_counter >= DEBOUNCE_COUNT) {
            /* State change confirmed */
            state->last_state = state->current_state;
            state->current_state = raw_reading;
            state->debounce_counter = 0;
            
            if (state->current_state) {
                /* Button just pressed */
                state->press_duration = 0;
                state->long_press_sent = false;
            } else {
                /* Button just released */
                if (!state->long_press_sent) {
                    /* Short press - this is a click */
                    state->click_pending = true;
                }
                state->press_duration = 0;
            }
        }
    }
    
    /* Update press duration if button is held */
    if (state->current_state) {
        state->press_duration += elapsed_ms;
    }
}

/**
 * @brief Check if long press threshold reached for a button
 * 
 * @param state Pointer to button state structure
 * @return true if long press detected (and not already sent)
 */
static bool CheckLongPress(ButtonState_t *state)
{
    if (state->current_state && 
        state->press_duration >= LONG_PRESS_THRESHOLD_MS &&
        !state->long_press_sent) {
        state->long_press_sent = true;
        state->click_pending = false;  /* Cancel click if long press */
        return true;
    }
    return false;
}

/*============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

void Buttons_Init(void)
{
    /* Configure GPIO pins as inputs with pull-ups */
    PB1_Init();
    PB2_Init();
    PB3_Init();
    
    /* Small delay to let pull-ups stabilize */
    volatile uint16_t i;
    for (i = 0; i < 1000; i++) { }
    
    /* Initialize button state structures with ACTUAL current state */
    /* This prevents false triggers at startup */
    pb1_state.current_state = PB1_Read();
    pb1_state.last_state = pb1_state.current_state;
    pb1_state.raw_state = pb1_state.current_state;
    pb1_state.debounce_counter = 0;
    pb1_state.press_duration = 0;
    pb1_state.click_pending = false;
    pb1_state.long_press_sent = false;
    
    pb2_state.current_state = PB2_Read();
    pb2_state.last_state = pb2_state.current_state;
    pb2_state.raw_state = pb2_state.current_state;
    pb2_state.debounce_counter = 0;
    pb2_state.press_duration = 0;
    pb2_state.click_pending = false;
    pb2_state.long_press_sent = false;
    
    pb3_state.current_state = PB3_Read();
    pb3_state.last_state = pb3_state.current_state;
    pb3_state.raw_state = pb3_state.current_state;
    pb3_state.debounce_counter = 0;
    pb3_state.press_duration = 0;
    pb3_state.click_pending = false;
    pb3_state.long_press_sent = false;
    
    /* Initialize combo tracking */
    pb2_pb3_combo_click_pending = false;
    pb2_pb3_combo_long_press_sent = false;
    pb2_pb3_combo_duration = 0;
    pb2_was_pressed_in_combo = false;
    pb3_was_pressed_in_combo = false;
}

void Buttons_Update(uint16_t elapsed_ms)
{
    /* Read and update each button */
    UpdateButtonState(&pb1_state, PB1_Read(), elapsed_ms);
    UpdateButtonState(&pb2_state, PB2_Read(), elapsed_ms);
    UpdateButtonState(&pb3_state, PB3_Read(), elapsed_ms);
    
    /*------------------------------------------------------------------------
     * Track PB2+PB3 combo with improved detection
     * 
     * More forgiving combo detection:
     * - Track if each button was pressed during the combo window
     * - Combo triggers when both have been pressed and both are released
     * - Allows sequential pressing within the combo window
     *------------------------------------------------------------------------*/
    
    /* Track if buttons are pressed */
    if (pb2_state.current_state) {
        pb2_was_pressed_in_combo = true;
    }
    if (pb3_state.current_state) {
        pb3_was_pressed_in_combo = true;
    }
    
    /* If either button is pressed, accumulate combo duration */
    if (pb2_state.current_state || pb3_state.current_state) {
        pb2_pb3_combo_duration += elapsed_ms;
    }
    
    /* Check for combo when BOTH buttons are currently pressed */
    if (pb2_state.current_state && pb3_state.current_state) {
        /* Check for long press */
        if (pb2_pb3_combo_duration >= LONG_PRESS_THRESHOLD_MS &&
            !pb2_pb3_combo_long_press_sent) {
            pb2_pb3_combo_long_press_sent = true;
            /* Cancel individual clicks since this is a combo */
            pb2_state.click_pending = false;
            pb3_state.click_pending = false;
        }
    }
    
    /* Check when BOTH buttons are released */
    if (!pb2_state.current_state && !pb3_state.current_state) {
        /* If both buttons were pressed during this combo window, register combo click */
        if (pb2_was_pressed_in_combo && pb3_was_pressed_in_combo) {
            if (pb2_pb3_combo_duration > 0 && 
                pb2_pb3_combo_duration < LONG_PRESS_THRESHOLD_MS &&
                !pb2_pb3_combo_long_press_sent) {
                /* This was a short combo press - register as combo click */
                pb2_pb3_combo_click_pending = true;
                /* Cancel individual clicks */
                pb2_state.click_pending = false;
                pb3_state.click_pending = false;
            }
        }
        
        /* Reset combo tracking */
        pb2_pb3_combo_duration = 0;
        pb2_pb3_combo_long_press_sent = false;
        pb2_was_pressed_in_combo = false;
        pb3_was_pressed_in_combo = false;
    }
}

bool Buttons_IsPB1Clicked(void)
{
    if (pb1_state.click_pending) {
        pb1_state.click_pending = false;
        return true;
    }
    return false;
}

bool Buttons_IsPB2Clicked(void)
{
    if (pb2_state.click_pending) {
        pb2_state.click_pending = false;
        return true;
    }
    return false;
}

bool Buttons_IsPB3Clicked(void)
{
    if (pb3_state.click_pending) {
        pb3_state.click_pending = false;
        return true;
    }
    return false;
}

bool Buttons_ArePB2AndPB3Pressed(void)
{
    return pb2_state.current_state && pb3_state.current_state;
}

bool Buttons_IsPB3LongPress(void)
{
    return CheckLongPress(&pb3_state);
}

bool Buttons_IsPB2AndPB3LongPress(void)
{
    if (pb2_state.current_state && pb3_state.current_state &&
        pb2_pb3_combo_duration >= LONG_PRESS_THRESHOLD_MS &&
        !pb2_pb3_combo_long_press_sent) {
        pb2_pb3_combo_long_press_sent = true;
        return true;
    }
    return false;
}

bool Buttons_IsPB2AndPB3Click(void)
{
    if (pb2_pb3_combo_click_pending) {
        pb2_pb3_combo_click_pending = false;
        return true;
    }
    return false;
}

bool Buttons_IsPB1Pressed(void)
{
    return pb1_state.current_state;
}

bool Buttons_IsPB2Pressed(void)
{
    return pb2_state.current_state;
}

bool Buttons_IsPB3Pressed(void)
{
    return pb3_state.current_state;
}

void Buttons_ClearEvents(void)
{
    pb1_state.click_pending = false;
    pb2_state.click_pending = false;
    pb3_state.click_pending = false;
    pb2_pb3_combo_click_pending = false;
    
    pb1_state.long_press_sent = true;  /* Prevent pending long press */
    pb2_state.long_press_sent = true;
    pb3_state.long_press_sent = true;
    pb2_pb3_combo_long_press_sent = true;
}

bool Buttons_SendPendingEvents(void)
{
    ButtonEvent_t event;
    BaseType_t result;
    
    /* Check for combo events first (higher priority) */
    if (pb2_pb3_combo_click_pending) {
        pb2_pb3_combo_click_pending = false;
        event.button = BUTTON_PB2_AND_PB3;
        event.event = EVENT_CLICK;
        result = xQueueSend(xButtonQueue, &event, 0);
        return (result == pdPASS);
    }
    
    /* Check for PB2+PB3 long press */
    if (Buttons_IsPB2AndPB3LongPress()) {
        event.button = BUTTON_PB2_AND_PB3;
        event.event = EVENT_LONG_PRESS;
        result = xQueueSend(xButtonQueue, &event, 0);
        return (result == pdPASS);
    }
    
    /* Check individual button events */
    if (pb1_state.click_pending) {
        pb1_state.click_pending = false;
        event.button = BUTTON_PB1;
        event.event = EVENT_CLICK;
        result = xQueueSend(xButtonQueue, &event, 0);
        return (result == pdPASS);
    }
    
    if (CheckLongPress(&pb3_state)) {
        event.button = BUTTON_PB3;
        event.event = EVENT_LONG_PRESS;
        result = xQueueSend(xButtonQueue, &event, 0);
        return (result == pdPASS);
    }
    
    if (pb3_state.click_pending) {
        pb3_state.click_pending = false;
        event.button = BUTTON_PB3;
        event.event = EVENT_CLICK;
        result = xQueueSend(xButtonQueue, &event, 0);
        return (result == pdPASS);
    }
    
    if (pb2_state.click_pending) {
        pb2_state.click_pending = false;
        event.button = BUTTON_PB2;
        event.event = EVENT_CLICK;
        result = xQueueSend(xButtonQueue, &event, 0);
        return (result == pdPASS);
    }
    
    return false;
}

