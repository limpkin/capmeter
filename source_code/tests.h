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
    #define testdprintf   printf
    #define testdprintf_P printf_P
#else
    #define testdprintf
    #define testdprintf_P
#endif

void peak_to_peak_adc_noise_measurement_test(void);
void ramp_bias_voltage_test(void);
void voltage_settling_test(void);
void ramp_current_test(void);
void functional_test(void);

#endif /* TESTS_H_ */