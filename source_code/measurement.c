/*
 * measurement.c
 *
 * Created: 26/04/2015 11:37:49
 *  Author: limpkin
 */
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "eeprom_addresses.h"
#include "measurement.h"
#include "calibration.h"
#include "meas_io.h"
#include "vbias.h"
#include "main.h"
#include "dac.h"
#include "adc.h"
// Error flag
volatile uint8_t error_flag = FALSE;
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
// Current measurement frequency
uint16_t cur_freq_meas;


/*
 * Timer overflow interrupt
 */
ISR(TCC0_OVF_vect)
{
    // If we have an overflow, it means we couldn't measure the pulse width :/
    error_flag = TRUE;
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
 * Timer overflow interrupt
 */
ISR(TCC1_OVF_vect)
{
    // If we have an overflow, it means we couldn't measure the pulse width :/
    error_flag = TRUE;
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
 * Compute current measurement numerator from adc value (den is the ampl)
 * @param   adc_val     ADC value
 * @return  numerator
 * @note    Only precise at 0.118%
 */
uint16_t compute_cur_mes_numerator_from_adc_val(uint16_t adc_val)
{    
    /******************* MATHS *******************/    
    // Vadc = I(A) * 1k * 100 * ampl
    // Vadc = I(A) * 100k * ampl
    // I(A) = Vadc / (100k * ampl)
    // I(A) = Val(ADC) * (1.24 / 2047) / (100k * ampl)
    // I(A) = Val(ADC) * 1.24 / (204.7M * ampl)
    // I(nA) = Val(ADC) * 1.24 / (0.2047 * ampl)
    // I(nA) = Val(ADC) * 6,057645335 / ampl
    // I(nA) = Val(ADC)*5 + Val(ADC)*1,057645335 / ampl
    // I(nA) ~ Val(ADC)*5 + Val(ADC)*(18/17) / ampl
    
    uint16_t return_value = (adc_val * 18) / 17;
    return_value += (adc_val * 5);
    return return_value;
}

/*
 * Compute voltage from single ended adc measurement
 * @param   adc_val     ADC value
 * @return  the voltage
 * @note    Only precise at 0.07%
 */
uint16_t compute_voltage_from_se_adc_val(uint16_t adc_val)
{    
    /******************* MATHS *******************/    
    // Vadc = Val(ADC) * (1.24 / 4095)
    // Vadc(mV) ~ Val(ADC) * 10/33
    
    return (adc_val * 10)/33;
}

/*
 * Set the frequency at which we will perform the measurements
 * @param   freq     the frequency (see mes_freq_t)
 */
void set_measurement_frequency(uint16_t freq)
{    
    cur_freq_meas = freq;
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
    TCC0.PER = 0xFFFF;                                              // Set period to max
    TCC0.CTRLA = TC_CLKSEL_DIV1_gc;                                 // Use 32MHz as frequency input
    TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;                             // Overflow interrupt
    TCC0.INTCTRLB = TC_CCAINTLVL_HI_gc;                             // High level interrupt on capture
    // TC1: pulse width capture of AN2_COMPOUT
    TCC1.CTRLB = TC1_CCAEN_bm;                                      // Enable compare A on TCC1
    TCC1.CTRLD = TC_EVACT_PW_gc | TC_EVSEL_CH3_gc;                  // Pulse width capture on event line 3 (AN2_COMPOUT)
    TCC1.PER = 0xFFFF;                                              // Set period to max
    TCC1.CTRLA = TC_CLKSEL_DIV1_gc;                                 // Use 32MHz as frequency input
    TCC1.INTCTRLA = TC_OVFINTLVL_HI_gc;                             // Overflow interrupt
    
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
 * Get value for freq measurement define
 * @param   define  The define
 * @return  the value
 */
uint16_t get_val_for_freq_define(uint16_t define)
{
    switch(define)
    {
        case FREQ_1HZ:      return 1;
        case FREQ_32HZ:     return 32;
        case FREQ_64HZ:     return 64;
        case FREQ_128HZ:    return 128;
    }
    return 0;
}

/*
 * Get value for res mux measurement define
 * @param   define  The define
 * @return  the value divided by two
 */
uint16_t get_half_val_for_res_mux_define(uint16_t define)
{
    switch(define)
    {
        case RES_270:       return 135;
        case RES_100K:      return 50000;
        case RES_1K:        return 500;
        case RES_10K:       return 5000;
    }
    return 0;
}

/*
 * Print the formulate to compute the capacitance
 * @param   nb_ticks     Number of ticks
 */
void print_compute_c_formula(uint32_t nb_ticks)
{
    // C = 1 / 2 * half_r * freq_measurement * nb_ticks * (ln(3300-Vl/3300-vh) + ln(vh/vl))
    //measdprintf("Formula to compute C: 1 / (2 * %u * %u * %lu * (ln((3300-%u)/(3300-%u)) + ln(%u/%u)))\r\n", get_half_val_for_res_mux_define(get_cur_res_mux()), get_val_for_freq_define(cur_freq_meas), nb_ticks, compute_voltage_from_se_adc_val(get_calib_vlow()), compute_voltage_from_se_adc_val(get_calib_vup()), compute_voltage_from_se_adc_val(get_calib_vup()), compute_voltage_from_se_adc_val(get_calib_vlow()));
    //measdprintf("Formula to compute C: -1 / (2 * %u * %u * %lu * (ln((3300-%u)/(3300-%u)) + ln(%u/%u)))\r\n", get_half_val_for_res_mux_define(get_cur_res_mux()), get_val_for_freq_define(cur_freq_meas), nb_ticks, compute_voltage_from_se_adc_val(get_calib_vup()), compute_voltage_from_se_adc_val(get_calib_vlow()), compute_voltage_from_se_adc_val(get_calib_vlow()), compute_voltage_from_se_adc_val(get_calib_vup()));
}

/*
 * Our main measurement loop
 * @param   mes_mode     Our measurement mode (see enum_mes_mode_t)
 */
void measurement_loop(uint8_t ampl)
{
    // If we received a new measurement
    if (new_val_flag == TRUE)
    {
        new_val_flag = FALSE;
        print_compute_c_formula(last_agg);
        //measdprintf("%lu\r\n", last_measured_value);
    }
}

/*
 * Set quiescent current measurement mode
 * @param   ampl        Our measurement amplification (see enum_cur_mes_mode_t)
 */
void set_current_measurement_ampl(uint8_t ampl)
{
    disable_feedback_mos();
    disable_res_mux();
    enable_cur_meas_mos();
    configure_adc_channel(ADC_CHANNEL_CUR, ampl, TRUE);
    start_and_wait_for_adc_conversion();
}

/*
 * Disable current measurement mode
 */
void disable_current_measurement_mode(void)
{    
    disable_cur_meas_mos();
}

/*
 * Print resistance computation formula
 * @param   adc     Current ADC val for current measurement
 */
void print_compute_r_formula(uint16_t adc_val)
{    
    if (adc_val >= get_max_value_for_diff_channel(get_configured_adc_ampl())-10)
    {
        measdprintf_P(PSTR("ADC val too high, measurement invalid\r\n"));
        return;
    }
    /******************* MATHS *******************/
    // Vadc = I(A) * 1k * 100 * ampl
    // Vadc = I(A) * 100k * ampl
    // I(A) = Vadc / (100k * ampl)
    // I(A) = Val(ADC) * (1.24 / 2047) / (100k * ampl)
    // I(A) = Val(ADC) * 1.24 / (204.7M * ampl)
    // I(nA) = Val(ADC) * 1.24 / (0.2047 * ampl)
    measdprintf("Measured current in nA: %u * 1.24 / (0.2047 * %u)\r\n", adc_val, 1 << get_configured_adc_ampl());
    
    // Depending if we performed the advanced current measurement calibration
    if (get_advanced_current_calib_done() == TRUE)
    {                
        // Correcting for gain
        // During calibration:
        // Icalib(nA) * X = Vbiascalib / 1.01
        // X = Vbiascalib / (1.01 * Icalib(nA))
        // Corrected result:
        // I(nA) = Val(ADC) * 1.24 * Vbiascalib / (0.2047 * ampl * 1.01 * Icalib(nA)) 
        // I(nA) = Val(ADC) * Vbiascalib / (1.01 * Val(ADC)calib)
        // Final R measurement:
        // R + 11k = U / I
        // R = U/I - 11k
        measdprintf_P(PSTR("Advanced current calibration values available, applying gain correction...\r\n"));
        measdprintf("Adjusted current in nA: (%u * %u)/(%u * 1.01)\r\n", adc_val, get_vbias_for_gain_correction(get_configured_adc_ampl()), get_adc_cur_value_for_gain_correction(get_configured_adc_ampl()));
        measdprintf("Resulting R: (%u * %u * 1.01)) / (%u * %u))*1000000 - 11000\r\n", get_last_measured_vbias(), get_adc_cur_value_for_gain_correction(get_configured_adc_ampl()), adc_val, get_vbias_for_gain_correction(get_configured_adc_ampl()));
    } 
    else
    {
        // TODO
    }    
}

/*
 * Our main current measurement loop
 * @param   ampl        Our measurement amplification (see enum_cur_mes_mode_t)
 */
uint16_t quiescent_cur_measurement_loop(uint8_t avg_bitshift)
{    
    // Check that the adc channel remained the same
    if (get_configured_adc_channel() != ADC_CHANNEL_CUR)
    {
        configure_adc_channel(ADC_CHANNEL_CUR, get_configured_adc_ampl(), FALSE);
    }
    
    uint16_t cur_val = get_averaged_adc_value(avg_bitshift);    
    return cur_val;
}