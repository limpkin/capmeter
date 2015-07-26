/*
 * measurement.c
 *
 * Created: 26/04/2015 11:37:49
 *  Author: limpkin
 */
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "conversions.h"
#include "measurement.h"
#include "calibration.h"
#include "meas_io.h"
#include "vbias.h"
#include "dac.h"
#include "adc.h"
// Error flag
volatile uint8_t tc_error_flag2 = FALSE;
// Error flag
volatile uint8_t tc_error_flag = FALSE;
// Current counter
volatile uint32_t current_counter;
// Current aggregate
volatile uint32_t current_agg;
// Last counter
volatile uint32_t last_counter;
// Last aggregate
volatile uint32_t last_agg;
// New measurement value
volatile uint8_t new_val_flag;
// Current counter divider
uint8_t cur_counter_divider;
// Current measurement frequency
uint16_t cur_freq_meas;


/*
 * Timer overflow interrupt
 */
ISR(TCC0_OVF_vect)
{
    // If we have an overflow, it means we couldn't measure the pulse width :/
    tc_error_flag = TRUE;
}

/*
 * Timer overflow interrupt
 */
ISR(TCC1_OVF_vect)
{
    // If we have an overflow, it means we couldn't measure the pulse width :/
    tc_error_flag2 = TRUE;
}

/*
 * Channel A capture interrupt on TC0
 */
ISR(TCC0_CCA_vect)
{
    // Pulse width of AN1_COMPOUT, subtract it with pulse width of AN2_COMPOUT
    current_agg += (TCC0.CCA - TCC1.CCA);
    current_counter++;
}

/*
 * RTC overflow interrupt
 */
ISR(RTC_OVF_vect)
{
    new_val_flag = TRUE;                // Indicate new values to be read
    last_agg = current_agg;             // Copy current aggregate
    last_counter = current_counter;     // Copy current counter
    current_counter = 0;                // Reset counter
    current_agg = 0;                    // Reset agg
}

/*
 * Set quiescent current measurement mode
 * @param   ampl        Our measurement amplification (see enum_cur_mes_mode_t)
 */
void set_current_measurement_mode(uint8_t ampl)
{
    disable_feedback_mos();
    disable_res_mux();
    enable_cur_meas_mos();
    configure_adc_channel(ADC_CHANNEL_CUR, ampl, TRUE);
}

/*
 * Disable current measurement mode
 */
void disable_current_measurement_mode(void)
{    
    disable_cur_meas_mos();
}

/*
 * Set capacitance measurement mode
 * @param   freq                the frequency (see mes_freq_t)
 * @param   counter_divider     The TCCx counter divider (TC_CLKSEL_DIV2_gc etc)
 */
