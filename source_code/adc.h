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

// enums
enum adc_channel_t     {ADC_CHANNEL_COMPOUT = 0, ADC_CHANNEL_VBIAS = 1, ADC_CHANNEL_GND_EXT = 2, ADC_CHANNEL_CUR = 3};
    
// defines
#define MAX_ADC_VAL     4096

void configure_adc_channel(uint8_t channel, uint8_t ampl);
uint16_t get_averaged_adc_value(uint8_t avg_bit_shift);
int16_t start_and_wait_for_adc_conversion(void);
void calibrate_cur_mos_0nA(void);
void init_adc(void);

#endif /* ADC_H_ */