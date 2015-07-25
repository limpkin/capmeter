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

// Typedefs
typedef void (*bootloader_f_ptr_type)(void);

// IO debug printf
//#define IO_PRINTF
// Serial debug printf
#define SERIAL_PRINTF
// Measurement debug printf
#define MEAS_PRINTF
// Dac debug printf
#define DAC_PRINTF
// Adc debug printf
#define ADC_PRINTF
// Calib debug printf
#define CALIB_PRINTF
// Vbias debug printf
#define VBIAS_PRINTF
// Tests debug printf
#define TESTS_PRINTF
// Conversions debug printf
#define CONV_PRINTF
// Automated testing printf
#define AUTOMATED_TESTS_PRINTF

// Check if we need to enable printf
#if defined(AUTOMATED_TESTS_PRINTF) || defined(CONV_PRINTF) || defined(TESTS_PRINTF) || defined(IO_PRINTF) || defined(VBIAS_PRINTF) || defined(CALIB_PRINTF) || defined(SERIAL_PRINTF) || defined(MEAS_PRINTF) || defined(DAC_PRINTF) || defined(ADC_PRINTF)
    #define PRINTF_ENABLED
#endif

#endif /* DEFINES_H_ */