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
    #define calibprintf   printf
    #define calibprintf_P printf_P
#else
    #define calibprintf
    #define calibprintf_P
#endif

// Prototypes
void calibrate_single_ended_offset(void);
uint16_t get_single_ended_offset(void);
void init_calibration(void);

#endif /* CALIBRATION_H_ */