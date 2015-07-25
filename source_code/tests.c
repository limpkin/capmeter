/*
 * tests.c
 *
 * Created: 24/07/2015 18:11:32
 *  Author: limpkin
 */ 
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include "conversions.h"
#include "calibration.h"
#include "measurement.h"
#include "meas_io.h"
#include "vbias.h"
#include "tests.h"
#include "adc.h"

/*
 * To measure set voltages
 */
void bias_voltage_test(void)
{
    // This is the routine to check that we actually can provide a vbias within +-0.5% specifications
    // We are running this scenario in the worst case: 270Ohms resistor, ~17uF cap connected
    // We want to make sure that the oscillations induced to vbias via the capacitor don't
    // break our vbias setting algorithm.
    testdprintf_P(PSTR("-----------------------\r\n"));
    testdprintf_P(PSTR("Bias Voltage Test\r\n\r\n"));
    
    uint16_t set_voltage, correct_voltage, agg_error = 0;
    
    for (uint8_t div = ADC_PRESCALER_DIV16_gc; div <= ADC_PRESCALER_DIV512_gc; div++)
    {        
        //set_adc_vbias_channel_clock_divider(div);
        enable_bias_voltage(VBIAS_MIN_V);
        for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
        {
            set_voltage = update_bias_voltage(i);
            _delay_ms(1500);
            correct_voltage = compute_vbias_for_adc_value(get_averaged_adc_value(18));    
            if (i > correct_voltage)
            {
                agg_error += i - correct_voltage;
            }
            else
            {
                agg_error += correct_voltage - i;
            }
            testdprintf("Call: %u, Set: %u, Measured: %u, Error Set: %d, Error Measured: %d\r\n", i, set_voltage, correct_voltage, correct_voltage - set_voltage, correct_voltage - i);
        }
        testdprintf_P(PSTR("-----------------------\r\n"));
        testdprintf("Accumulated error for div %u: %d\r\n", 1 << ((uint16_t)div + 2), agg_error);
        testdprintf_P(PSTR("-----------------------\r\n"));
        disable_bias_voltage();
        _delay_ms(50000);
    } 
}

/*
 * To measure set voltages
 */
void ramp_bias_voltage_test(void)
{
    // This is the routine to check that we actually can provide a vbias within +-0.5% specifications
    // We are running this scenario in the worst case: 270Ohms resistor, ~17uF cap connected
    // We want to make sure that the oscillations induced to vbias via the capacitor don't
    // break our vbias setting algorithm.
    testdprintf_P(PSTR("-----------------------\r\n"));
    testdprintf_P(PSTR("Ramp Voltage Test\r\n\r\n"));
    
    //set_measurement_mode_io(RES_10K);        
    enable_bias_voltage(15000);
    _delay_ms(5000);
    disable_bias_voltage();
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
    {        
        update_bias_voltage(i);
        _delay_ms(2222);
    }
    
    disable_bias_voltage();
    disable_measurement_mode_io();    
}

void peak_to_peak_adc_noise_measurement_test(void)
{
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
    {
        update_bias_voltage(i);
        _delay_ms(100);
        measure_peak_to_peak_on_channel(BIT_AVG_FINE, ADC_CHANNEL_VBIAS, 0);
    }
    
    disable_bias_voltage();
}

/*
 * Ramp voltage and measure the current
 */
void ramp_current_test(void)
{
    uint16_t cur_measure;
    
    testdprintf_P(PSTR("-----------------------\r\n"));
    testdprintf_P(PSTR("Ramp Current Test\r\n\r\n"));
    
    set_current_measurement_mode(CUR_MES_1X);
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
    {
        update_bias_voltage(i);
        _delay_ms(10);
        cur_measure = cur_measurement_loop(16);
        print_compute_cur_formula(cur_measure);
        if (cur_measure >= 2047)
        {
            break;
        }        
    }
    
    disable_bias_voltage();
    disable_current_measurement_mode();
}