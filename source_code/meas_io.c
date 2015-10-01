/*
 * io.c
 *
 * Created: 25/04/2015 19:12:43
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>
#include "meas_io.h"
// current res mux
uint8_t cur_res_mux;
// ldo enabled bool
uint8_t ldo_enabled_bool = FALSE;
// stepup enabled bool
uint8_t stepup_enabled_bool = FALSE;


/*
 * Get current res mux
 * @return  The res mux
 */
uint8_t get_cur_res_mux(void)
{
    return cur_res_mux;
}

/*
 * Enable vbias quenching
 */
void enable_vbias_quenching(void)
{
    PORTC_OUTSET = PIN1_bm;
    iodprintf_P(PSTR("Quenching Vbias\r\n"));
}

/*
 * Disable vbias quenching
 */
void disable_vbias_quenching(void)
{
    PORTC_OUTCLR = PIN1_bm;
    iodprintf_P(PSTR("Disabled Vbias quench\r\n"));
}

/*
 * Enable the current measurement mosfet
 */
void enable_cur_meas_mos(void)
{
    PORTA_OUTSET = PIN0_bm;
    iodprintf_P(PSTR("Quiescent Cur Measurement Mos On\r\n"));   
}

/*
 * Disable the current measurement mosfet
 */
void disable_cur_meas_mos(void)
{
    PORTA_OUTCLR = PIN0_bm;
    iodprintf_P(PSTR("Quiescent Cur Measurement Mos Off\r\n"));
}

/*
 * Set opampin- low
 */
void set_opampin_low(void)
{    
    PORTB.DIRSET = PIN2_bm;
    PORTB.OUTCLR = PIN2_bm;
    iodprintf_P(PSTR("Opampin Low\r\n"));
}

/*
 * Set opampin- high
 */
void set_opampin_high(void)
{    
    PORTB.DIRSET = PIN2_bm;
    PORTB.OUTSET = PIN2_bm;
    //iodprintf_P(PSTR("Opampin High\r\n"));
}

/*
 * Set opampin- as input
 */
void opampin_as_input(void)
{    
    PORTB.DIRCLR = PIN2_bm;
    iodprintf_P(PSTR("Opampin as Input\r\n"));
}

/*
 * Enable the oscillator feedback mosfet
 */
void enable_feedback_mos(void)
{
    PORTB_OUTSET = PIN1_bm;
    iodprintf_P(PSTR("Feedback Mos On\r\n"));
}

/*
 * Disable the oscillator feedback mosfet
 */
void disable_feedback_mos(void)
{
    PORTB_OUTCLR = PIN1_bm;
    iodprintf_P(PSTR("Feedback Mos Off\r\n"));
}

/*
 * Enable the stepup
 */
void enable_stepup(void)
{
    PORTC_OUTSET = PIN5_bm;
    stepup_enabled_bool = TRUE;
    iodprintf_P(PSTR("Stepup On\r\n"));
}

/*
 * Disable the stepup
 */
void disable_stepup(void)
{
    PORTC_OUTCLR = PIN5_bm;
    stepup_enabled_bool = FALSE;
    iodprintf_P(PSTR("Stepup Off\r\n"));
}

/*
 * Know if stepup is enabled or not
 * @return  The bool
 */
uint8_t is_stepup_enabled(void)
{
    return stepup_enabled_bool;
}

/*
 * Enable the ldo
 */
void enable_ldo(void)
{
    PORTC_OUTSET = PIN4_bm;
    ldo_enabled_bool = TRUE;
    iodprintf_P(PSTR("LDO On\r\n"));
}

/*
 * Disable the LDO
 */
void disable_ldo(void)
{
    PORTC_OUTCLR = PIN4_bm;
    ldo_enabled_bool = FALSE;
    iodprintf_P(PSTR("LDO Off\r\n"));
}

/*
 * Know if LDO is enabled or not
 * @return  The bool
 */
 uint8_t is_ldo_enabled(void)
 {
     return ldo_enabled_bool;
 }

/*
 * Disable the resistor mux
 */
void disable_res_mux(void)
{
    PORTA_OUTCLR = PIN4_bm;
    PORTA_OUTCLR = PIN3_bm;
    PORTA_OUTCLR = PIN2_bm;
    iodprintf_P(PSTR("Resistor Mux Off\r\n"));
}


/*
 * Print current res_mux
 */
void print_res_mux_val(void)
{ 
    // Debug print
    switch(cur_res_mux)
    {
        case RES_470:
        {
            iodprintf_P(PSTR("Resistor Mux: 470R\r\n"));
            break;
        }
        case RES_1K:
        {
            iodprintf_P(PSTR("Resistor Mux: 1K\r\n"));
            break;
        }
        case RES_10K:
        {
            iodprintf_P(PSTR("Resistor Mux: 10K\r\n"));
            break;
        }
        case RES_100K:
        {
            iodprintf_P(PSTR("Resistor Mux: 100K\r\n"));
            break;
        }
        default: break;
    }
}

/*
 * Enable resistor mux
 * @param   val     the resistor to be selected (see res_mux_t)
 * @param   debug   bool to indicate if we should printf
 */
void enable_res_mux(uint8_t val, uint8_t debug)
{
    cur_res_mux = val;
    PORTA_OUTSET = PIN4_bm;
    if (val & 0x01)
    {
        PORTA_OUTSET = PIN3_bm;
    } 
    else
    {
        PORTA_OUTCLR = PIN3_bm;
    }
    if (val & 0x02)
    {
        PORTA_OUTSET = PIN2_bm;
    } 
    else
    {
        PORTA_OUTCLR = PIN2_bm;
    }
    if (debug == TRUE)
    {
        print_res_mux_val();
    }    
}

/*
 * Enable everything to allow freq measurement
 * @param   res     the resistor to be selected (see res_mux_t)
 */
void set_measurement_mode_io(uint8_t res)
{
    opampin_as_input();
    enable_feedback_mos();
    disable_cur_meas_mos();
    enable_res_mux(res, TRUE);
}

/*
 * Disable the ios for freq measurement
 */
void disable_measurement_mode_io(void)
{
    disable_res_mux();
    set_opampin_low();
    disable_feedback_mos();
}

/*
 * Init project IOs
 */
void init_ios(void)
{
    iodprintf_P(PSTR("-----------------------\r\n"));
    iodprintf_P(PSTR("IO init\r\n\r\n"));
    enable_vbias_quenching();                               // Enable vbias quenching
    disable_cur_meas_mos();                                 // Disable cur measurement mosfet
    disable_feedback_mos();                                 // Disable opamp feedback
    disable_res_mux();                                      // Disable resistor mux
    set_opampin_low();                                      // Prevent oscillations
    disable_stepup();                                       // Disable stepup
    disable_ldo();                                          // Disable LDO
    PORTA_DIRSET = PIN0_bm | PIN2_bm | PIN3_bm | PIN4_bm;   // CON_SH, MUX_A1, MUX_A0, MUX_EN
    PORTB_DIRSET = PIN1_bm;                                 // AMP_FB_EN
    PORTC_DIRSET = PIN1_bm | PIN4_bm | PIN5_bm;             // QUENCH, LDO_EN, SU_EN
}