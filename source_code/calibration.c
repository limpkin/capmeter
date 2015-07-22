/*
 * calibration.c
 *
 * Created: 02/07/2015 12:54:16
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <stdio.h>
#include "eeprom_addresses.h"
#include "measurement.h"
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


/*
 * Get ADC value for single ended offset
 * @return  ADC value
 */
uint16_t get_single_ended_offset(void)
{
    return calib_0v_value_se;
}

/*
 * Measure the offset for single ended conversions
 */
void calibrate_single_ended_offset(void)
{
    calibprintf_P(PSTR("-----------------------\r\n"));
    calibprintf_P(PSTR("Single Ended Offset Calibration...\r\n\r\n"));
    configure_adc_channel(ADC_CHANNEL_GND_EXT, 0, TRUE);
    calib_0v_value_se = get_averaged_stabilized_adc_value(13, 13, TRUE);
    calibprintf("0V ADC value: %u, approx %umV\r\n", calib_0v_value_se, compute_voltage_from_se_adc_val(calib_0v_value_se));    
}

/*
 * Measure the comparison thresholds
 */
void calibrate_thresholds(void)
{
    calibprintf_P(PSTR("-----------------------\r\n"));
    calibprintf_P(PSTR("Threshold calibration\r\n\r\n"));
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    disable_feedback_mos();
    _delay_ms(10);
    enable_bias_voltage(4000);
    
    // Leave time for a possible cap to charge
    _delay_ms(1000);
    uint16_t dac_val = DAC_MIN_VAL;
    setup_opampin_dac(dac_val);
    calib_second_thres_down = 0;
    calib_first_thres_down = 0;
    calib_second_thres_up = 0;
    calib_first_thres_up = 0;
    
    // Ramp up, wait for all the toggles
    calibprintf_P(PSTR("\r\nRamping up...\r\n"));
    while(dac_val != DAC_MAX_VAL)
    {
        update_opampin_dac(++dac_val);
        _delay_us(10);
        // First threshold crossed
        if ((calib_first_thres_down == 0) && ((PORTE_IN & PIN2_bm) == 0))
        {
            calib_first_thres_down = dac_val;
        }
        // Second threshold crossed
        if ((calib_second_thres_down == 0) && ((PORTE_IN & PIN3_bm) != 0))
        {
            calib_second_thres_down = dac_val;
        }
    }
    
    calibprintf("First thres found: %u, approx %umV\r\n", calib_first_thres_down, compute_voltage_from_se_adc_val(calib_first_thres_down));
    calibprintf("Second thres found: %u, approx %umV\r\n\r\n", calib_second_thres_down, compute_voltage_from_se_adc_val(calib_second_thres_down));
        
    // Ramp low, wait for toggle
    calibprintf_P(PSTR("Ramping down...\r\n"));
    while(dac_val != DAC_MIN_VAL)
    {
        update_opampin_dac(--dac_val);
        _delay_us(10);
        // First threshold crossed
        if ((calib_first_thres_up == 0) && ((PORTE_IN & PIN2_bm) != 0))
        {
            calib_first_thres_up = dac_val;
        }
        // Second threshold crossed
        if ((calib_second_thres_up == 0) && ((PORTE_IN & PIN3_bm) == 0))
        {
            calib_second_thres_up = dac_val;
        }
    }
    
    calibprintf("First thres found: %u, approx %umV\r\n", calib_first_thres_up, compute_voltage_from_se_adc_val(calib_first_thres_up));
    calibprintf("Second thres found: %u, approx %umV\r\n\r\n", calib_second_thres_up, compute_voltage_from_se_adc_val(calib_second_thres_up));
    
    disable_opampin_dac();
    disable_bias_voltage();
}

/*
 * Init calibration part
 */
void init_calibration(void)
{
    calibprintf_P(PSTR("-----------------------\r\n"));
    calibprintf_P(PSTR("Calibration Init\r\n"));
    
    // Calibrate single ended offset
    calibrate_single_ended_offset();
    
    // Wait for 0V bias
    wait_for_0v_bias();
    
    // Calibrate thresholds
    //calibrate_thresholds();
}