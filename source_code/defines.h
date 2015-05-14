/*
 * defines.h
 *
 * Created: 25/04/2015 19:16:15
 *  Author: limpkin
 */ 


#ifndef DEFINES_H_
#define DEFINES_H_

// Val defines
#define FALSE   0
#define TRUE    (!FALSE)

// IO debug printf
#define IO_PRINTF
// Serial debug printf
#define SERIAL_PRINTF
// Measurement debug printf
#define MEAS_PRINTF
// Dac debug printf
#define DAC_PRINTF
// Adc debug printf
#define ADC_PRINTF

// Check if we need to enable printf
#if defined(IO_PRINTF) || defined(SERIAL_PRINTF) || defined(MEAS_PRINTF) || defined(DAC_PRINTF) || defined(ADC_PRINTF)
    #define PRINTF_ENABLED
#endif

#endif /* DEFINES_H_ */