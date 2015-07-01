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
// Current set bias voltage
uint16_t cur_set_vbias_voltage;
// Current vbias dac_val
uint16_t cur_vbias_dac_val;
// Calibrated vup
uint16_t calib_vup;
// Calibrated vlow
uint16_t calib_vlow;
// Opamp 0v output voltage when no load
uint16_t opamp_0v_output_no_load;
// Opamp 0v output voltages at vbias 3.3v depending on rout
uint16_t opamp_0v_outputs_for_3v[4];
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
 * Init measurement
 */
void init_measurement(void)
{
    measdprintf_P(PSTR("-----------------------\r\n"));
    measdprintf_P(PSTR("Measurement Init\r\n"));
    
    // Fetch vup vlow calibration data if stored
    if (eeprom_read_byte((uint8_t*)CALIB_VUP_VLOW_BOOL) == EEPROM_BOOL_OK_VAL)
    {
        calib_vup = eeprom_read_word((uint16_t*)CALIB_VUP);
        calib_vlow = eeprom_read_word((uint16_t*)CALIB_VDOWN);
        measdprintf_P(PSTR("Vup/Vlow calibration values fetched from EEPROM\r\n"));
    }    
}

/*
 * Update bias voltage
 * @param   val     bias voltage in mV
 * @return  Actual mV value set
 */
uint16_t update_bias_voltage(uint16_t val_mv)
{
    measdprintf("Vbias call for %umV\r\n", val_mv);
    uint16_t measured_vbias = 0, temp_val;
    uint8_t peak_peak = 14;
    
    /******************* MATHS *******************/
    // Vadc = Vbias * 1.2 / (1.2+15)
    // Vadc = Vbias * 1.2 / 16.2
    // Vbias = Vadc * 16.2 / 1.2
    // Vbias(mV) = VALadc * Vref / 4095 * 16.2 / 1.2
    // Vbias(mV) = VALadc * 1240 / 4095 * 16.2 / 1.2
    // Vbias(mV) = VALadc * 20088 / 4914
    // Vbias(mV) = VALadc * 4,0879120879120879120879120879121
    // Vbias(mV) = VALadc * (4 + 0,0879120879120879120879120879121)
    // Vbias(mV) = VALadc * 4 + VALadc * 16 / 182
    /******************* MEASUREMENTS *******************/
    // On fluke45, one set (to be automatized)
    // 1500mV  => 1506mV   => 0.4%
    // 2000mV  => 2004mV   => 0.2%
    // 3000mV  => 3004mV   => 0.13%
    // 4000mV  => 3996mV   => 0.1%
    // 5000mV  => 4998mV   => 0.04%
    // 6000mV  => 5997mV   => 0.05%
    // 7000mV  => 6998mV   => 0.03%
    // 8000mV  => 7990mV   => 0.125%
    // 9000mV  => 8988mV   => 0.13%
    // 10000mV => 9984mV   => 0.16%
    // 11000mV => 10983mV  => 0.15%
    // 12000mV => 11982mV  => 0.15%
    // 13000mV => 12985mV  => 0.11%
    // 14000mV => 13980mV  => 0.14%
    // 15000mV => 14977mV  => 0.15%
    // Which is quite awesome when you notice than one ADC LSB actually is 4mV!
    
    // Check that value isn't too low...
    if (val_mv < VBIAS_MIN_V)
    {
        measdprintf_P(PSTR("Value too low, setting it to 1250mV!\r\n"));
        val_mv = VBIAS_MIN_V;
    }
    
    // Ramp up or ramp low depending on currently set vbias
    if (cur_set_vbias_voltage == val_mv)
    {
        measdprintf_P(PSTR("Same val requested!\r\n"));
        return val_mv;
    }
    else if (cur_set_vbias_voltage > val_mv)
    {
        measured_vbias = 0xFFFF;
        
        // Check if we need to deactivate the stepup
        if ((cur_set_vbias_voltage >= STEPUP_ACTIV_V) && (val_mv < STEPUP_ACTIV_V))
        {
            disable_stepup();                                   // Disable stepup
            _delay_ms(1);                                       // Wait a bit
        }
        
        // Our voltage decreasing loop (takes 40ms to reach vmax)
        while ((measured_vbias > val_mv) && (cur_vbias_dac_val != DAC_MAX_VAL))
        {
            update_vbias_dac(cur_vbias_dac_val++);
            _delay_us(20);
            measured_vbias = get_averaged_stabilized_adc_value(8, peak_peak, FALSE);
            temp_val = (measured_vbias * 16 / 182);
            measured_vbias = (measured_vbias * 4) + temp_val;
            
            // Adjust peak-peak depending on how close we are
            if ((val_mv - measured_vbias) < 300)
            {
                peak_peak = 0;
            }
        }
    } 
    else
    {
        measured_vbias = 0;
        
        // Check if we need to activate the stepup
        if ((cur_set_vbias_voltage < STEPUP_ACTIV_V) && (val_mv >= STEPUP_ACTIV_V))
        {            
            enable_stepup();                                    // Enable stepup
            _delay_ms(10);                                      // Step up start takes around 1.5ms (oscilloscope)
        }
        
        // Our voltage increasing loop (takes 40ms to reach vmax)
        while ((measured_vbias < val_mv) && (cur_vbias_dac_val != 0)) 
        {
            update_vbias_dac(cur_vbias_dac_val--);
            _delay_us(20);
            measured_vbias = get_averaged_stabilized_adc_value(8, peak_peak, FALSE);
            temp_val = (measured_vbias * 16 / 182);
            measured_vbias = (measured_vbias * 4) + temp_val;
            
            // Adjust peak-peak depending on how close we are
            if ((measured_vbias - val_mv) < 300)
            {
                peak_peak = 0;
            }
        }
    }
    
    // Wait before continuing
    _delay_ms(10);     
    cur_set_vbias_voltage = val_mv;                                 
    measdprintf("Vbias set, actual value: %umV\r\n", measured_vbias);
    return measured_vbias;
}

