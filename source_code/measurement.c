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
// Resistor mux modes in order of value
uint8_t res_mux_modes[] = {RES_470, RES_1K, RES_10K, RES_100K};
uint8_t digital_filter_for_res[] = {8, 8, 6, 4};
#define DEFAULT_RES_INDEX   3
// Error flag
volatile uint8_t tc_error_flag = FALSE;
// Current counter for the fall/rise
volatile uint32_t current_counter_rise;
volatile uint32_t current_counter_fall;
// Current aggregate for the fall/rise
volatile uint32_t current_agg_rise;
volatile uint32_t current_agg_fall;
// Last counter for the fall/rise
volatile uint32_t last_counter_rise;
volatile uint32_t last_counter_fall;
// Last aggregate for the fall/rise
volatile uint32_t last_agg_rise;
volatile uint32_t last_agg_fall;
// Frequency counter current value
volatile uint32_t cur_freq_counter_val;
// Last frequency counter value
volatile uint16_t last_counter_val;
// Number of freq timer overflows
volatile uint8_t nb_freq_overflows;
// Current resistor for measure
volatile uint8_t cur_resistor_index;
// Counter to discard next measure
volatile uint8_t discard_next_mes_cnt;
// Consecutive tc_error_flags seen on smaller R
volatile uint8_t consec_tc_error_flags;
// Current measurement frequency
uint16_t cur_freq_meas = FREQ_2HZ;
// New measurement value
volatile uint8_t new_val_flag;
// Number of consecutive freq errors
uint8_t nb_conseq_freq_pb = 0;
// Current counter divider
uint8_t cur_counter_divider;


/*
 * Timer counter 0 overflow interrupt (pulse width counter)
 */
ISR(TCC0_OVF_vect)
{
    // If we have an overflow, it means we couldn't measure the pulse width
    // That would mean 65536/32000000 = 2ms so around 500Hz oscillations when counter divider is 1 
    tc_error_flag = TRUE;
}

/*
 * Timer counter 1 overflow interrupt (oscillator frequency counter)
 */
ISR(TCC1_OVF_vect)
{
    // Frequency counter rolled over
    nb_freq_overflows++;
}

/*
 * Channel A capture interrupt on TC1, triggered by the RTC
 * Here we copy our counter values and aggregates
 */
ISR(TCC1_CCA_vect)
{
    uint16_t count_value = TCC1.CCA;
    cur_freq_counter_val = 0;
    
    // Compute frequency counter value
    if (count_value < last_counter_val)
    {
        nb_freq_overflows--;
    }
    cur_freq_counter_val = nb_freq_overflows * 0x10000 + (count_value - last_counter_val);
    
    // Copy aggregates & counters, reset counters
    last_counter_val = count_value;                 // Copy current freq counter val
    last_agg_fall = current_agg_fall;               // Copy current aggregate
    last_agg_rise = current_agg_rise;               // Copy current aggregate
    last_counter_fall = current_counter_fall;       // Copy current counter
    last_counter_rise = current_counter_rise;       // Copy current counter
    current_counter_fall = 0;                       // Reset counter
    current_counter_rise = 0;                       // Reset counter
    current_agg_fall = 0;                           // Reset agg
    current_agg_rise = 0;                           // Reset agg
    nb_freq_overflows = 0;                          // Reset overflow
    
    // Only do the following operation if we weren't asked to discard next measure
    if (discard_next_mes_cnt == 0)
    {
        // If we got an error flag, the oscillations are too slow and the measurement isn't valid (pulse width at around 1k)
        if (tc_error_flag == TRUE)
        {
            if (cur_resistor_index > 0)
            {
                // Set smaller resistor, discard next measurement
                adjust_digital_filter(digital_filter_for_res[--cur_resistor_index]);
                enable_res_mux(res_mux_modes[cur_resistor_index], TRUE);
                discard_next_mes_cnt = 1;
            }
            else if (cur_counter_divider < TC_CLKSEL_DIV256_gc)
            {
                // Increase counter divider
                discard_next_mes_cnt = 1;
                TCC0.CTRLA = ++cur_counter_divider;
                measdprintf("Count div: %d\r\n", cur_counter_divider);
            }            
            else
            {
                if (consec_tc_error_flags++ > NB_CONSEQ_TC_ERR_FLAG_CHG_RES)
                {
                    // Set resistor mux to 10k, reset counter divider
                    cur_counter_divider = TC_CLKSEL_DIV1_gc;
                    TCC0.CTRLA = cur_counter_divider;
                    cur_resistor_index += 2;
                    discard_next_mes_cnt = 1;
                    measdprintf("Count div: %d\r\n", cur_counter_divider);
                    enable_res_mux(res_mux_modes[cur_resistor_index], TRUE);
                    adjust_digital_filter(digital_filter_for_res[cur_resistor_index]);
                }                
            }
            tc_error_flag = FALSE;
        }
        else
        {
            // New values to be read
            new_val_flag = TRUE;
        }
    }  
    else
    {
        discard_next_mes_cnt--;
        tc_error_flag = FALSE;
    }  
}    

