/*
 * vbias.c
 *
 * Created: 02/07/2015 16:50:00
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <stdio.h>
#include "meas_io.h"
#include "vbias.h"
#include "dac.h"
#include "adc.h"
// Last measured vbias voltage
uint16_t last_measured_vbias;
// Current set bias voltage
uint16_t cur_set_vbias_voltage;
// Current vbias dac_val
uint16_t cur_vbias_dac_val;


/*
 * Get the last measured vbias
 * @return  Vbias
 */
uint16_t get_last_measured_vbias(void)
{
    return last_measured_vbias;
}

/*
 * Compute real vbias value from adc value
 * @param   adc_val     ADC value
 * @return  Actual mV value
 */
uint16_t compute_vbias_for_adc_value(uint16_t adc_val)
{    
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
    /******************* INTERVAL ARITHMETIC *******************/
    // 0.1% resistors
    // Vadc = Vbias * [1198.8,1201.2] / ([1198.8,1201.2]+[14985,15015])
    // Vadc = Vbias * [1198.8,1201.2] / ([16183.8,16216,2])
    // Vbias = Vadc * [16183.8,16216,2] / [1198.8,1201.2]
    // Vbias = Vadc * [16183.8,16216,2] * [1/1201.2, 1/1198.8]
    // Multiplication: [x1,x2]*[y2,y1] = [min(x1y1, x1y2, x2y1, x2y2), max(x1y1, x1y2, x2y1, x2y2)]
    // Vbias = Vadc * [13,473, 13.527]
    // 1.24V 0.25% voltage reference
    // Vbias = VALadc * [1243.1, 1236.9] * [13,473, 13.527] / 4095
    // Vbias = VALadc * [16664.7537, 16815,4137] / 4095
    // Vbias = VALadc * 16740.0837 (+-0.45%) / 4095
    // Taking into account a +-3lsb INL
    // Vbias = VALadc * 16740.0837 (+-0.45%) / 4095 +- 3 * 16740.0837 (+-0.45%) / 4095
    // Vbias = VALadc * 16740.0837 (+-0.45%) / 4095 +- 12mV (+-0.45%)
    
    uint16_t return_value = (adc_val * 16 / 182);
    return_value += (adc_val * 4);
    return return_value;
}

/*
 * To measure set voltages
 */
void bias_voltage_test(void)
{
    // This is the routine to check that we actually can provide a vbias within +-0.5% specifications
    // We are running this scenario in the worst case: 270Ohms resistor, ~17uF cap connected
    // We want to make sure that the oscillations induced to vbias via the capacitor don't
    // break our vbias setting algorithm.
    vbiasprintf_P(PSTR("-----------------------\r\n"));
    vbiasprintf_P(PSTR("Bias Voltage Test\r\n\r\n"));
    
    uint16_t set_voltage, correct_voltage, agg_error = 0;
    set_measurement_mode_io(RES_270);    
    
    for (uint8_t div = ADC_PRESCALER_DIV16_gc; div <= ADC_PRESCALER_DIV512_gc; div++)
    {        
        set_adc_vbias_channel_clock_divider(div);
        enable_bias_voltage(VBIAS_MIN_V);
        for (uint16_t i = VBIAS_MIN_V; i <= 15500; i+= 100)
        {
            set_voltage = update_bias_voltage(i);
            _delay_ms(10);
            correct_voltage = compute_vbias_for_adc_value(get_averaged_adc_value(16));
            
            if (set_voltage > correct_voltage)
            {
                agg_error += set_voltage - correct_voltage;
            }
            else
            {
                agg_error += correct_voltage - set_voltage;
            }
            //printf("Voltage error: %d\r\n", correct_voltage - set_voltage);
        }
        printf_P(PSTR("-----------------------\r\n"));
        printf("Accumulated error for div %u: %d\r\n", 1 << ((uint16_t)div + 2), agg_error);
        printf_P(PSTR("-----------------------\r\n"));
        disable_bias_voltage();
        _delay_ms(50000);
    }
    
    disable_measurement_mode_io();    
}

