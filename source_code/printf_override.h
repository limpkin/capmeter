/*
 * printf_override.h
 *
 * Created: 23/07/2015 20:11:53
 *  Author: limpkin
 */ 


#ifndef PRINTF_OVERRIDE_H_
#define PRINTF_OVERRIDE_H_

#define OVERRIDE
#ifdef OVERRIDE
    #undef IO_PRINTF
    #undef SERIAL_PRINTF
    #undef MEAS_PRINTF
    #undef DAC_PRINTF
    #undef ADC_PRINTF
    #undef CALIB_PRINTF
    #undef VBIAS_PRINTF
#endif

#endif /* PRINTF_OVERRIDE_H_ */