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
 #include "measurement.h"
 #include "meas_io.h"
 #include "main.h"
 #include "dac.h"
 // To indicate the number of timer overflows
 volatile uint8_t nb_overflows;
 // Last counter value
 volatile uint16_t last_counter_val;
 // Counter values
 volatile uint16_t last_measured_value;
 // Chosen resistor for measure
 uint8_t chosen_res = RES_100K;
 // New measurement value
 volatile uint8_t new_val_flag;
 // Calibrated vup
 uint16_t calib_vup;
 // Calibrated vlow
 uint16_t calib_vlow;

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
 * Enable bias voltage
 * @param   val     DAC value
 */
void enable_bias_voltage(uint16_t val)
{
    uint16_t temp_val = VBIAS_MIN_DAC_VAL;  // For our loop
    enable_stepup();                        // Enable stepup
    setup_vbias_dac(temp_val);              // Start with lowest voltage possible
    _delay_ms(10);                          // Step up start takes around 1.5ms (oscilloscope)
    enable_ldo();                           // Enable ldo
    _delay_ms(10);                          // Soft start on the LDO takes around 1ms (oscilloscope)
    while(temp_val != val)                  // Our voltage increasing loop (takes 40ms to reach vmax)
    {
        update_vbias_dac(temp_val--);
        _delay_us(10);
    }
    _delay_ms(10);                          // Wait before continuing
    measdprintf("DAC Val for Vbias set to %d\r\n", val);
}

/*
 * Disable bias voltage
 */
void disable_bias_voltage(void)
{
    disable_ldo();              // Disable LDO
    disable_stepup();           // Disable stepup
    disable_vbias_dac();        // Disable DAC controlling the ldo
    _delay_ms(2500);            // It takes around 1.5 sec for vbias to decrease!
}

/*
 * Calibrate vup/vlow
 */
void calibrate_vup_vlow(void)
{    
    measdprintf_P(PSTR("Vup/Vlow calibration\r\n"));
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    disable_feedback_mos();
    _delay_ms(10);
    enable_bias_voltage(DAC_MIN_VAL/2);
    
    // Leave time for a possible cap to charge
    _delay_ms(1000);
    setup_opampin_dac(DAC_MIN_VAL);
    calib_vlow = DAC_MAX_VAL;
    calib_vup = DAC_MIN_VAL;
    
    // Ramp up, wait for toggle
    while((PORTA_IN & PIN6_bm) != 0)
    {
        update_opampin_dac(calib_vup++);
        _delay_us(10);
    }
    
    measdprintf("Vhigh found: %d\r\n", calib_vup);
    update_opampin_dac(calib_vlow);
    _delay_us(10);
        
    // Ramp low, wait for toggle
    while((PORTA_IN & PIN6_bm) == 0)
    {
        update_opampin_dac(calib_vlow--);
        _delay_us(10);
    }
    
    measdprintf("Vlow found: %d\r\n", calib_vlow);
    disable_opampin_dac();
    disable_bias_voltage();
}

/*
 * Our main measurement loop
 * @param   mes_mode     Our measurement mode (see enum_mes_mode_t)
 */
void measurement_loop(uint8_t mes_mode)
{
    // If we received a new measurement
    if (new_val_flag == TRUE)
    {
        new_val_flag = FALSE;
        printf("%d ", last_measured_value);
    }
}