void set_capacitance_measurement_mode(uint16_t freq, uint8_t counter_divider)
{    
    cur_freq_meas = freq;
    cur_counter_divider = counter_divider;
    // RTC: set period depending on measurement freq
    RTC.PER = freq;                                                 // Set correct RTC timer freq
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;                               // Keep the 32kHz base clock for the RTC
    EVSYS.CH1MUX = EVSYS_CHMUX_RTC_OVF_gc;                          // Event line 1 for RTC overflow
    CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;              // Select 32kHz crystal for the RTC, enable it
    RTC.INTCTRL = RTC_OVFINTLVL_gm;                                 // Interrupt on RTC overflow
    // IOs and event lines
    PORTA.DIRCLR = PIN6_bm;                                         // Set COMP_OUT as input
    PORTC.DIRSET = PIN7_bm;                                         // Set PC7 as EVOUT
    PORTE.DIRCLR = PIN2_bm | PIN3_bm;                               // Set PE2 & PE3 as inputs
    PORTA.PIN6CTRL = PORT_ISC_RISING_gc;                            // Generate event on rising edge of COMPOUT
    PORTE.PIN3CTRL = PORT_ISC_BOTHEDGES_gc;                         // Generate events on both edges of AN2_COMPOUT
    PORTE.PIN2CTRL = PORT_ISC_BOTHEDGES_gc | PORT_INVEN_bm;         // Generate events on both edges of AN1_COMPOUT, invert
    EVSYS.CH0MUX = EVSYS_CHMUX_PORTA_PIN6_gc;                       // Use event line 0 for COMP_OUT rising edge
    EVSYS.CH2MUX = EVSYS_CHMUX_PORTE_PIN2_gc;                       // Use event line 2 for AN1_COMPOUT edges
    EVSYS.CH3MUX = EVSYS_CHMUX_PORTE_PIN3_gc;                       // Use event line 3 for AN2_COMPOUT edges
    PORTCFG.CLKEVOUT = PORTCFG_EVOUT_PC7_gc;                        // Event line 0 output on PC7
    // TC0: pulse width capture of AN1_COMPOUT
    TCC0.CTRLB = TC0_CCAEN_bm;                                      // Enable compare A on TCC0
    TCC0.CTRLD = TC_EVACT_PW_gc | TC_EVSEL_CH2_gc;                  // Pulse width capture on event line 2 (AN1_COMPOUT)
    TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;                             // Overflow interrupt
    TCC0.INTCTRLB = TC_CCAINTLVL_HI_gc;                             // High level interrupt on capture
    TCC0.CTRLA = counter_divider;                                   // Set correct counter divider
    // TC1: pulse width capture of AN2_COMPOUT
    TCC1.CTRLB = TC1_CCAEN_bm;                                      // Enable compare A on TCC1
    TCC1.CTRLD = TC_EVACT_PW_gc | TC_EVSEL_CH3_gc;                  // Pulse width capture on event line 3 (AN2_COMPOUT)
    TCC1.INTCTRLA = TC_OVFINTLVL_HI_gc;                             // Overflow interrupt
    TCC1.CTRLA = counter_divider;                                   // Set correct counter divider
    
    switch(freq)
    {
        case FREQ_1HZ:
        {
            measdprintf_P(PSTR("Measurement frequency set to 1Hz\r\n"));
            break;
        }
        case FREQ_32HZ:
        {
            measdprintf_P(PSTR("Measurement frequency set to 32Hz\r\n"));
            break;
        }
        case FREQ_64HZ:
        {
            measdprintf_P(PSTR("Measurement frequency set to 64Hz\r\n"));
            break;
        }
        case FREQ_128HZ:
        {
            measdprintf_P(PSTR("Measurement frequency set to 128Hz\r\n"));
            break;
        }
        default: break;
    }
}

/*
 * Our main capacitance measurement loop
 */
uint8_t cap_measurement_loop(uint8_t temp)
{
    // If we received a new measurement
    if (tc_error_flag == TRUE)
    {
        tc_error_flag = FALSE;
        measdprintf_P(PSTR("ERR\r\n"));
    }
    if (tc_error_flag2 == TRUE)
    {
        tc_error_flag2 = FALSE;
        measdprintf_P(PSTR("ERR2\r\n"));
    }    
    if (new_val_flag == TRUE)
    {
        new_val_flag = FALSE;
        //measdprintf_P(PSTR("."));
        measdprintf("agg: %lu, counter: %lu\r\n", last_agg, last_counter);
        if (temp == FALSE)
        {
            print_compute_c_formula(last_agg, last_counter, cur_counter_divider, get_cur_res_mux());
        }        
        return TRUE;
        //print_compute_c_formula(last_agg);
    }
    return FALSE;
}

/*
 * Our main current measurement loop
 * @param   avg_bitshift    Bit shift for averaging
 */
uint16_t cur_measurement_loop(uint8_t avg_bitshift)
{    
    // Check that the adc channel remained the same
    if (get_configured_adc_channel() != ADC_CHANNEL_CUR)
    {
        configure_adc_channel(ADC_CHANNEL_CUR, get_configured_adc_ampl(), FALSE);
    }
    
    uint16_t cur_val = get_averaged_adc_value(avg_bitshift);    
    return cur_val;
}