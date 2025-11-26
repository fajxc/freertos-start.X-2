/*
 * File:   pwm.c
 * Author: ENCM 511
 * 
 * Software PWM Module Implementation
 * 
 * Description: Implements software PWM using Timer2 interrupt.
 *              The ISR runs at (PWM_FREQUENCY * 100) Hz to provide
 *              100-step resolution at the target PWM frequency.
 * 
 * Example: For 500Hz PWM with 100 steps:
 *          Timer2 ISR runs at 500 * 100 = 50,000 Hz (every 20us)
 * 
 * PWM Algorithm:
 *   - Counter increments from 0 to 99 each PWM period
 *   - LED is ON when counter < duty_cycle
 *   - LED is OFF when counter >= duty_cycle
 *   - This creates duty_cycle% ON time
 * 
 * IMPORTANT: Timer2 is dedicated to PWM. Timer1 is used by FreeRTOS.
 * 
 * Created on Nov 2025
 */

#include "pwm.h"
#include "hw_config.h"
#include "FreeRTOSConfig.h"  /* For configCPU_CLOCK_HZ */
#include <xc.h>

/*============================================================================
 * CONFIGURATION CONSTANTS
 *============================================================================*/

/* PWM resolution - number of steps per period */
#define PWM_RESOLUTION      100UL

/* Target PWM frequency in Hz */
#define PWM_TARGET_FREQ     PWM_FREQUENCY_HZ    /* From hw_config.h */

/* Timer2 interrupt frequency = PWM_FREQ * PWM_RESOLUTION */
#define TIMER2_FREQ         ((unsigned long)PWM_TARGET_FREQ * PWM_RESOLUTION)

/* System clock frequency (Fosc/2 for instruction cycle) */
#define FCY                 ((unsigned long)configCPU_CLOCK_HZ)    /* 4MHz from FreeRTOSConfig.h */

/* Timer2 prescaler options: 1:1, 1:8, 1:64, 1:256 */
/* For 50kHz timer with 4MHz Fcy: Period = 4MHz / 50kHz = 80 cycles */
/* With 1:1 prescaler, PR2 = 80 - 1 = 79 */
#define TIMER2_PRESCALE     1UL
#define TIMER2_PR_VALUE     ((uint16_t)((FCY / TIMER2_FREQ / TIMER2_PRESCALE) - 1))

/*============================================================================
 * STATIC VARIABLES
 *============================================================================*/

/* Current duty cycle (0-100) */
static volatile uint8_t pwm_duty_cycle = 0;

/* PWM counter (0-99) */
static volatile uint8_t pwm_counter = 0;

/* Output enable flag */
static volatile bool pwm_output_enabled = true;

/* Pulse effect phase (0-65535 maps to 0-360 degrees) */
static volatile uint16_t pulse_phase = 0;

/*============================================================================
 * SINE TABLE FOR SMOOTH PULSING
 * 
 * 16 entries representing a half sine wave (0 to 180 degrees)
 * Values are 0-100 representing brightness percentage.
 * This creates a smooth "breathing" effect.
 *============================================================================*/

static const uint8_t sine_table[16] = {
    0,   5,  20,  39,  59,  78,  91,  98,    /* Rising: 0 to peak */
    100, 98,  91,  78,  59,  39,  20,   5     /* Falling: peak to 0 */
};

/*============================================================================
 * TIMER2 INTERRUPT SERVICE ROUTINE
 * 
 * This ISR runs at high frequency to generate the PWM waveform.
 * Keep it as short as possible!
 *============================================================================*/

void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void)
{
    /* Clear interrupt flag immediately */
    IFS0bits.T2IF = 0;
    
    /* Increment PWM counter */
    pwm_counter++;
    if (pwm_counter >= PWM_RESOLUTION) {
        pwm_counter = 0;
    }
    
    /* Update LED output based on duty cycle and enable state */
    if (pwm_output_enabled && (pwm_counter < pwm_duty_cycle)) {
        LED2_On();
    } else {
        LED2_Off();
    }
}

