/*
 * calibration.h
 *
 * Created: 02/07/2015 15:55:23
 *  Author: limpkin
 */ 


#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include "defines.h"

// Debug printf
#ifdef CALIB_PRINTF
    #define calibprintf   printf
    #define calibprintf_P printf_P
#else
    #define calibprintf
    #define calibprintf_P
#endif

// Prototypes
int16_t get_differential_offset_for_ampl(uint8_t ampl);
void calibrate_opamp_internal_resistance(void);
void calibrate_current_measurement(void);
void calibrate_single_ended_offset(void);
uint16_t get_single_ended_offset(void);
void calibrate_cur_mos_0nA(void);
void calibrate_vup_vlow(void);
uint16_t get_calib_vlow(void);
uint16_t get_calib_vup(void);
void init_calibration(void);

#endif /* CALIBRATION_H_ */