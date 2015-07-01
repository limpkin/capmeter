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
#define DAC_MAX_VAL         0x0FFF              // 12 bits DAC
#define DAC_MIN_VAL         0x0000              // Obviously...
#define VBIAS_MIN_DAC_VAL   DAC_MAX_VAL - 200   // Approx 1200mV
#define VBIAS_MAX_DAC_VAL   DAC_MIN_VAL         // Around 15440mV
#define STEPUP_ACTIV_V      4300                // Technically 4500mV should work
#define VBIAS_MIN_V         1250                // TPS datasheet mentions 1212mV

// enums
enum mes_freq_t     {FREQ_1HZ = (32768-1), FREQ_32HZ = ((32768/32)-1), FREQ_64HZ = ((32768/64)-1), FREQ_128HZ = ((32768/128)-1)};
enum cur_mes_mode_t {CUR_MES_1X = 0, CUR_MES_2X = 1, CUR_MES_4X = 2, CUR_MES_8X = 3, CUR_MES_16X = 4, CUR_MES_32X = 5, CUR_MES_64X = 6};
enum mes_mode_t     {MES_OFF = 0, MES_CONT = 1};
    
// prototypes
uint16_t get_averaged_stabilized_adc_value(uint8_t avg_bit_shift, uint16_t max_pp, uint8_t debug);
void quiescent_cur_measurement_loop(uint8_t ampl);
void set_current_measurement_ampl(uint8_t ampl);
void set_measurement_frequency(uint16_t freq);
uint16_t enable_bias_voltage(uint16_t val_mv);
void measure_opamp_internal_resistance(void);
void disable_current_measurement_mode(void);
void measurement_loop(uint8_t ampl);
void disable_bias_voltage(void);
void calibrate_vup_vlow(void);
void bias_voltage_test(void);
void wait_for_1v_bias(void);
void init_measurement(void);

#endif /* MEASUREMENT_H_ */