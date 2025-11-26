/*
 * File:   adc.c
 * Author: ENCM 511
 * 
 * ADC Module Implementation
 * 
 * Description: Implements ADC initialization and potentiometer reading
 *              for PIC24 microcontroller.
 * 
 * ADC Configuration:
 *   - 10-bit resolution (0-1023)
 *   - Manual sampling with auto-convert
 *   - Uses internal RC oscillator for timing
 *   - Single channel, single sample mode
 * 
 * Created on Nov 2025
 */

#include "adc.h"
#include "hw_config.h"
#include <xc.h>

/*============================================================================
 * PUBLIC FUNCTION IMPLEMENTATIONS
 *============================================================================*/

void ADC_Init(void)
{
    /*------------------------------------------------------------------------
     * Configure the analog pin
     * Set the potentiometer pin as analog input
     *------------------------------------------------------------------------*/
    ADC_Init_Pin();  /* From hw_config.h: sets TRIS=1 and ANSEL=1 */
    
    /*------------------------------------------------------------------------
     * AD1CON1: ADC Control Register 1
     * 
     * Bit 15 ADON:     0 = ADC off (will turn on after config)
     * Bit 13 ADSIDL:   0 = Continue operation in idle mode
     * Bit 10 ADDMABM:  0 = DMA buffer written in scatter/gather mode
     * Bit 9-8 AD12B:   0 = 10-bit, 4-channel ADC operation
     * Bit 7-5 FORM:    000 = Integer (0000 00dd dddd dddd)
     * Bit 4 SSRC:      111 = Auto-convert after sampling
     * Bit 2 ASAM:      0 = Sampling begins when SAMP bit is set
     * Bit 1 SAMP:      0 = ADC sample/hold is holding
     * Bit 0 DONE:      0 = ADC conversion not done
     *------------------------------------------------------------------------*/
    AD1CON1 = 0x00E0;   /* SSRC = 111 (auto-convert), 10-bit mode */
    
    /*------------------------------------------------------------------------
     * AD1CON2: ADC Control Register 2
     * 
     * Bit 15-13 VCFG:  000 = Vref+ = AVdd, Vref- = AVss
     * Bit 10 CSCNA:    0 = Do not scan inputs
     * Bit 7 BUFS:      Read only
     * Bit 2 SMPI:      0000 = Interrupt after every sample
     * Bit 1 BUFM:      0 = Always starts filling buffer from start address
     * Bit 0 ALTS:      0 = Always use channel input selects for Sample A
     *------------------------------------------------------------------------*/
    AD1CON2 = 0x0000;
    
    /*------------------------------------------------------------------------
     * AD1CON3: ADC Control Register 3
     * 
     * Bit 15 ADRC:     1 = ADC internal RC clock (no dependency on system clock)
     * Bit 12-8 SAMC:   00001 = 1 Tad auto-sample time
     * Bit 7-0 ADCS:    00000001 = Tad = 2*Tcy (minimum for RC clock)
     * 
     * Using internal RC oscillator for ADC timing for simplicity.
     *------------------------------------------------------------------------*/
    AD1CON3 = 0x8101;   /* ADRC = 1, SAMC = 1, ADCS = 1 */
    
    /*------------------------------------------------------------------------
     * AD1CHS: ADC Input Channel Select Register
     * 
     * Configure to sample the potentiometer channel.
     * CH0SA = ADC_POT_CHANNEL (from hw_config.h)
     * CH0NA = 0 (Vref- as negative reference)
     *------------------------------------------------------------------------*/
    AD1CHS = (ADC_POT_CHANNEL << 0);  /* Sample AN5 (or configured channel) */
    
    /*------------------------------------------------------------------------
     * AD1CSSL: ADC Input Scan Select Register
     * 
     * Not used (scanning disabled), set to 0.
     *------------------------------------------------------------------------*/
    AD1CSSL = 0x0000;
    
    /*------------------------------------------------------------------------
     * Enable the ADC module
     *------------------------------------------------------------------------*/
    AD1CON1bits.ADON = 1;
}

uint16_t ADC_ReadPotentiometer(void)
{
    /*------------------------------------------------------------------------
     * Perform single ADC conversion
     * 
     * Steps:
     * 1. Select the input channel (already set, but set again for safety)
     * 2. Start sampling
     * 3. Wait for conversion to complete
     * 4. Read result
     *------------------------------------------------------------------------*/
    
    /* Select potentiometer channel */
    AD1CHS = (ADC_POT_CHANNEL << 0);
    
    /* Clear DONE bit */
    AD1CON1bits.DONE = 0;
    
    /* Start sampling */
    AD1CON1bits.SAMP = 1;
    
    /* Wait for conversion to complete (auto-converts after sampling) */
    while (!AD1CON1bits.DONE) {
        /* Spin wait - this is very fast, typically < 2us */
    }
    
    /* Return the conversion result */
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

uint8_t ADC_ReadBrightnessPercent(void)
{
    uint16_t adc_value = ADC_ReadPotentiometer();
    return ADC_ToPercent(adc_value);
}

