/*
 * vbias.h
 *
 * Created: 02/07/2015 16:50:24
 *  Author: limpkin
 */ 


#ifndef VBIAS_H_
#define VBIAS_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef VBIAS_PRINTF
    #define vbiasdprintf   printf
    #define vbiasdprintf_P printf_P
#else
    #define vbiasdprintf
    #define vbiasdprintf_P
#endif

// Defines
#define VBIAS_MIN_DAC_VAL       DAC_MAX_VAL - 66      // Bit less than 700mV
#define VBIAS_MAX_DAC_VAL       DAC_MIN_VAL           // Around 15300mV
#define VBIAS_MIN_V             700                   // Min V we can reach for Vbias
#define STEPUP_ACTIV_V          4300                  // Technically 4500mV should work
#define STEPUP_ACTIV_DAC_V      3070                  // Around 4350mV
#define BIT_AVG_APPROACH        3                     // Bit averaging for approach
#define BIT_AVG_FINE            13                    // Bit averaging for fine approach
#define CONV_DELAY_FINE         1                     // How long we should wait before launching a conversion (ms)
#define VBIAS_OVERSHOOT_MV      10                    // The overshoot we want (due to the capacitors)
#define MV_APPROCH              50+VBIAS_OVERSHOOT_MV // When to start fine approach

// Prototypes
uint16_t force_vbias_dac_change(uint16_t dac_value, uint16_t wait_ms);
uint16_t update_bias_voltage(uint16_t val_mv);
uint16_t enable_bias_voltage(uint16_t val_mv);
uint16_t get_current_vbias_dac_value(void);
uint16_t get_last_measured_vbias(void);
void disable_bias_voltage(void);
void wait_for_0v4_bias(void);
void wait_for_0v_bias(void);

#endif /* VBIAS_H_ */