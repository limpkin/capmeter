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
#define NB_CONSEQ_FREQ_PB_CHG_RES       1       // Number of consecutive freq problem before changing resistors
#define NB_CONSEQ_TC_ERR_FLAG_CHG_RES   3       // Number of consecutive tc error flags on smaller R before setting a higher R
#define MIN_OSC_FREQUENCY               800UL   // Minimum oscillation frequency we want

// typedefs
typedef struct capacitance_report_struct
{
    uint16_t counter_divider;                   // 32M time counter divider
    uint32_t aggregate_fall;                    // Fall aggregate
    uint32_t counter_value;                     // Counter value
    uint8_t report_freq;                        // Report frequency
    uint16_t half_res;                          // Resistor value / 2
    uint16_t second_thres;                      // Second comparison threshold
    uint16_t first_thres;                       // First comparison threshold
} capacitance_report_t;

// enums
enum mes_freq_t     {FREQ_1HZ = (32768-1), FREQ_2HZ = ((32768/2)-1), FREQ_4HZ = ((32768/4)-1), FREQ_8HZ = ((32768/8)-1), FREQ_16HZ = ((32768/16)-1), FREQ_32HZ = ((32768/32)-1), FREQ_64HZ = ((32768/64)-1), FREQ_128HZ = ((32768/128)-1)};
enum cur_mes_mode_t {CUR_MES_1X = 0, CUR_MES_2X = 1, CUR_MES_4X = 2, CUR_MES_8X = 3, CUR_MES_16X = 4, CUR_MES_32X = 5, CUR_MES_64X = 6};
enum mes_mode_t     {MES_OFF = 0, MES_CONT = 1};
    
// prototypes
uint8_t cap_measurement_loop(capacitance_report_t* cap_report);
void set_capacitance_report_frequency(uint8_t bit_shift);
void discard_next_cap_measurements(uint8_t nb_samples);
uint16_t cur_measurement_loop(uint8_t avg_bitshift);
void set_current_measurement_mode(uint8_t ampl);
void disable_capacitance_measurement_mode(void);
void adjust_digital_filter(uint8_t nb_samples);
void resume_capacitance_measurement_mode(void);
void pause_capacitance_measurement_mode(void);
void disable_current_measurement_mode(void);
void set_capacitance_measurement_mode(void);

#endif /* MEASUREMENT_H_ */