/*
 * To measure set voltages
 */
void bias_voltage_test2(void)
{
    // This is the routine to check that we actually can provide a vbias within +-0.5% specifications
    // We are running this scenario in the worst case: 270Ohms resistor, ~17uF cap connected
    // We want to make sure that the oscillations induced to vbias via the capacitor don't
    // break our vbias setting algorithm.
    vbiasprintf_P(PSTR("-----------------------\r\n"));
    vbiasprintf_P(PSTR("Bias Voltage Test\r\n\r\n"));
    
    set_measurement_mode_io(RES_270);        
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= 15500; i+= 100)
    {        
        for (uint8_t div = ADC_PRESCALER_DIV16_gc; div <= ADC_PRESCALER_DIV512_gc; div++)
        {
            set_adc_vbias_channel_clock_divider(div);
            update_bias_voltage(i);
            _delay_ms(2000);
            update_bias_voltage(i - 200);
        }
    }
    
    disable_bias_voltage();
    disable_measurement_mode_io();    
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
    vbiasprintf_P(PSTR("-----------------------\r\n"));
    vbiasprintf_P(PSTR("Ramp Voltage Test\r\n\r\n"));
    
    set_measurement_mode_io(RES_10K);        
    enable_bias_voltage(15500);
    _delay_ms(5000);
    disable_bias_voltage();
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= 15500; i+= 100)
    {        
        update_bias_voltage(i);
        _delay_ms(2222);
    }
    
    disable_bias_voltage();
    disable_measurement_mode_io();    
}

/*
 * Wait for 0v bias
 */
void wait_for_0v_bias(void)
{
    uint16_t measured_vbias = 2000;
    
    vbiasprintf_P(PSTR("-----------------------\r\n"));
    vbiasprintf_P(PSTR("Waiting for low bias voltage...\r\n"));
    
    // Wait for bias voltage to be under ~400mV
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
    enable_vbias_quenching();
    while (measured_vbias > 100)
    {
        measured_vbias = get_averaged_stabilized_adc_value(6, 15, FALSE);
    }
    
    disable_vbias_quenching();
    vbiasprintf_P(PSTR("Bias voltage at 0.4V\r\n"));
}

/*
 * Enable bias voltage
 * @param   val     bias voltage in mV
 * @return  Actual mV value set
 */
uint16_t enable_bias_voltage(uint16_t val_mv)
{
    last_measured_vbias = VBIAS_MIN_V;                  // Set min vbias voltage by default
    cur_set_vbias_voltage = VBIAS_MIN_V-1;              // Set min vbias voltage by default
    cur_vbias_dac_val = VBIAS_MIN_DAC_VAL;              // Set min vbias voltage by default
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);  // Enable ADC for vbias monitoring
    setup_vbias_dac(cur_vbias_dac_val);                 // Start with lowest voltage possible
    enable_ldo();                                       // Enable ldo
    _delay_ms(100);                                     // Soft start wait
    return update_bias_voltage(val_mv);                 // Return the actual voltage that was set
}

/*
 * Disable bias voltage
 */
void disable_bias_voltage(void)
{
    vbiasprintf_P(PSTR("Disabling bias voltage\r\n"));  // Debug
    disable_ldo();                                      // Disable LDO
    disable_stepup();                                   // Disable stepup
    disable_vbias_dac();                                // Disable DAC controlling the ldo
    wait_for_0v_bias();                                 // Wait bias voltage to be at 0v
}

/*
 * Update bias voltage
 * @param   val     bias voltage in mV
 * @return  Actual mV value set
 */
