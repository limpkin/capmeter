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
#include "conversions.h"
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
 * Get the current vbias dac value
 * @return  The dac value
 */
uint16_t get_current_vbias_dac_value(void)
{
    return cur_vbias_dac_val;
}

/*
 * Wait for 0v4 bias
 */
void wait_for_0v4_bias(void)
{
    uint16_t measured_vbias = 2000;
    
    vbiasdprintf_P(PSTR("-----------------------\r\n"));
    vbiasdprintf_P(PSTR("Waiting for low bias voltage...\r\n"));
    
    // Wait for bias voltage to be under ~400mV
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
    enable_vbias_quenching();
    while (measured_vbias > 60)
    {
        measured_vbias = get_averaged_adc_value(8);
    }
    
    vbiasdprintf_P(PSTR("Bias voltage at 0.4V\r\n"));
    vbiasdprintf_P(PSTR("-----------------------\r\n"));
}

/*
 * Wait for 0v bias
 */
void wait_for_0v_bias(void)
{
    uint16_t measured_vbias = 2000;
    
    vbiasdprintf_P(PSTR("-----------------------\r\n"));
    vbiasdprintf_P(PSTR("Waiting for low bias voltage...\r\n"));
    
    // Wait for bias voltage to be at 0V exactly
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
    enable_vbias_quenching();
    while (measured_vbias != 0)
    {
        measured_vbias = get_averaged_adc_value(6);
    }
    
    vbiasdprintf_P(PSTR("Bias voltage at 0V\r\n"));
    vbiasdprintf_P(PSTR("-----------------------\r\n"));
}

/*
 * Enable bias voltage
 * @param   val     bias voltage in mV
 * @return  Actual mV value set
 */
uint16_t enable_bias_voltage(uint16_t val_mv)
{
    // Check if the bias voltage isn't already enabled
    if (is_ldo_enabled() == FALSE)
    {
        disable_vbias_quenching();                          // Disable quenching
        last_measured_vbias = VBIAS_MIN_V-1;                // Set min vbias voltage by default
        cur_set_vbias_voltage = VBIAS_MIN_V-1;              // Set min vbias voltage by default
        cur_vbias_dac_val = VBIAS_MIN_DAC_VAL;              // Set min vbias voltage by default
        configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);  // Enable ADC for vbias monitoring
        setup_vbias_dac(cur_vbias_dac_val);                 // Start with lowest voltage possible
        enable_ldo();                                       // Enable ldo
        _delay_ms(100);                                     // Soft start wait
    }
    
    return update_bias_voltage(val_mv);                     // Return the actual voltage that was set
}

/*
 * Disable bias voltage
 */
void disable_bias_voltage(void)
{
    vbiasdprintf_P(PSTR("Disabling bias voltage\r\n")); // Debug
    disable_ldo();                                      // Disable LDO
    disable_stepup();                                   // Disable stepup
    disable_vbias_dac();                                // Disable DAC controlling the ldo
    wait_for_0v4_bias();                                // Wait bias voltage to be at 0v
}

/*
 * Update bias voltage
 * @param   val     bias voltage in mV
 * @return  Actual mV value set
 */
