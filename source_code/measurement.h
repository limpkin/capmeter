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

// enums
enum mes_freq_t     {FREQ_1HZ = (32768-1), FREQ_32HZ = ((32768/32)-1), FREQ_64HZ = ((32768/64)-1), FREQ_128HZ = ((32768/128)-1)};
enum cur_mes_mode_t {CUR_MES_1X = 0, CUR_MES_2X = 1, CUR_MES_4X = 2, CUR_MES_8X = 3, CUR_MES_16X = 4, CUR_MES_32X = 5, CUR_MES_64X = 6};
enum mes_mode_t     {MES_OFF = 0, MES_CONT = 1};
    
// prototypes
uint16_t compute_cur_mes_numerator_from_adc_val(uint16_t adc_val);
uint16_t quiescent_cur_measurement_loop(uint8_t avg_bitshift);
uint16_t compute_voltage_from_se_adc_val(uint16_t adc_val);
void set_current_measurement_ampl(uint8_t ampl);
void print_compute_r_formula(uint16_t adc_val);
void set_measurement_frequency(uint16_t freq);
void disable_current_measurement_mode(void);
void measurement_loop(uint8_t ampl);

#endif /* MEASUREMENT_H_ */