uint16_t update_bias_voltage(uint16_t val_mv)
{
    vbiasprintf("Vbias call for %umV\r\n", val_mv);
    uint16_t measured_vbias = last_measured_vbias;
    uint8_t peak_peak = PEAKPEAK_APPROCH;
    uint8_t bit_avg = BIT_AVG_APPROACH;
    uint8_t precise_phase = FALSE;
    
    // Check that the ADC channel remained the same
    if (get_configured_adc_channel() != ADC_CHANNEL_VBIAS)
    {
        configure_adc_channel(ADC_CHANNEL_VBIAS, 0, FALSE);
    }
    
    // Check that value isn't too low...
    if (val_mv < VBIAS_MIN_V)
    {
        vbiasprintf("Value too low, setting it to %dmV!\r\n", VBIAS_MIN_V);
        val_mv = VBIAS_MIN_V;
    }
    
    // Ramp up or ramp low depending on currently set vbias
    if (cur_set_vbias_voltage == val_mv)
    {
        vbiasprintf_P(PSTR("Same val requested!\r\n"));
        return last_measured_vbias;
    }
    else if (cur_set_vbias_voltage > val_mv)
    {        
        // Check if we need to deactivate the stepup
        if ((cur_set_vbias_voltage >= STEPUP_ACTIV_V) && (val_mv < STEPUP_ACTIV_V))
        {
            disable_stepup();
            _delay_ms(1);
        }
        
        // Our voltage decreasing loop
        enable_vbias_quenching();
        do
        {            
            // Adjust peak-peak depending on how close we are
            if (((measured_vbias - val_mv) < MV_APPROCH) && (precise_phase == FALSE))
            {
                peak_peak = PEAKPEAK_FINE;
                bit_avg = BIT_AVG_FINE;
                precise_phase = TRUE;
            }
            
            // Update DAC, wait and get measured vbias
            update_vbias_dac(++cur_vbias_dac_val);
            _delay_us(20);
            measured_vbias = compute_vbias_for_adc_value(get_averaged_stabilized_adc_value(bit_avg, peak_peak, FALSE));
        }
        while ((measured_vbias > val_mv) && (cur_vbias_dac_val != DAC_MAX_VAL));
        disable_vbias_quenching();
        
        // To remove maybe: decrease voltage a bit to compensate for overshoot
        if (val_mv > OVERSHOOT_COR_V)
        {
            update_vbias_dac(++cur_vbias_dac_val);
            measured_vbias = compute_vbias_for_adc_value(get_averaged_stabilized_adc_value(bit_avg, peak_peak, FALSE));
        }
    } 
    else
    {        
        // Check if we need to activate the stepup
        if ((cur_set_vbias_voltage < STEPUP_ACTIV_V) && (val_mv >= STEPUP_ACTIV_V))
        {            
            enable_stepup();                                    // Enable stepup
            _delay_ms(10);                                      // Step up start takes around 1.5ms (oscilloscope)
        }
        
        // Our voltage increasing loop (takes 40ms to reach vmax)
        do
        {
            // Adjust peak-peak & averaging depending on how close we are
            if (((val_mv - measured_vbias) < MV_APPROCH) && (precise_phase == FALSE))
            {
                peak_peak = PEAKPEAK_FINE;
                bit_avg = BIT_AVG_FINE;
                precise_phase = TRUE;
            }
            
            // Update DAC, wait and get measured vbias
            update_vbias_dac(--cur_vbias_dac_val);
            _delay_us(20);
            measured_vbias = compute_vbias_for_adc_value(get_averaged_stabilized_adc_value(bit_avg, peak_peak, FALSE));
        }
        while ((measured_vbias < val_mv) && (cur_vbias_dac_val != 0));
        
        // To remove maybe: increase voltage a bit to compensate for overshoot
        if (val_mv > OVERSHOOT_COR_V)
        {
            update_vbias_dac(--cur_vbias_dac_val);
            measured_vbias = compute_vbias_for_adc_value(get_averaged_stabilized_adc_value(bit_avg, peak_peak, FALSE));
        }        
    }  
    
    // Wait before continuing
    _delay_ms(10);     
    cur_set_vbias_voltage = val_mv;      
    last_measured_vbias = measured_vbias;                           
    vbiasprintf("Vbias set, actual value: %umV\r\n", measured_vbias);
    return measured_vbias;
}