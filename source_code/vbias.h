/*
 * vbias.h
 *
 * Created: 02/07/2015 16:50:24
 *  Author: limpkin
 */ 


#ifndef VBIAS_H_
#define VBIAS_H_

#include "defines.h"

// Debug printf
#ifdef VBIAS_PRINTF
    #define vbiasprintf   printf
    #define vbiasprintf_P printf_P
#else
    #define vbiasprintf
    #define vbiasprintf_P
#endif

// Defines
#define VBIAS_MIN_DAC_VAL   DAC_MAX_VAL - 66    // Bit less than 700mV
#define VBIAS_MAX_DAC_VAL   DAC_MIN_VAL         // Around 15440mV
#define VBIAS_MIN_V         700                 // Min V we can reach for Vbias
#define STEPUP_ACTIV_V      4300                // Technically 4500mV should work
#define STEPUP_ACTIV_DAC_V  3070                // Around 4350mV

// Prototypes
uint16_t update_bias_voltage(uint16_t val_mv);
uint16_t enable_bias_voltage(uint16_t val_mv);
void disable_bias_voltage(void);
void bias_voltage_test(void);
void wait_for_0v_bias(void);


#endif /* VBIAS_H_ */