/*============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

void PWM_Init(void)
{
    /*------------------------------------------------------------------------
     * Initialize LED2 pin
     *------------------------------------------------------------------------*/
    LED2_Init();
    LED2_Off();
    
    /*------------------------------------------------------------------------
     * Configure Timer2 for PWM generation
     * 
     * Timer2 period = (PR2 + 1) * prescale * Tcy
     * We want timer interrupt at TIMER2_FREQ Hz
     * 
     * With 4MHz Fcy and 500Hz PWM with 100 steps:
     * TIMER2_FREQ = 50,000 Hz
     * Period = 4,000,000 / 50,000 = 80 cycles
     * PR2 = 80 - 1 = 79
     *------------------------------------------------------------------------*/
    
    /* Stop timer during configuration */
    T2CONbits.TON = 0;
    
    /* Clear timer register */
    TMR2 = 0;
    
    /* Set period register */
    PR2 = TIMER2_PR_VALUE;
    
    /* Configure timer:
     * - TCKPS = 00: 1:1 prescale
     * - TCS = 0: Internal clock (Fosc/2)
     */
    T2CON = 0x0000;
    
    /* Configure interrupt */
    IPC1bits.T2IP = 4;      /* Priority 4 (above FreeRTOS kernel) */
    IFS0bits.T2IF = 0;      /* Clear interrupt flag */
    IEC0bits.T2IE = 1;      /* Enable interrupt */
    
    /* Initialize PWM state */
    pwm_duty_cycle = 0;
    pwm_counter = 0;
    pwm_output_enabled = true;
}

void PWM_Start(void)
{
    /* Reset counter */
    pwm_counter = 0;
    TMR2 = 0;
    
    /* Clear any pending interrupt */
    IFS0bits.T2IF = 0;
    
    /* Enable interrupt */
    IEC0bits.T2IE = 1;
    
    /* Start timer */
    T2CONbits.TON = 1;
}

void PWM_Stop(void)
{
    /* Stop timer */
    T2CONbits.TON = 0;
    
    /* Disable interrupt */
    IEC0bits.T2IE = 0;
    
    /* Turn off LED */
    LED2_Off();
}

void PWM_SetDutyCycle(uint8_t duty_percent)
{
    /* Clamp to valid range */
    if (duty_percent > 100) {
        duty_percent = 100;
    }
    
    pwm_duty_cycle = duty_percent;
}

uint8_t PWM_GetDutyCycle(void)
{
    return pwm_duty_cycle;
}

void PWM_SetOutputEnabled(bool enabled)
{
    pwm_output_enabled = enabled;
    
    /* If disabling, turn off LED immediately */
    if (!enabled) {
        LED2_Off();
    }
}

bool PWM_IsOutputEnabled(void)
{
    return pwm_output_enabled;
}

void PWM_UpdatePulse(uint16_t elapsed_ms, uint16_t period_ms)
{
    /*------------------------------------------------------------------------
     * Update pulse phase based on elapsed time
     * 
     * phase ranges from 0 to 65535 (maps to 0-360 degrees or one full cycle)
     * We use 16 entries in sine table, so divide phase into 16 segments.
     *------------------------------------------------------------------------*/
    
    uint32_t phase_increment;
    uint8_t table_index;
    
    /* Calculate phase increment for this time step */
    /* phase_increment = (elapsed_ms / period_ms) * 65536 */
    phase_increment = ((uint32_t)elapsed_ms * 65536UL) / period_ms;
    
    /* Update phase (will wrap around naturally) */
    pulse_phase += (uint16_t)phase_increment;
    
    /* Map phase to table index (0-15) */
    /* Top 4 bits of 16-bit phase give us 0-15 */
    table_index = pulse_phase >> 12;
    
    /* Set duty cycle from table */
    pwm_duty_cycle = sine_table[table_index];
}

void PWM_ResetPulse(void)
{
    pulse_phase = 0;
    pwm_duty_cycle = sine_table[0];
}

