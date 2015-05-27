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


/*
 * Get current res mux
 * @return  The res mux
 */
uint8_t get_cur_res_mux(void)
{
    return cur_res_mux;
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
    iodprintf_P(PSTR("Stepup On\r\n"));
}

/*
 * Disable the stepup
 */
void disable_stepup(void)
{
    PORTC_OUTCLR = PIN5_bm;
    iodprintf_P(PSTR("Stepup Off\r\n"));
}

/*
 * Enable the ldo
 */
void enable_ldo(void)
{
    PORTC_OUTSET = PIN4_bm;
    iodprintf_P(PSTR("LDO On\r\n"));
}

/*
 * Disable the LDO
 */
void disable_ldo(void)
{
    PORTC_OUTCLR = PIN4_bm;
    iodprintf_P(PSTR("LDO Off\r\n"));
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
 * Enable resistor mux
 * @param   val     the resistor to be selected (see res_mux_t)
 */
void enable_res_mux(uint8_t val)
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
    
    // Debug print
    switch(val)
    {
        case RES_270:
        {
            iodprintf_P(PSTR("Resistor Mux: 270R\r\n"));
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
 * Enable everything to allow freq measurement
 * @param   res     the resistor to be selected (see res_mux_t)
 */
void set_measurement_mode_io(uint8_t res)
{
    enable_res_mux(res);
    enable_feedback_mos();
    disable_cur_meas_mos();
}

/*
 * Disable the ios for freq measurement
 */
void disable_measurement_mode_io(void)
{
    disable_res_mux();
    disable_feedback_mos();
}

/*
 * Init project IOs
 */
void init_ios(void)
{
    iodprintf_P(PSTR("-----------------------\r\n"));
    iodprintf_P(PSTR("IO init\r\n"));
    disable_cur_meas_mos();                                 // Disable cur measurement mosfet
    disable_feedback_mos();                                 // Disable opamp feedback
    disable_res_mux();                                      // Disable resistor mux
    disable_stepup();                                       // Disable stepup
    disable_ldo();                                          // Disable LDO
    PORTA_DIRSET = PIN0_bm | PIN2_bm | PIN3_bm | PIN4_bm;   // CON_SH, MUX_A1, MUX_A0, MUX_EN
    PORTB_DIRSET = PIN1_bm;                                 // AMP_FB_EN
    PORTC_DIRSET = PIN4_bm | PIN5_bm;                       // LDO_EN, SU_EN
}