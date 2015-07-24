/*
 * tests.h
 *
 * Created: 24/07/2015 18:11:40
 *  Author: limpkin
 */ 


#ifndef TESTS_H_
#define TESTS_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef TESTS_PRINTF
    #define testprintf   printf
    #define testprintf_P printf_P
#else
    #define testprintf
    #define testprintf_P
#endif

void peak_to_peak_adc_noise_measurement_test(void);
void ramp_bias_voltage_test(void);
void bias_voltage_test2(void);
void bias_voltage_test(void);

#endif /* TESTS_H_ */