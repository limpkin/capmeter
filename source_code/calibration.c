/*
 * calibration.c
 *
 * Created: 02/07/2015 12:54:16
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include "conversions.h"
#include "calibration.h"
#include "meas_io.h"
#include "vbias.h"
#include "dac.h"
#include "adc.h"
/////////////////////////////////////
//
// After many different experiments,
// we found that the offset for differential
// measurements didn't need to be calibrated.
// We are therefore using the default one
// which allows us to be +-0.5% accurate for
// current measurements when ampl is 1X
//
/////////////////////////////////////
// Calibrated second threshold, ramping up
uint16_t calib_second_thres_down;
// Calibrated first threshold, ramping up
uint16_t calib_first_thres_down;
// Calibrated second threshold, ramping down
uint16_t calib_second_thres_up;
// Calibrated first threshold, ramping down
uint16_t calib_first_thres_up;
// Single ended measurement offset
uint16_t calib_0v_value_se = 0;
// Maximum voltage that can be generated
uint16_t max_voltage = 0;


/*
 * Get calibrated second threshold, ramping up
 * @return  DAC value
 */
uint16_t get_calib_second_thres_down(void)
{
    return calib_second_thres_down;
}

/*
 * Get calibrated first threshold, ramping up
 * @return  DAC value
 */
uint16_t get_calib_first_thres_down(void)
{
    return calib_first_thres_down;
}

/*
 * Get calibrated second threshold, ramping down
 * @return  DAC value
 */
uint16_t get_calib_second_thres_up(void)
{
    return calib_second_thres_up;
}

/*
 * Get calibrated first threshold, ramping down
 * @return  DAC value
 */
uint16_t get_calib_first_thres_up(void)
{
    return calib_first_thres_up;
}

/*
 * Get ADC value for single ended offset
 * @return  ADC value
 */
uint16_t get_single_ended_offset(void)
{
    return calib_0v_value_se;
}

/*
 * Delete single ended offset
 */
void delete_single_ended_offset(void)
{
    calib_0v_value_se = 0;
}

/*
 * Get the max voltage we can generate
 * @return  the voltage
 */
uint16_t get_max_vbias_voltage(void)
{
    return max_voltage;
}

/*
 * Measure the offset for single ended conversions
 */
void calibrate_single_ended_offset(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Single Ended Offset Calibration...\r\n\r\n"));
    configure_adc_channel(ADC_CHANNEL_GND_EXT, 0, TRUE);
    calib_0v_value_se = 0;
    calib_0v_value_se = get_averaged_adc_value(16);
    calibdprintf("0V ADC value: %u, approx %umV\r\n", calib_0v_value_se, compute_voltage_from_se_adc_val(calib_0v_value_se));    
}

/*
 * Measure the comparison thresholds
 */
void calibrate_thresholds(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Threshold calibration\r\n\r\n"));
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    disable_feedback_mos();
    _delay_ms(10);
    enable_bias_voltage(4000);
    
    // Leave time for a possible cap to charge
    _delay_ms(1000);
    uint16_t dac_val = DAC_MIN_VAL;
    setup_opampin_dac(dac_val);
    uint32_t calib_second_thres_down_agg = 0;
    uint32_t calib_first_thres_down_agg = 0;
    uint32_t calib_second_thres_up_agg = 0;
    uint32_t calib_first_thres_up_agg = 0;
    uint8_t temp_bool = TRUE;
    
    for (uint16_t i = 0; i < (1 << THRESHOLD_AVG_BIT_SHIFT); i++)
    {
        // Ramp up, wait for all the toggles
        temp_bool = TRUE;
        while(dac_val != DAC_MAX_VAL && temp_bool == TRUE)
        {
            update_opampin_dac(++dac_val);
            _delay_us(10);
            // First threshold crossed
            if ((calib_first_thres_down == 0) && ((PORTE_IN & PIN2_bm) == 0))
            {
                calib_first_thres_down = dac_val;
                calib_first_thres_down_agg += calib_first_thres_down;
            }
            // Second threshold crossed
            if ((calib_second_thres_down == 0) && ((PORTE_IN & PIN3_bm) != 0))
            {
                temp_bool = FALSE;
                calib_second_thres_down = dac_val;
                calib_second_thres_down_agg += calib_second_thres_down;
            }
        }
        
        // Get a little margin before ramping down
        if (dac_val < DAC_MAX_VAL - 100)
        {
            dac_val += 100;
        } 
        else
        {
            dac_val = DAC_MAX_VAL;
        }    
        update_opampin_dac(dac_val);
        _delay_us(20);    
        
        // Ramp low, wait for toggle
        temp_bool = TRUE;
        while(dac_val != DAC_MIN_VAL && temp_bool == TRUE)
        {
            update_opampin_dac(--dac_val);
            _delay_us(10);
            // First threshold crossed
            if ((calib_first_thres_up == 0) && ((PORTE_IN & PIN2_bm) != 0))
            {
                temp_bool = FALSE;
                calib_first_thres_up = dac_val;
                calib_first_thres_up_agg += calib_first_thres_up;
            }
            // Second threshold crossed
            if ((calib_second_thres_up == 0) && ((PORTE_IN & PIN3_bm) == 0))
            {
                calib_second_thres_up = dac_val;
                calib_second_thres_up_agg += calib_second_thres_up;
            }
        }
        
        // Get a little margin before ramping up
        if (dac_val > 100)
        {
            dac_val -= 100;
        } 
        else
        {
            dac_val = DAC_MIN_VAL;
        }    
        update_opampin_dac(dac_val);
        _delay_us(20);   
        
        // Reset values
        calib_first_thres_down = 0;
        calib_second_thres_down = 0;
        calib_first_thres_up = 0;
        calib_second_thres_up = 0;
    }    
    
    // Compute values
    
    calib_first_thres_down = (uint16_t)(calib_first_thres_down_agg >> THRESHOLD_AVG_BIT_SHIFT);
    calib_second_thres_down = (uint16_t)(calib_second_thres_down_agg >> THRESHOLD_AVG_BIT_SHIFT);
    calib_first_thres_up = (uint16_t)(calib_first_thres_up_agg >> THRESHOLD_AVG_BIT_SHIFT);
    calib_second_thres_up = (uint16_t)(calib_second_thres_up_agg >> THRESHOLD_AVG_BIT_SHIFT);
    calibdprintf("First thres found: %u, approx %umV\r\n", calib_first_thres_down, compute_voltage_from_se_adc_val(calib_first_thres_down));
    calibdprintf("Second thres found: %u, approx %umV\r\n", calib_second_thres_down, compute_voltage_from_se_adc_val(calib_second_thres_down));
    calibdprintf("First thres found: %u, approx %umV\r\n", calib_first_thres_up, compute_voltage_from_se_adc_val(calib_first_thres_up));
    calibdprintf("Second thres found: %u, approx %umV\r\n", calib_second_thres_up, compute_voltage_from_se_adc_val(calib_second_thres_up));
    
    disable_opampin_dac();
    disable_bias_voltage();
}

/*
 * Init calibration part
 */
void init_calibration(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Calibration Init\r\n"));
    
    // Calibrate single ended offset
    calibrate_single_ended_offset();
    
    // Wait for 0V bias
    wait_for_0v4_bias();
    
    // Calibrate thresholds
    calibrate_thresholds();
    
    // Find max voltage
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Max Voltage Measure\r\n\r\n"));
    max_voltage = enable_bias_voltage(33333);
    calibdprintf("Max voltage found: %dmV\r\n", max_voltage);
    disable_bias_voltage();
}