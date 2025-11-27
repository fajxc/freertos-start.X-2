#include <xc.h>
#include <stdint.h>
#include "adc.h"
#include "hw_config.h"

#define FCY 16000000UL
#include <libpic30.h>

void init_ADC(void) {
    ANSAbits.ANSA3 = 1;     // RA3 = analog
    TRISAbits.TRISA3 = 1;   // RA3 input
    
    AD1CON1bits.ADON = 0;   // Disable ADC
    AD1CON1bits.FORM = 0;   // Integer
    AD1CON1bits.SSRC = 0b111; // Internal counter ends sampling
    AD1CON1bits.ASAM = 0;   // Manual sampling
    AD1CON1bits.MODE12 = 0; // 10-bit mode for lab spec
    
    AD1CON2 = 0;            // Use MUXA, AVdd/AVss
    
    AD1CON3bits.ADCS = 10;  // TAD
    AD1CON3bits.SAMC = 15;  // Sample time
    
    AD1CHSbits.CH0SA = 5;   // AN5 input (note: your code says AN3 but CH0SA=5 is AN5)
    AD1CHSbits.CH0NA = 0;   // VSS-
    
    AD1CON1bits.ADON = 1;   // Turn on ADC
    __delay_ms(2);
}

uint16_t do_ADC(void) {
    AD1CON1bits.SAMP = 1;   // Start sampling
    __delay_us(20);
    AD1CON1bits.SAMP = 0;   // Start conversion
    while(!AD1CON1bits.DONE); // Wait
    return ADC1BUF0;
}

uint8_t ADC_ToPercent(uint16_t adc_value)
{
    /*------------------------------------------------------------------------
     * Convert 10-bit ADC value (0-1023) to percentage (0-100)
     * 
     * Formula: percent = (adc_value * 100) / 1023
     * 
     * Using 32-bit intermediate to avoid overflow.
     *------------------------------------------------------------------------*/
    uint32_t temp = (uint32_t)adc_value * 100;
    return (uint8_t)(temp / ADC_MAX_VALUE);
}
