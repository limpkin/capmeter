/*
 * calibration.h
 *
 * Created: 02/07/2015 15:55:23
 *  Author: limpkin
 */ 


#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef CALIB_PRINTF
    #define calibdprintf   printf
    #define calibdprintf_P printf_P
#else
    #define calibdprintf
    #define calibdprintf_P
#endif

// Defines
#define THRESHOLD_AVG_BIT_SHIFT     6

// Prototypes
uint16_t get_offset_for_current_measurement(uint8_t ampl);
uint16_t get_calib_second_thres_down(void);
uint16_t get_calib_first_thres_down(void);
uint16_t get_calib_second_thres_up(void);
uint16_t get_calib_first_thres_up(void);
void delete_cur_measurement_offsets(void);
void calibrate_single_ended_offset(void);
uint16_t get_single_ended_offset(void);
void delete_single_ended_offset(void);
uint16_t get_max_vbias_voltage(void);
uint16_t get_calib_osc_low_v(void);
void init_calibration(void);

#endif /* CALIBRATION_H_ */