/*
 * adc.h
 *
 * Created: 14/05/2015 20:48:47
 *  Author: limpkin
 */ 


#ifndef ADC_H_
#define ADC_H_

#include "defines.h"

// Debug printf
#ifdef ADC_PRINTF
    #define adcprintf   printf
    #define adcprintf_P printf_P
#else
    #define adcprintf
    #define adcprintf_P
#endif

enum adc_channel_t     {ADC_CHANNEL_COMPOUT = 0};

uint16_t start_and_wait_for_adc_conversion(void);
void configure_adc_channel(uint8_t channel);
void init_adc(void);

#endif /* ADC_H_ */