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
#include "main.h"
#include "dac.h"
#include "adc.h"
// To indicate the number of timer overflows
volatile uint8_t nb_overflows;
// Last counter value
volatile uint16_t last_counter_val;
// Counter values
volatile uint16_t last_measured_value;
// New measurement value
volatile uint8_t new_val_flag;
// Current measurement frequency
uint16_t cur_freq_meas;

/*
 * Timer overflow interrupt
 */
ISR(TCC0_OVF_vect)
{
    nb_overflows++;
}

/*
 * Channel A capture interrupt
 */
ISR(TCC0_CCA_vect)
{
    uint16_t count_value = TCC0.CCA;
    
    // Check if the count number isn't more than uint16_t
    if (((nb_overflows == 1) && (count_value > last_counter_val)) || (nb_overflows > 1))
    {
        last_measured_value = 0xFFFF;
    }
    else
    {
        last_measured_value = count_value - last_counter_val;
    }
    
    // Set correct values
    nb_overflows = 0;
    last_counter_val = count_value;
    
    // Set Flag
    new_val_flag = TRUE;
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
 * Set the frequency at which we will perform the measurements
 * @param   freq     the frequency (see mes_freq_t)
 */
void set_measurement_frequency(uint16_t freq)
{    
    cur_freq_meas = freq;
    RTC.PER = freq;                                     // Set correct RTC timer freq
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;                   // Keep the 32kHz base clock
    EVSYS.CH1MUX = EVSYS_CHMUX_RTC_OVF_gc;              // Event line 1 for RTC overflow
    CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;  // Select 32kHz crystal, enable
    PORTA.DIRCLR = PIN6_bm;                             // COMP_OUT
    PORTC.DIRSET = PIN7_bm;                             // EVOUT
    PORTA.PIN6CTRL = PORT_ISC_RISING_gc;                // Generate event on rising edge
    EVSYS.CH0MUX = EVSYS_CHMUX_PORTA_PIN6_gc;           // Event line 0 for COMP_OUT
    PORTCFG.CLKEVOUT = PORTCFG_EVOUT_PC7_gc;            // Event line 0 output on PC7
    TCC0.CTRLB = TC0_CCAEN_bm;                          // Enable compare A
    TCC0.CTRLD = TC_EVACT_CAPT_gc | TC_EVSEL_CH1_gc;    // Capture event on channel 1
    TCC0.PER = 0xFFFF;                                  // Set period to max
    TCC0.CTRLA = TC_CLKSEL_EVCH0_gc;                    // Use event line 0 as frequency input
    TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;                 // Overflow interrupt
    TCC0.INTCTRLB = TC_CCAINTLVL_HI_gc;                 // High level interrupt on capture
    
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
void print_compute_c_formula(uint16_t nb_ticks)
{
    // C = 1 / 2 * half_r * freq_measurement * nb_ticks * (ln(3300-Vl/3300-vh) + ln(vh/vl))
    measdprintf("Formula to compute C: 1 / (2 * %u * %u * %u * (ln((3300-%u)/(3300-%u)) + ln(%u/%u)))\r\n", get_half_val_for_res_mux_define(get_cur_res_mux()), get_val_for_freq_define(cur_freq_meas), nb_ticks, (get_calib_vlow()*10)/33, (get_calib_vup()*10)/33, (get_calib_vup()*10)/33, (get_calib_vlow()*10)/33);
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
        print_compute_c_formula(last_measured_value);
        measdprintf("%u\r\n", last_measured_value);
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
 * Our main current measurement loop
 * @param   ampl        Our measurement amplification (see enum_cur_mes_mode_t)
 */
uint16_t quiescent_cur_measurement_loop(uint8_t avg_bitshift)
{
    // Vadc = I(A) * 1k * 100 * ampl
    // Vadc = I(A) * 100k * ampl
    // I(A) = Vadc / (100k * ampl)
    // I(A) = Val(ADC) * (1.24 / 2048) / (100k * ampl)
    // I(A) = Val(ADC) * 1.24 / (204.8M * ampl)
    // I(nA) = Val(ADC) * 1.24 / (0.2048 * ampl)
    // I(nA) = Val(ADC) * 6,0546875 / ampl
    
    // Check that the adc channel remained the same
    if (get_configured_adc_channel() != ADC_CHANNEL_CUR)
    {
        configure_adc_channel(ADC_CHANNEL_CUR, get_configured_adc_ampl(), FALSE);
    }
    
    uint16_t cur_val = get_averaged_adc_value(avg_bitshift);
    measdprintf("Quiescent current: %u, approx %u/%unA\r\n", cur_val, compute_cur_mes_numerator_from_adc_val(cur_val), 1 << get_configured_adc_ampl());
    
    return cur_val;
}