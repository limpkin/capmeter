/*
 * measurement.h
 *
 * Created: 26/04/2015 11:37:58
 *  Author: limpkin
 */ 


#ifndef MEASUREMENT_H_
#define MEASUREMENT_H_

#include "defines.h"

// Debug printf
#ifdef MEAS_PRINTF
    #define measdprintf   printf
    #define measdprintf_P printf_P
#else
    #define measdprintf
    #define measdprintf_P
#endif

// defines
#define DAC_MAX_VAL         0x0FFF
#define DAC_MIN_VAL         0x0000
#define VBIAS_MIN_DAC_VAL   DAC_MAX_VAL
#define VBIAS_MAX_DAC_VAL   DAC_MIN_VAL

// enums
enum mes_freq_t     {FREQ_1HZ = (32768-1), FREQ_32HZ = ((32768/32)-1), FREQ_64HZ = ((32768/64)-1), FREQ_128HZ = ((32768/128)-1)};
enum mes_mode_t     {MES_OFF = 0, MES_CONT = 1};
    
// prototypes
void measurement_loop(uint8_t mes_mode);
void measure_opamp_internal_resistance(void);
void set_measurement_frequency(uint16_t freq);
void enable_bias_voltage(uint16_t val);
void disable_bias_voltage(void);
void calibrate_vup_vlow(void);

#endif /* MEASUREMENT_H_ */