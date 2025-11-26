/*
 * File:   hw_config.h
 * Author: ENCM 511
 * 
 * Hardware Configuration Header
 * 
 * Description: Defines all hardware pin mappings for LEDs, buttons, and ADC.
 *              Modify these definitions to match your specific hardware setup.
 * 
 * Created on Nov 2025
 */

#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include <xc.h>
#include <stdint.h>

/*============================================================================
 * LED CONFIGURATION
 * 
 * LED0 - Used in completion state (alternating blink)
 * LED1 - Blinks during countdown (1s on/1s off)
 * LED2 - Variable brightness LED controlled by potentiometer (software PWM)
 * 
 * Modify these macros to match your hardware pin connections.
 *============================================================================*/

/* LED0 - Completion indicator LED (RB5) */
#define LED0_TRIS       TRISBbits.TRISB5    /* Data direction register */
#define LED0_LAT        LATBbits.LATB5      /* Latch register for output */
#define LED0_PORT       PORTBbits.RB5       /* Port register for reading */
#define LED0_Init()     do { LED0_TRIS = 0; LED0_LAT = 0; } while(0)
#define LED0_On()       (LED0_LAT = 1)
#define LED0_Off()      (LED0_LAT = 0)
#define LED0_Toggle()   (LED0_LAT ^= 1)

/* LED1 - Countdown blink LED (RB6) */
#define LED1_TRIS       TRISBbits.TRISB6
#define LED1_LAT        LATBbits.LATB6
#define LED1_PORT       PORTBbits.RB6
#define LED1_Init()     do { LED1_TRIS = 0; LED1_LAT = 0; } while(0)
#define LED1_On()       (LED1_LAT = 1)
#define LED1_Off()      (LED1_LAT = 0)
#define LED1_Toggle()   (LED1_LAT ^= 1)

/* LED2 - PWM controlled brightness LED (RB7) */
#define LED2_TRIS       TRISBbits.TRISB7
#define LED2_LAT        LATBbits.LATB7
#define LED2_PORT       PORTBbits.RB7
#define LED2_Init()     do { LED2_TRIS = 0; LED2_LAT = 0; } while(0)
#define LED2_On()       (LED2_LAT = 1)
#define LED2_Off()      (LED2_LAT = 0)
#define LED2_Toggle()   (LED2_LAT ^= 1)

/* Alias for LED0 (backwards compatibility with original demo code) */
#define LED_DEMO_TRIS   LED0_TRIS
#define LED_DEMO_LAT    LED0_LAT
#define LED_DEMO_Toggle() LED0_Toggle()

/*============================================================================
 * BUTTON CONFIGURATION
 * 
 * PB1 - Used to initiate time entry mode (from waiting state)
 * PB2 - Used with PB3 to start/reset timer
 * PB3 - Used for pause/resume/reset operations
 * 
 * Buttons are assumed to be active-low (pressed = 0, released = 1)
 * with external or internal pull-up resistors.
 * 
 * Modify these macros to match your hardware pin connections.
 *============================================================================*/

/* PB1 - Start button (initiates time entry) - RB8 */
/* Note: RB8 is digital-only on PIC24FJ256GA702 - no ANSEL bit */
#define PB1_TRIS        TRISBbits.TRISB8
#define PB1_PORT        PORTBbits.RB8
#define PB1_PULLUP      CNPUBbits.CNPUB8    /* Internal pull-up enable */
#define PB1_Init()      do { PB1_TRIS = 1; PB1_PULLUP = 1; } while(0)
#define PB1_Read()      (!PB1_PORT)         /* Returns 1 when pressed (active-low) */

/* PB2 - Used with PB3 for start/reset - RB9 */
/* Note: RB9 is digital-only on PIC24FJ256GA702 - no ANSEL bit */
#define PB2_TRIS        TRISBbits.TRISB9
#define PB2_PORT        PORTBbits.RB9
#define PB2_PULLUP      CNPUBbits.CNPUB9    /* Internal pull-up enable */
#define PB2_Init()      do { PB2_TRIS = 1; PB2_PULLUP = 1; } while(0)
#define PB2_Read()      (!PB2_PORT)         /* Returns 1 when pressed (active-low) */

/* PB3 - Pause/Resume/Reset button - RA4 */
/* Note: RA4 is digital-only on PIC24FJ256GA702 - no ANSEL bit */
#define PB3_TRIS        TRISAbits.TRISA4
#define PB3_PORT        PORTAbits.RA4
#define PB3_PULLUP      CNPUAbits.CNPUA4    /* Internal pull-up enable */
#define PB3_Init()      do { PB3_TRIS = 1; PB3_PULLUP = 1; } while(0)
#define PB3_Read()      (!PB3_PORT)         /* Returns 1 when pressed (active-low) */

/*============================================================================
 * ADC CONFIGURATION
 * 
 * Potentiometer connected to ADC channel for LED2 brightness control.
 * PIC24 has 10-bit ADC (0-1023 range).
 * 
 * Modify ADC_POT_CHANNEL to match your potentiometer connection.
 *============================================================================*/

/* Potentiometer ADC channel - assumed on AN5 (RB3) */
#define ADC_POT_CHANNEL     5               /* AN5 */
#define ADC_POT_TRIS        TRISBbits.TRISB3
#define ADC_POT_ANSEL       ANSELBbits.ANSB3
#define ADC_MAX_VALUE       1023            /* 10-bit ADC maximum */
#define ADC_Init_Pin()      do { ADC_POT_TRIS = 1; ADC_POT_ANSEL = 1; } while(0)

/*============================================================================
 * TIMING CONFIGURATION
 * 
 * Timing constants for various system operations.
 *============================================================================*/

/* Debounce time for buttons (in milliseconds) */
#define BUTTON_DEBOUNCE_MS      50

/* Long press threshold (in milliseconds) */
#define BUTTON_LONG_PRESS_MS    1000

/* Software PWM frequency (Hz) - must be >60Hz to avoid flicker */
#define PWM_FREQUENCY_HZ        500

/* LED pulsing period for waiting state (full cycle in ms) */
#define PULSE_PERIOD_MS         2000

/* ADC sampling period (in milliseconds) */
#define ADC_SAMPLE_PERIOD_MS    50

/*============================================================================
 * SYSTEM INITIALIZATION
 * 
 * Macro to initialize all hardware.
 *============================================================================*/

#define HW_InitAllPins() do {   \
    LED0_Init();                \
    LED1_Init();                \
    LED2_Init();                \
    PB1_Init();                 \
    PB2_Init();                 \
    PB3_Init();                 \
    ADC_Init_Pin();             \
} while(0)

#endif /* HW_CONFIG_H */

