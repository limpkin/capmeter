/*
 * dac.h
 *
 * Created: 30/04/2015 20:22:55
 *  Author: limpkin
 */ 


#ifndef DAC_H_
#define DAC_H_

#include "defines.h"

// Debug printf
#ifdef DAC_PRINTF
    #define dacdprintf   printf
    #define dacdprintf_P printf_P
#else
    #define dacdprintf
    #define dacdprintf_P
#endif

// Defines
#define DACA0OFFCAL_offset  0x30
#define DACA0GAINCAL_offset 0x31
#define DACB0OFFCAL_offset  0x32
#define DACB0GAINCAL_offset 0x33
#define DACA1OFFCAL_offset  0x34
#define DACA1GAINCAL_offset 0x35
#define DACB1OFFCAL_offset  0x36
#define DACB1GAINCAL_offset 0x37

// Prototypes
void update_opampin_dac(uint16_t val);
void update_vbias_dac(uint16_t val);
void setup_vbias_dac(uint16_t val);
void disable_vbias_dac(void);
void setup_opampin_dac(uint16_t val);
void disable_opampin_dac(void);
void init_dac(void);

#endif /* DAC_H_ */