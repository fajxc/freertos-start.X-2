/* 
 * File:   ADC.h
 * Author: Ali
 * 
 * Created on October 29, 2025, 12:42 PM
 */

#ifndef ADC_H
#define ADC_H

#include <xc.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t do_ADC(void);   // Function prototype
void init_ADC(void);

/* Additional function for percentage conversion */
uint8_t ADC_ToPercent(uint16_t adc_value);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
