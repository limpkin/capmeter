/*
 * automated_testing.h
 *
 * Created: 23/07/2015 19:48:00
 *  Author: limpkin
 */ 


#ifndef AUTOMATED_TESTING_H_
#define AUTOMATED_TESTING_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef AUTOMATED_TESTS_PRINTF
    #define auttestdprintf   printf
    #define auttestdprintf_P printf_P
#else
    #define auttestdprintf
    #define auttestdprintf_P
#endif

void automated_current_testing(void);
void automated_vbias_testing(void);

#endif /* AUTOMATED_TESTING_H_ */