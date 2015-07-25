/*
 * conversions.h
 *
 * Created: 25/07/2015 22:09:53
 *  Author: limpkin
 */ 


#ifndef CONVERSIONS_H_
#define CONVERSIONS_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef CONV_PRINTF
    #define convprintf   printf
    #define convprintf_P printf_P
#else
    #define convprintf
    #define convprintf_P
#endif

// Prototypes
void print_compute_c_formula(uint32_t aggregate, uint32_t counter, uint16_t counter_divider, uint8_t res_mux);
uint16_t compute_cur_mes_numerator_from_adc_val(uint16_t adc_val);
uint16_t compute_voltage_from_se_adc_val(uint16_t adc_val);
uint16_t get_half_val_for_res_mux_define(uint16_t define);
uint16_t compute_vbias_for_adc_value(uint16_t adc_val);
uint16_t get_val_for_counter_divider(uint8_t divider);
uint16_t get_val_for_freq_define(uint16_t define);
void print_compute_cur_formula(uint16_t adc_val);

#endif /* CONVERSIONS_H_ */