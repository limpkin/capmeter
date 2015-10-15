/*
 * main.h
 *
 * Created: 25/04/2015 19:50:31
 *  Author: limpkin
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef MAIN_PRINTF
    #define maindprintf   printf
    #define maindprintf_P printf_P
#else
    #define maindprintf
    #define maindprintf_P
#endif

#endif /* MAIN_H_ */