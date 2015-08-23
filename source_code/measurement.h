/*
 * measurement.h
 *
 * Created: 26/04/2015 11:37:58
 *  Author: limpkin
 */ 


#ifndef MEASUREMENT_H_
#define MEASUREMENT_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef MEAS_PRINTF
    #define measdprintf   printf
    #define measdprintf_P printf_P
#else
    #define measdprintf
    #define measdprintf_P
#endif

// defines state
#define NB_ERROR_FLAGS_CHG_RES      10          // Number of consecutive error flags to change resistors
#define MAX_FREQ_FOR_MEAS           100000UL    // Max oscillation frequency we allow
#define MIN_FREQ_FOR_MEAS           5000UL      // Min oscillation frequency we allow

// enums
enum mes_freq_t     {FREQ_1HZ = (32768-1), FREQ_2HZ = ((32768/2)-1), FREQ_4HZ = ((32768/4)-1), FREQ_8HZ = ((32768/8)-1), FREQ_16HZ = ((32768/16)-1), FREQ_32HZ = ((32768/32)-1), FREQ_64HZ = ((32768/64)-1), FREQ_128HZ = ((32768/128)-1)};
enum cur_mes_mode_t {CUR_MES_1X = 0, CUR_MES_2X = 1, CUR_MES_4X = 2, CUR_MES_8X = 3, CUR_MES_16X = 4, CUR_MES_32X = 5, CUR_MES_64X = 6};
enum mes_mode_t     {MES_OFF = 0, MES_CONT = 1};
    
// prototypes
uint16_t cur_measurement_loop(uint8_t avg_bitshift);
void set_current_measurement_mode(uint8_t ampl);
void disable_current_measurement_mode(void);
void set_capacitance_measurement_mode(void);
uint8_t cap_measurement_loop(uint8_t temp);

#endif /* MEASUREMENT_H_ */