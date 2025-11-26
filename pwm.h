/*
 * File:   pwm.h
 * Author: ENCM 511
 * 
 * Software PWM Module Header
 * 
 * Description: Provides software-based PWM generation using Timer2.
 *              Used to control LED2 brightness based on potentiometer input.
 * 
 * IMPORTANT: This module uses Timer2 ISR for PWM generation.
 *            Do NOT use hardware MCCP modules as per project requirements.
 * 
 * PWM Frequency: Configurable, default 500Hz (>60Hz to avoid flicker)
 * Resolution: 100 steps (0-100% duty cycle)
 * 
 * Created on Nov 2025
 */

#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Initialize the software PWM module
 * 
 * Configures Timer2 for PWM frequency and enables the interrupt.
 * PWM output starts at 0% duty cycle (LED off).
 */
void PWM_Init(void);

/**
 * @brief Start PWM output
 * 
 * Enables the Timer2 interrupt to generate PWM signal.
 */
void PWM_Start(void);

/**
 * @brief Stop PWM output
 * 
 * Disables the Timer2 interrupt and turns off the LED.
 */
void PWM_Stop(void);

/**
 * @brief Set PWM duty cycle
 * 
 * @param duty_percent Duty cycle percentage (0-100)
 *        0 = LED fully off
 *        100 = LED fully on
 */
void PWM_SetDutyCycle(uint8_t duty_percent);

/**
 * @brief Get current PWM duty cycle
 * 
 * @return uint8_t Current duty cycle percentage (0-100)
 */
uint8_t PWM_GetDutyCycle(void);

/**
 * @brief Set LED2 output state directly (for blinking)
 * 
 * When blinking mode is active, this can be used to
 * force LED2 on or off regardless of PWM.
 * 
 * @param enabled true = use PWM output, false = force LED off
 */
void PWM_SetOutputEnabled(bool enabled);

/**
 * @brief Check if PWM output is currently enabled
 * 
 * @return bool true if PWM output is active
 */
bool PWM_IsOutputEnabled(void);

/**
 * @brief Update brightness for pulsing effect (waiting state)
 * 
 * Call this periodically to create a smooth breathing/pulsing effect.
 * Uses a sine-wave approximation for smooth transitions.
 * 
 * @param elapsed_ms Time since last update in milliseconds
 * @param period_ms Full pulsing period in milliseconds
 */
void PWM_UpdatePulse(uint16_t elapsed_ms, uint16_t period_ms);

/**
 * @brief Reset the pulse phase to beginning (bright)
 */
void PWM_ResetPulse(void);

#endif /* PWM_H */

