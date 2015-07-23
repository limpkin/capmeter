/*
 * serial.h
 *
 * Created: 25/04/2015 19:13:12
 *  Author: limpkin
 */ 
#include "defines.h"


#ifndef SERIAL_H_
#define SERIAL_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef SERIAL_PRINTF
    #define serialdprintf   printf
    #define serialdprintf_P printf_P
#else
    #define serialdprintf
    #define serialdprintf_P
#endif

// Prototypes
void init_serial_port(void);

#endif /* SERIAL_H_ */