/*
 * File:   adc.h
 * Author: ENCM 511
 * 
 * ADC Module Header
 * 
 * Description: Provides ADC initialization and potentiometer reading.
 *              Uses polling mode for simplicity (as allowed per spec).
 *              The potentiometer value controls LED2 brightness.
 * 
 * Created on Nov 2025
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * @brief Initialize the ADC module
 * 
 * Configures the ADC for 10-bit resolution, manual sampling,
 * and sets up the potentiometer channel as analog input.
 */
void ADC_Init(void);

/**
 * @brief Read the potentiometer ADC value
 * 
 * Performs a single ADC conversion on the potentiometer channel.
 * This function blocks until conversion is complete (~few microseconds).
 * 
 * @return uint16_t ADC value (0-1023 for 10-bit ADC)
 */
uint16_t ADC_ReadPotentiometer(void);

/**
 * @brief Convert ADC value to percentage
 * 
 * @param adc_value Raw ADC reading (0-1023)
 * @return uint8_t Percentage value (0-100)
 */
uint8_t ADC_ToPercent(uint16_t adc_value);

/**
 * @brief Read potentiometer and return as percentage
 * 
 * Convenience function that reads ADC and converts to percentage.
 * 
 * @return uint8_t Brightness percentage (0-100)
 */
uint8_t ADC_ReadBrightnessPercent(void);

#endif /* ADC_H */