uint16_t update_bias_voltage(uint16_t val_mv)
{
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
    
    vbiasdprintf("Vbias call for %umV\r\n", val_mv);
    uint16_t measured_vbias = last_measured_vbias;
    uint8_t precise_phase = FALSE;
    
    // Check that the ADC channel remained the same
    if (get_configured_adc_channel() != ADC_CHANNEL_VBIAS)
    {
        configure_adc_channel(ADC_CHANNEL_VBIAS, 0, FALSE);
    }
    
    // Check that value isn't too low...
    if (val_mv < VBIAS_MIN_V)
    {
        vbiasdprintf("Value too low, setting it to %dmV!\r\n", VBIAS_MIN_V);
        val_mv = VBIAS_MIN_V;
    }
    
    // Same voltage requested, return last measured vbias
    if (cur_set_vbias_voltage == val_mv)
    {
        vbiasdprintf_P(PSTR("Same val requested!\r\n"));
        return last_measured_vbias;
    }
    
    // Voltage lower than the previous one requested, lower voltage then ramp up again
    if (cur_set_vbias_voltage > val_mv)
    {        
        // Check if we need to deactivate the stepup
        if ((cur_set_vbias_voltage >= STEPUP_ACTIV_V) && (val_mv < STEPUP_ACTIV_V))
        {
            disable_stepup();
            _delay_ms(1);
        }
        
        // Compute voltage to reach
        uint16_t voltage_to_reach;
        if (val_mv > VBIAS_MIN_V + 4*MV_APPROCH)
        {
            voltage_to_reach = val_mv - 4*MV_APPROCH;
        } 
        else
        {
            voltage_to_reach = VBIAS_MIN_V;
        }        
        
        // Our voltage decreasing loop
        enable_vbias_quenching();
        do
        {                        
            // Update DAC, wait and get measured vbias
            cur_vbias_dac_val += 20;
            if (cur_vbias_dac_val > DAC_MAX_VAL)
            {
                cur_vbias_dac_val = DAC_MAX_VAL;
            }            
            update_vbias_dac(cur_vbias_dac_val);
            _delay_us(20);
            measured_vbias = compute_vbias_for_adc_value(get_averaged_stabilized_adc_value(BIT_AVG_APPROACH + 1, 4, FALSE));
        }
        while ((measured_vbias > voltage_to_reach) && (cur_vbias_dac_val != DAC_MAX_VAL));
        disable_vbias_quenching();
    } 
       
    // Check if we need to activate the stepup
    if ((cur_set_vbias_voltage < STEPUP_ACTIV_V) && (val_mv >= STEPUP_ACTIV_V))
    {            
        enable_stepup();                                    // Enable stepup
        _delay_ms(10);                                      // Step up start takes around 1.5ms (oscilloscope)
    }
        
    // Our voltage increasing loop (takes 400ms to reach vmax)
    do
    {
        // Adjust peak-peak & averaging depending on how close we are
        if (((val_mv - measured_vbias) < MV_APPROCH) && (precise_phase == FALSE))
        {
            precise_phase = TRUE;
            //enable_cur_meas_mos();
        }
            
        // Update DAC, wait and get measured vbias
        update_vbias_dac(--cur_vbias_dac_val);
        _delay_us(10);
        if (precise_phase == FALSE)
        {
            measured_vbias = compute_vbias_for_adc_value(get_averaged_adc_value(BIT_AVG_APPROACH));
        } 
        else
        {
            _delay_ms(CONV_DELAY_FINE);
            measured_vbias = compute_vbias_for_adc_value(get_averaged_adc_value(BIT_AVG_FINE));
        }            
    }
    while ((measured_vbias < val_mv - VBIAS_OVERSHOOT_MV) && (cur_vbias_dac_val != 0));       
    
    // Wait before continuing
    _delay_ms(10);
    cur_set_vbias_voltage = val_mv;      
    last_measured_vbias = measured_vbias + VBIAS_OVERSHOOT_MV;                           
    vbiasdprintf("Vbias set, actual value: %umV\r\n", measured_vbias + VBIAS_OVERSHOOT_MV);
    return measured_vbias + VBIAS_OVERSHOOT_MV;
}

/*
 * Change Vbias dac value
 * @param   dac_value   DAC value to set
 * @param   wait_ms     How many ms to wait before measuring vbias
 * @return  Actual bias voltage in mV
 * @note    Due to the vbias capacitance, it is much better to ramp up. Don't forget about the overshoot as well.
 */
uint16_t force_vbias_dac_change(uint16_t dac_value, uint16_t wait_ms)
{
    vbiasdprintf("Vbias forced dac change for %u\r\n", dac_value);
    
    // Check that the ADC channel remained the same
    if (get_configured_adc_channel() != ADC_CHANNEL_VBIAS)
    {
        configure_adc_channel(ADC_CHANNEL_VBIAS, 0, FALSE);
    }
    
    // Check if we need to enable/disable the stepup
    if (dac_value < STEPUP_ACTIV_DAC_V && is_stepup_enabled() == FALSE)
    {
        enable_stepup();                                    // Enable stepup
        _delay_ms(10);                                      // Step up start takes around 1.5ms (oscilloscope)
    }
    else if (dac_value > STEPUP_ACTIV_DAC_V && is_stepup_enabled() == TRUE)
    {
        disable_stepup();
        _delay_ms(1);
    }
    
    // Update vbias dac
    update_vbias_dac(dac_value);
    
    // Wait for the given amount of ms
    for (uint16_t i = 0; i < wait_ms; i++)
    {
        _delay_ms(1);
    }
    
    // Store vbias and dac value, return vbias
    last_measured_vbias = compute_vbias_for_adc_value(get_averaged_adc_value(BIT_AVG_FINE));
    cur_set_vbias_voltage = last_measured_vbias;
    cur_vbias_dac_val = dac_value;
    
    return last_measured_vbias;    
}