/*
 * Channel A capture interrupt on TC0 (pulse width counter) 
 */
ISR(TCC0_CCA_vect)
{
    volatile uint16_t cur_pulse_width = TCC0.CCA;    

    // Aggregate depending if the voltage is rising / falling
    if ((PORTA_IN & PIN6_bm) == 0)
    {
        current_agg_fall += cur_pulse_width;
        current_counter_fall++;
    }
    else
    {
        current_counter_rise += cur_pulse_width;
        current_counter_rise++;
    }        
}

/*
 * RTC overflow interrupt
 */
ISR(RTC_OVF_vect)
{
}

/*
 * Discard a given number of capacitance measurements
 * @param   nb_samples  Number of samples to discard
 */
void discard_next_cap_measurements(uint8_t nb_samples)
{
    // copy done in one clock cycle, safe
    discard_next_mes_cnt = nb_samples;
}

/*
 * Adjust the digital filter on the input signal
 * @param   nb_samples  Number of consecutive samples
 */
void adjust_digital_filter(uint8_t nb_samples)
{
    EVSYS.CH0CTRL = nb_samples - 1;
}

/*
 * Capacitance measurement logic - change resistor, freq measurement...
 */
void cap_measurement_logic(void)
{   
    // If a change wasn't made before coming here
    if (discard_next_mes_cnt == 0)
    {
        if (last_agg_fall < (last_counter_fall<<7))
        {
            // Check that we're not oscillating too fast (average counter value less than 128 to keep a 1% precision >> 32M/128(counter)/5(ratio pulse width/period) = 50kHz)
            if (nb_conseq_freq_pb++ > NB_CONSEQ_FREQ_PB_CHG_RES)
            {
                // Check counter divider
                if (cur_counter_divider != TC_CLKSEL_DIV1_gc)
                {
                    // Decrease counter divider
                    TCC0.CTRLA = --cur_counter_divider;
                    measdprintf("Count div: %d\r\n", cur_counter_divider);
                    discard_next_mes_cnt = 1;
                } 
                else if(cur_resistor_index < sizeof(res_mux_modes)-1)
                {
                    // Decrease resistor value
                    adjust_digital_filter(digital_filter_for_res[++cur_resistor_index]);
                    enable_res_mux(res_mux_modes[cur_resistor_index], TRUE);
                    discard_next_mes_cnt = 1;
                }      
                nb_conseq_freq_pb = 0;
            }
        }
        else if ((last_agg_fall > (last_counter_fall<<11)) && (cur_resistor_index > 0))
        {
            // Check that we're not oscillating too slow (average counter value more than 128*16=2048)
            if (nb_conseq_freq_pb++ > NB_CONSEQ_FREQ_PB_CHG_RES)
            {
                adjust_digital_filter(digital_filter_for_res[--cur_resistor_index]);
                enable_res_mux(res_mux_modes[cur_resistor_index], TRUE);
                nb_conseq_freq_pb = 0;
            }
        }
        else
        {
            nb_conseq_freq_pb = 0;
        }
    }    
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
 * Set the frequency at which counter values will be returned
 * @param   bit_shift   1Hz division bit shift (0 is 1Hz, 1 is 2Hz, 2 is 4Hz...)
 */
void set_capacitance_report_frequency(uint8_t bit_shift)
{
    cur_freq_meas = (32768 >> bit_shift) - 1;
}

/*
 * Set capacitance measurement mode
 */
void set_capacitance_measurement_mode(void)
{    
    discard_next_mes_cnt = 2;                                       // Discard next measures by default
    cur_resistor_index = DEFAULT_RES_INDEX;                         // Last resistor by default
    cur_counter_divider = TC_CLKSEL_DIV1_gc;                        // Counter divider 1
    // RTC: set period depending on measurement freq
    RTC.PER = cur_freq_meas;                                        // Set correct RTC timer freq
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;                               // Keep the 32kHz base clock for the RTC
    EVSYS.CH1MUX = EVSYS_CHMUX_RTC_OVF_gc;                          // Event line 1 for RTC overflow
    CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;              // Select 32kHz crystal for the RTC, enable it
    //RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;                              // Interrupt on RTC overflow (low priority)
    // IOs and event lines
    PORTA.DIRCLR = PIN6_bm;                                         // Set COMP_OUT as input
    PORTC.DIRSET = PIN7_bm;                                         // Set PC7 as EVOUT
    PORTE.DIRCLR = PIN0_bm | PIN1_bm | PIN3_bm;                     // Set PE0 & PE1 & PE3 as inputs
    PORTA.PIN6CTRL = PORT_ISC_RISING_gc;                            // Generate event on rising edge of COMPOUT
    PORTE.PIN3CTRL = PORT_ISC_BOTHEDGES_gc;                         // Generate events on both edges of T_FALL
    PORTE.PIN0CTRL = PORT_ISC_BOTHEDGES_gc;                         // Generate events on both edges of AN1_COMPOUT
    PORTE.PIN1CTRL = PORT_ISC_BOTHEDGES_gc | PORT_INVEN_bm;         // Generate events on both edges of AN2_COMPOUT, invert
    EVSYS.CH0MUX = EVSYS_CHMUX_PORTE_PIN3_gc;                       // Use event line 0 for T_FALL edges
    EVSYS.CH0CTRL = EVSYS_DIGFILT_4SAMPLES_gc;                      // Apply a digital filter of 4 samples
    EVSYS.CH2MUX = EVSYS_CHMUX_PORTA_PIN6_gc;                       // Use event line 2 for COMPOUT rising edge
    EVSYS.CH3MUX = EVSYS_CHMUX_PORTE_PIN0_gc;                       // Use event line 3 for AN1_COMPOUT edges
    EVSYS.CH4MUX = EVSYS_CHMUX_PORTE_PIN1_gc;                       // Use event line 4 for AN2_COMPOUT edges
    PORTCFG.CLKEVOUT = PORTCFG_EVOUT_PC7_gc;                        // Event line 0 output on PC7
    // TC0: pulse width capture of T_FALL
    TCC0.CNT = 0;                                                   // Reset counter
    TCC0.CTRLB = TC0_CCAEN_bm;                                      // Enable compare A on TCC0
    TCC0.CTRLD = TC_EVACT_PW_gc | TC_EVSEL_CH0_gc;                  // Pulse width capture on event line 0 (T_FALL)
    TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;                             // Overflow interrupt
    TCC0.INTCTRLB = TC_CCAINTLVL_HI_gc;                             // High level interrupt on capture
    TCC0.CTRLA = cur_counter_divider;                               // Set correct counter divider
    // TC1: frequency counter
    TCC1.CNT = 0;                                                   // Reset counter
    TCC1.PER = 0xFFFF;                                              // Set max period
    TCC1.CTRLB = TC1_CCAEN_bm;                                      // Enable compare A
    TCC1.CTRLD = TC_EVACT_CAPT_gc | TC_EVSEL_CH1_gc;                // Capture event on channel 1 (RTC)
    TCC1.CTRLA = TC_CLKSEL_EVCH2_gc;                                // Use event line 2 as frequency input (COMPOUT)
    TCC1.INTCTRLA = TC_OVFINTLVL_HI_gc;                             // Overflow interrupt
    TCC1.INTCTRLB = TC_CCAINTLVL_HI_gc;                             // High level interrupt on capture    
    
    switch(cur_freq_meas)
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
    
    // Start oscillations
    adjust_digital_filter(digital_filter_for_res[DEFAULT_RES_INDEX]);
    set_measurement_mode_io(res_mux_modes[cur_resistor_index]);
}

/*
 * Pause capacitance measurement mode
 */
void pause_capacitance_measurement_mode(void)
{
    discard_next_mes_cnt = 0xFF;
    disable_measurement_mode_io();
}

/*
 * Resume capacitance measurement mode
 */
void resume_capacitance_measurement_mode(void)
{
    discard_next_mes_cnt = 1;
    set_measurement_mode_io(res_mux_modes[cur_resistor_index]);
}

/*
 * Disable capacitance measurement mode
 */
void disable_capacitance_measurement_mode(void)
{
    TCC0.INTCTRLA = 0x00;                           // Disable timer counter interrupts
    TCC0.INTCTRLB = 0x00;                           // Disable timer counter interrupts
    TCC1.INTCTRLA = 0x00;                           // Disable timer counter interrupts
    TCC1.INTCTRLB = 0x00;                           // Disable timer counter interrupts
    TCC0.CTRLD = 0x00;                              // Disable counters
    TCC1.CTRLD = 0x00;                              // Disable counters
    TCC0.CNT = 0x0000;                              // Disable counters
    TCC1.CNT = 0x0000;                              // Disable counters
    current_counter_fall = 0;                       // Reset counter
    current_counter_rise = 0;                       // Reset counter
    current_agg_fall = 0;                           // Reset agg
    current_agg_rise = 0;                           // Reset agg
    nb_freq_overflows = 0;                          // Reset overflow
    last_counter_val = 0;                           // Reset last counter val
    disable_measurement_mode_io();                  // Disable measurement mode IOs
}

/*
 * Our main capacitance measurement loop
 * @param   cap_report  Pointer to where to store the capacitance measurement report
 */
uint8_t cap_measurement_loop(capacitance_report_t* cap_report)
{
    // Check if we have a new value to report
    if (new_val_flag == TRUE)
    {        
        // Store the report
        cap_report->counter_divider = get_val_for_counter_divider(cur_counter_divider);
        cap_report->half_res = get_half_val_for_res_mux_define(get_cur_res_mux());
        cap_report->report_freq = get_val_for_freq_define(cur_freq_meas);
        cap_report->second_thres = get_calib_second_thres_up();
        cap_report->first_thres = get_calib_first_thres_up();
        cap_report->counter_value = cur_freq_counter_val;
        cap_report->aggregate_fall = last_agg_fall;
        
        // Necessary to change the resistor...
        cap_measurement_logic();
        
        // Remove the new val flag
        new_val_flag = FALSE;
        
        if (FALSE)
        {
            //print_compute_c_formula(last_agg_fall, cur_freq_counter_val, cur_counter_divider, get_cur_res_mux());
            measdprintf("SYNC\r\n");
            measdprintf("%u\r\n", get_val_for_counter_divider(cur_counter_divider));
            measdprintf("%lu\r\n", last_agg_fall);
            measdprintf("%lu\r\n", cur_freq_counter_val);
            measdprintf("%u\r\n", get_half_val_for_res_mux_define(get_cur_res_mux()));
            measdprintf("%u\r\n", get_calib_second_thres_up());
            measdprintf("%u\r\n", get_calib_first_thres_up());
            measdprintf("%u\r\n\r\n", get_val_for_freq_define(cur_freq_meas));
        }     
        
        // Return TRUE         
        return TRUE;
    }
    // For debug purposes only
    if ((tc_error_flag == TRUE) && (discard_next_mes_cnt == 0))
    {
        measdprintf_P(PSTR("ERR "));
        _delay_ms(10);
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