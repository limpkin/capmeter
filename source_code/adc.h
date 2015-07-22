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
#define MAX_ADC_VAL         4095
#define MAX_DIFF_ADC_VAL    2047    
#define ADCACAL0_offset     0x20
#define ADCACAL1_offset     0x21
#define ADCBCAL0_offset     0x24
#define ADCBCAL1_offset     0x25

// prototypes
uint16_t get_averaged_stabilized_adc_value(uint8_t avg_bit_shift, uint16_t max_pp, uint8_t debug);
void configure_adc_channel(uint8_t channel, uint8_t ampl, uint8_t debug);
void set_adc_vbias_channel_clock_divider(uint8_t divider);
uint16_t get_averaged_adc_value(uint8_t avg_bit_shift);
int16_t start_and_wait_for_adc_conversion(void);
uint8_t get_configured_adc_channel(void);
uint8_t get_configured_adc_ampl(void);
void init_adc(void);

#endif /* ADC_H_ */