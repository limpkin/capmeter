/*
 * io.h
 *
 * Created: 25/04/2015 19:19:16
 *  Author: limpkin
 */ 
#include "defines.h"


#ifndef IO_H_
#define IO_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef IO_PRINTF
    #define iodprintf   printf
    #define iodprintf_P printf_P
#else
    #define iodprintf
    #define iodprintf_P
#endif

// enums
enum res_mux_t     {RES_270 = 0, RES_100K = 1, RES_1K = 2, RES_10K = 3};
    
// prototypes
void init_ios(void);
uint8_t get_cur_res_mux(void);
void enable_cur_meas_mos(void);
void disable_cur_meas_mos(void);
void enable_feedback_mos(void);
void disable_feedback_mos(void);
void enable_vbias_quenching(void);
void disable_vbias_quenching(void);
void set_opampin_high(void);
void set_opampin_low(void);
void opampin_as_input(void);
void enable_stepup(void);
void disable_stepup(void);
void enable_ldo(void);
void disable_ldo(void);
void disable_res_mux(void);
void print_res_mux_val(void);
uint8_t is_ldo_enabled(void);
void disable_measurement_mode_io(void);
void set_measurement_mode_io(uint8_t res);
void enable_res_mux(uint8_t val, uint8_t debug);

#endif /* IO_H_ */