/*
 * To measure set voltages
 */
void bias_voltage_test(void)
{
    enable_bias_voltage(1500);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(2000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(3000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(4000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(5000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(6000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(7000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(8000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(9000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(10000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(11000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(12000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(13000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(14000);_delay_ms(5000);disable_bias_voltage();
    enable_bias_voltage(15000);_delay_ms(5000);disable_bias_voltage();    
}

/*
 * Enable bias voltage
 * @param   val     bias voltage in mV (min 1250mv, max 15300mV)
 * @return  Actual mV value set
 */
uint16_t enable_bias_voltage(uint16_t val_mv)
{
    cur_set_vbias_voltage = 0;                          // Set min vbias voltage by default
    cur_vbias_dac_val = VBIAS_MIN_DAC_VAL;              // Set min vbias voltage by default
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0);        // Enable ADC for vbias monitoring
    setup_vbias_dac(cur_vbias_dac_val);                 // Start with lowest voltage possible
    enable_ldo();                                       // Enable ldo   
    _delay_ms(100);                                     // Soft start wait
    return update_bias_voltage(val_mv);                 // Return the actual voltage that was set
}

/*
 * Wait for 1v bias
 */
void wait_for_1v_bias(void)
{
    uint16_t measured_vbias = 2000;
    
    measdprintf_P(PSTR("-----------------------\r\n"));
    measdprintf_P(PSTR("Waiting for low bias voltage...\r\n"));
    
    // Wait for bias voltage to be under ~1000mV
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0);
    while (measured_vbias > 245)
    {
        measured_vbias = get_averaged_stabilized_adc_value(6, 8, FALSE);
    }
    
    measdprintf_P(PSTR("Bias voltage at 1V\r\n"));    
}

/*
 * Disable bias voltage
 */
void disable_bias_voltage(void)
{
    measdprintf_P(PSTR("Disabling bias voltage\r\n"));  // Debug
    disable_ldo();                                      // Disable LDO
    disable_stepup();                                   // Disable stepup
    disable_vbias_dac();                                // Disable DAC controlling the ldo
    wait_for_1v_bias();                                 // Wait bias voltage to be at 1v
}

/*
 * Calibrate vup/vlow
 */
void calibrate_vup_vlow(void)
{
    measdprintf_P(PSTR("-----------------------\r\n"));
    measdprintf_P(PSTR("Vup/Vlow calibration\r\n"));
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    disable_feedback_mos();
    _delay_ms(10);
    enable_bias_voltage(4000);
    
    // Leave time for a possible cap to charge
    _delay_ms(1000);
    setup_opampin_dac(DAC_MIN_VAL);
    calib_vlow = DAC_MAX_VAL;
    calib_vup = DAC_MIN_VAL;
    
    // Ramp up, wait for toggle
    while((PORTA_IN & PIN6_bm) != 0)
    {
        update_opampin_dac(++calib_vup);
        _delay_us(10);
    }
    
    measdprintf("Vhigh found: %u, approx %umV\r\n", calib_vup, (calib_vup*10)/33);
    update_opampin_dac(calib_vlow);
    _delay_us(10);
        
    // Ramp low, wait for toggle
    while((PORTA_IN & PIN6_bm) == 0)
    {
        update_opampin_dac(--calib_vlow);
        _delay_us(10);
    }
    
    measdprintf("Vlow found: %u, approx %umV\r\n", calib_vlow, (calib_vlow*10)/33);
    
    eeprom_write_word((uint16_t*)CALIB_VUP, calib_vup);
    eeprom_write_word((uint16_t*)CALIB_VDOWN, calib_vlow);
    eeprom_write_byte((uint8_t*)CALIB_VUP_VLOW_BOOL, EEPROM_BOOL_OK_VAL);
    measdprintf_P(PSTR("Values stored in EEPROM\r\n"));
    
    disable_opampin_dac();
    disable_bias_voltage();
}

/*
 * Measure the opamp internal resistance
 */
void measure_opamp_internal_resistance(void)
{
    uint8_t cur_cal_res_mux = RES_270;    
    measdprintf_P(PSTR("-----------------------\r\n"));
    measdprintf_P(PSTR("Measuring opamp internal resistance\r\n"));
                
    configure_adc_channel(ADC_CHANNEL_COMPOUT,0);                                       // Configure ADC to measure the opamp output
    disable_feedback_mos();                                                             // Disable feedback mosfet
    disable_res_mux();                                                                  // Disable resistor mux
    _delay_ms(10);                                                                      // Wait before test
    setup_opampin_dac(calib_vup+100);                                                   // Force opamp output to 0 by setting IN- above IN+
    opamp_0v_output_no_load = get_averaged_stabilized_adc_value(8, 8, FALSE);           // Run averaging on 128 samples, max 8 lsb peak to peak noise    
    measdprintf("Opamp 0v output: %u, approx %umV\r\n", opamp_0v_output_no_load, (opamp_0v_output_no_load*10)/33);
    
    // Vbias should be short with the other terminal here
    enable_bias_voltage(3300);                                                          // Enable vbias at 3.3v to know its resistance during oscillations!
    enable_res_mux(cur_cal_res_mux);                                                    // Try all resistors one by one
    configure_adc_channel(ADC_CHANNEL_COMPOUT,0);                                       // Required as setting vbias uses the adc
    while (cur_cal_res_mux <= RES_10K)
    {
        opamp_0v_outputs_for_3v[cur_cal_res_mux] = get_averaged_stabilized_adc_value(7, 8, FALSE);  // Measure
        
        // Only pass through if the user shorted the terminals
        if (!((cur_cal_res_mux == RES_270) && (opamp_0v_outputs_for_3v[cur_cal_res_mux] < 300)) && !((cur_cal_res_mux == RES_1K) && (opamp_0v_outputs_for_3v[cur_cal_res_mux] < 80)))
        {
            measdprintf("Opamp 0v output at 3.3V: %u, approx %umV\r\n", opamp_0v_outputs_for_3v[cur_cal_res_mux], ((opamp_0v_outputs_for_3v[cur_cal_res_mux])*10)/33);
            
            #ifdef MEAS_PRINTF
                uint16_t voltage_mv = ((opamp_0v_outputs_for_3v[cur_cal_res_mux])*10)/33;
                if (cur_cal_res_mux == RES_270)
                {
                    measdprintf("Approx R of %umOhms\r\n", (voltage_mv*270) / (3300 - voltage_mv));
                }
                else if (cur_cal_res_mux == RES_1K)
                {
                    measdprintf("Approx R of %umOhms\r\n", (voltage_mv*1000) / (3300 - voltage_mv));
                }
            #endif
            
            enable_res_mux(++cur_cal_res_mux);
        }        
    }

    // We're done    
    disable_res_mux();
    disable_opampin_dac();
    disable_bias_voltage();
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
 * @return  the value divider by two
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
    measdprintf("Formula to compute C: 1 / (2 * %u * %u * %u * (ln((3300-%u)/(3300-%u)) + ln(%u/%u)))\r\n", get_half_val_for_res_mux_define(get_cur_res_mux()), get_val_for_freq_define(cur_freq_meas), nb_ticks, (calib_vlow*10)/33, (calib_vup*10)/33, (calib_vup*10)/33, (calib_vlow*10)/33);
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
    configure_adc_channel(ADC_CHANNEL_CUR, ampl);
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
void quiescent_cur_measurement_loop(uint8_t ampl)
{
    // Vadc = I(A) * 1k * 100 * ampl
    // Vadc = I(A) * 100k * ampl
    // I(A) = Vadc / (100k * ampl)
    // I(A) = Val(ADC) * (1.24 / 2048) / (100k * ampl)
    // I(A) = Val(ADC) * 1.24 / (204.8M * ampl)
    // I(nA) = Val(ADC) * 1.24 / (0.2048 * ampl)
    // I(nA) = Val(ADC) * 6,0546875 / ampl
    
    uint16_t cur_val = get_averaged_adc_value(18);
    #ifdef MEAS_PRINTF
        uint16_t debug_val = ((cur_val)*23)/38;
        measdprintf("Quiescent current: %u, approx %u/%unA\r\n", cur_val, debug_val*10, 1 << ampl);
    #endif
}