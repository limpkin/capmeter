/*
 * calibration.c
 *
 * Created: 02/07/2015 12:54:16
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include "eeprom_addresses.h"
#include "measurement.h"
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
// Open ended calibration data
oe_calib_data_t oe_calib_data;
// Know if platform is calibrated
uint8_t calib_ok = FALSE;


/*
 * Know if the platform is calibrated
 * @return  Boolean
 */
uint8_t is_platform_calibrated(void)
{
    return calib_ok;
}

/*
 * Get calibrated oscillator low value
 * @return  DAC value
 */
uint16_t get_calib_osc_low_v(void)
{
    return oe_calib_data.calib_osc_low_v;
}

/*
 * Get calibrated second threshold, ramping up
 * @return  DAC value
 */
uint16_t get_calib_second_thres_down(void)
{
    return oe_calib_data.calib_second_thres_down;
}

/*
 * Get calibrated first threshold, ramping up
 * @return  DAC value
 */
uint16_t get_calib_first_thres_down(void)
{
    return oe_calib_data.calib_first_thres_down;
}

/*
 * Get calibrated second threshold, ramping down
 * @return  DAC value
 */
uint16_t get_calib_second_thres_up(void)
{
    return oe_calib_data.calib_second_thres_up;
}

/*
 * Get calibrated first threshold, ramping down
 * @return  DAC value
 */
uint16_t get_calib_first_thres_up(void)
{
    return oe_calib_data.calib_first_thres_up;
}

/*
 * Get ADC value for single ended offset
 * @param   current_channel     The channel
 * @return  ADC value
 */
uint16_t get_single_ended_offset(uint8_t current_channel)
{
    if ((current_channel == ADC_CHANNEL_VBIAS) && (oe_calib_data.calib_0v_value_vbias != 0))
    {
        return oe_calib_data.calib_0v_value_vbias;
    } 
    else
    {
        return oe_calib_data.calib_0v_value_se;
    }    
}

/*
 * Get ADC value for current measurement offset
 * @param   ampl    The amplification used
 * @return  ADC value
 */
uint16_t get_offset_for_current_measurement(uint8_t ampl)
{
    return oe_calib_data.cur_measurement_offsets[ampl];
}

/*
 * Delete current measurement offsets
 */
void delete_cur_measurement_offsets(void)
{
    memset((void*)oe_calib_data.cur_measurement_offsets, 0x00, sizeof(oe_calib_data.cur_measurement_offsets));
}

/*
 * Delete single ended offset
 */
void delete_single_ended_offset(void)
{
    oe_calib_data.calib_0v_value_se = 0;
}

/*
 * Get the max voltage we can generate
 * @return  the voltage
 */
uint16_t get_max_vbias_voltage(void)
{
    return oe_calib_data.max_voltage;
}

/*
 * Get the complete open ended calibration data
 * @return  the number of bytes of this data
 */
uint8_t get_openended_calibration_data(uint8_t* buffer)
{
    memcpy((void*)buffer, (void*)&oe_calib_data, sizeof(oe_calib_data));
    return sizeof(oe_calib_data);
}

/*
 * Measure the offset for vbias
 */
void calibrate_single_ended_offset_for_vbias(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Single Ended Offset Calibration For Vbias...\r\n\r\n"));
    
    // Configure correct ADC channel, enable vbias quenching
    disable_bias_voltage();
    enable_vbias_quenching();
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
    
    // If we're not near the dedicated 0v adc value, add a delay
    if (get_averaged_adc_value(4) > 8)
    {
         while(get_averaged_adc_value(4) > 8);
        _delay_ms(3210);
    }        
    _delay_ms(500);
    
    // Delete current single ended offset, get vbias offset
    uint16_t calib_0v_value_se_copy = oe_calib_data.calib_0v_value_se;
    oe_calib_data.calib_0v_value_se = 0;
    oe_calib_data.calib_0v_value_vbias = 0;
    oe_calib_data.calib_0v_value_vbias = get_averaged_adc_value(15);
    
    // Restore current single ended offset
    oe_calib_data.calib_0v_value_se = calib_0v_value_se_copy;
    calibdprintf("0V ADC value for vbias: %u, approx %umV\r\n", oe_calib_data.calib_0v_value_vbias, compute_voltage_from_se_adc_val(oe_calib_data.calib_0v_value_vbias));    
}

/*
 * Measure the offset for single ended conversions
 */
void calibrate_single_ended_offset(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Single Ended Offset Calibration...\r\n\r\n"));
    configure_adc_channel(ADC_CHANNEL_GND_EXT, 0, TRUE);
    _delay_ms(10);
    oe_calib_data.calib_0v_value_se = 0;
    oe_calib_data.calib_0v_value_se = get_averaged_adc_value(15);
    calibdprintf("0V ADC value: %u, approx %umV\r\n", oe_calib_data.calib_0v_value_se, compute_voltage_from_se_adc_val(oe_calib_data.calib_0v_value_se));    
}

/*
 * Measure the offset for current measurements
 */
void calibrate_cur_measurement_offsets(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Current Measurement Offset Calibration...\r\n\r\n"));
    
    enable_feedback_mos();                  // Enable feedback mos to allow current passage   
    enable_cur_meas_mos();                  // Enable current measurement mosfet (or not?)
    delete_cur_measurement_offsets();       // Delete offsets
    
    for (uint8_t i = CUR_MES_1X; i < CUR_MES_4X; i++)
    {
        configure_adc_channel(ADC_CHANNEL_CUR, i, TRUE);
        oe_calib_data.cur_measurement_offsets[i] = get_averaged_adc_value(15);
        calibdprintf("Offset for ampl %u: %u\r\n", 1 << i, oe_calib_data.cur_measurement_offsets[i]);
    }
    
    disable_cur_meas_mos();                 // Disable current measurement technique
    disable_feedback_mos();                 // Disable feedback mos
}

/*
 * Measure the comparison thresholds
 */
void calibrate_thresholds(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Threshold Calibration\r\n\r\n"));
    
    // Init IOs
    PORTE.DIRCLR = PIN0_bm | PIN1_bm;       // May have been modified by other functions
    EVSYS.CH3MUX = 0x00;                    // May have been modified by other functions
    EVSYS.CH4MUX = 0x00;                    // May have been modified by other functions
    PORTE.PIN0CTRL = PORT_ISC_BOTHEDGES_gc; // May have been modified by other functions
    PORTE.PIN1CTRL = PORT_ISC_BOTHEDGES_gc; // May have been modified by other functions
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    disable_feedback_mos();
    opampin_as_input();
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
        // Reset values
        oe_calib_data.calib_first_thres_up = 0;
        oe_calib_data.calib_second_thres_up = 0;
        oe_calib_data.calib_first_thres_down = 0;
        oe_calib_data.calib_second_thres_down = 0;
        
        // Ramp up, wait for all the toggles
        temp_bool = TRUE;
        while(dac_val != DAC_MAX_VAL && temp_bool == TRUE)
        {
            update_opampin_dac(++dac_val);
            _delay_us(10);
            // First threshold crossed
            if ((oe_calib_data.calib_first_thres_down == 0) && ((PORTE_IN & PIN0_bm) == 0))
            {
                if (oe_calib_data.calib_second_thres_down != 0)
                {
                    temp_bool = FALSE;
                }                
                oe_calib_data.calib_first_thres_down = dac_val;
                calib_first_thres_down_agg += oe_calib_data.calib_first_thres_down;
            }
            // Second threshold crossed
            if ((oe_calib_data.calib_second_thres_down == 0) && ((PORTE_IN & PIN1_bm) != 0))
            {
                if (oe_calib_data.calib_first_thres_down != 0)
                {
                    temp_bool = FALSE;
                }
                oe_calib_data.calib_second_thres_down = dac_val;
                calib_second_thres_down_agg += oe_calib_data.calib_second_thres_down;
            }
        }
        
        // Get a little margin before ramping down
        if (dac_val < DAC_MAX_VAL - 200)
        {
            dac_val += 200;
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
            if ((oe_calib_data.calib_first_thres_up == 0) && ((PORTE_IN & PIN0_bm) != 0))
            {
                if (oe_calib_data.calib_second_thres_up != 0)
                {
                    temp_bool = FALSE;
                }
                oe_calib_data.calib_first_thres_up = dac_val;
                calib_first_thres_up_agg += oe_calib_data.calib_first_thres_up;
            }
            // Second threshold crossed
            if ((oe_calib_data.calib_second_thres_up == 0) && ((PORTE_IN & PIN1_bm) == 0))
            {
                if (oe_calib_data.calib_first_thres_up != 0)
                {
                    temp_bool = FALSE;
                }
                oe_calib_data.calib_second_thres_up = dac_val;
                calib_second_thres_up_agg += oe_calib_data.calib_second_thres_up;
            }
        }
        
        // Get a little margin before ramping up
        if (dac_val > 200)
        {
            dac_val -= 200;
        } 
        else
        {
            dac_val = DAC_MIN_VAL;
        }    
        update_opampin_dac(dac_val);
        _delay_us(20);   
    }    
    
    // Compute values    
    oe_calib_data.calib_first_thres_down = (uint16_t)(calib_first_thres_down_agg >> THRESHOLD_AVG_BIT_SHIFT);
    oe_calib_data.calib_second_thres_down = (uint16_t)(calib_second_thres_down_agg >> THRESHOLD_AVG_BIT_SHIFT);
    oe_calib_data.calib_first_thres_up = (uint16_t)(calib_first_thres_up_agg >> THRESHOLD_AVG_BIT_SHIFT);
    oe_calib_data.calib_second_thres_up = (uint16_t)(calib_second_thres_up_agg >> THRESHOLD_AVG_BIT_SHIFT);
    calibdprintf("First thres found: %u, approx %umV\r\n", oe_calib_data.calib_first_thres_down, compute_voltage_from_se_adc_val(oe_calib_data.calib_first_thres_down));
    calibdprintf("Second thres found: %u, approx %umV\r\n", oe_calib_data.calib_second_thres_down, compute_voltage_from_se_adc_val(oe_calib_data.calib_second_thres_down));
    calibdprintf("First thres found: %u, approx %umV\r\n", oe_calib_data.calib_first_thres_up, compute_voltage_from_se_adc_val(oe_calib_data.calib_first_thres_up));
    calibdprintf("Second thres found: %u, approx %umV\r\n", oe_calib_data.calib_second_thres_up, compute_voltage_from_se_adc_val(oe_calib_data.calib_second_thres_up));
    
    disable_opampin_dac();
    disable_bias_voltage();
    set_opampin_low();
}

/*
 * Measure the oscillator low voltage
 */
void calibrate_osc_low_v(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Oscillator Low Voltage Calibration\r\n\r\n"));
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    set_opampin_high();
    disable_feedback_mos();
    enable_bias_voltage(4000);
    
    // Leave time for a possible cap to charge
    _delay_ms(1000);
    uint16_t dac_val = DAC_MAX_VAL;
    opampin_as_input();
    setup_opampin_dac(dac_val);
    
    // Wait for compout toggle
    while (((PORTA_IN & PIN6_bm) == 0) && (dac_val != 0))
    {
        update_opampin_dac(--dac_val);
        _delay_us(20);
    }
    
    // Store result
    oe_calib_data.calib_osc_low_v = dac_val;
    calibdprintf("Osc Low Voltage found: %u\r\n", oe_calib_data.calib_osc_low_v);
    
    disable_opampin_dac();
    disable_bias_voltage();
    set_opampin_low();
}  

/*
 * Start open terminal calibration
 */  
void start_openended_calibration(uint8_t day, uint8_t month, uint8_t year)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Calibration Start\r\n\r\n"));
    
    // Store date
    oe_calib_data.day = day;
    oe_calib_data.month = month;
    oe_calib_data.year = year;
    
    // Calibrate single ended offset
    calibrate_single_ended_offset();
    
    // Calibrate single ended offset for vbias
    calibrate_single_ended_offset_for_vbias();
    
    // Calibrate current measurement offsets
    calibrate_cur_measurement_offsets();
    
    // Wait for 0V bias
    wait_for_0v4_bias();
    
    // Calibrate thresholds
    calibrate_thresholds();
    
    // Calibrate low oscillator value
    calibrate_osc_low_v();
    
    // Find max voltage
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Max Voltage Measure\r\n\r\n"));
    oe_calib_data.max_voltage = enable_bias_voltage(33333);
    calibdprintf("Max voltage found: %dmV\r\n", oe_calib_data.max_voltage);
    disable_bias_voltage();   
    
    // Store calib flag
    eeprom_write_block((void*)&oe_calib_data, (void*)EEP_OE_CALIB_DATA, sizeof(oe_calib_data));
    eeprom_write_byte((uint8_t*)EEP_OE_CALIB_DONE_BOOL, EEPROM_BOOL_OK_VAL);
    calib_ok = TRUE; 
}

/*
 * Init calibration part
 */
void init_calibration(void)
{
    calibdprintf_P(PSTR("-----------------------\r\n"));
    calibdprintf_P(PSTR("Calibration Init\r\n\r\n"));
    
    // Check if we stored calibration values in the eeprom
    if (eeprom_read_byte((uint8_t*)EEP_OE_CALIB_DONE_BOOL) == EEPROM_BOOL_OK_VAL)
    {        
        // Fetch values
        eeprom_read_block((void*)&oe_calib_data, (void*)EEP_OE_CALIB_DATA, sizeof(oe_calib_data));
        
        // Print values
        calibdprintf("Calibration data from %d/%d/%d\r\n", oe_calib_data.day, oe_calib_data.month, oe_calib_data.year);
        calibdprintf("Max Voltage: %umV\r\n", oe_calib_data.max_voltage);
        calibdprintf("Single ended offset: %u, approx %umV\r\n", oe_calib_data.calib_0v_value_se, compute_voltage_from_se_adc_val(oe_calib_data.calib_0v_value_se));
        calibdprintf("Singled ended offset for Vbias: %u, approx %umV\r\n", oe_calib_data.calib_0v_value_vbias, compute_voltage_from_se_adc_val(oe_calib_data.calib_0v_value_vbias));        
        calibdprintf("Oscillator low voltage: %u, approx %umV\r\n", oe_calib_data.calib_osc_low_v, compute_voltage_from_se_adc_val(oe_calib_data.calib_osc_low_v));
        calibdprintf("First thres (from high): %u, approx %umV\r\n", oe_calib_data.calib_first_thres_up, compute_voltage_from_se_adc_val(oe_calib_data.calib_first_thres_up));
        calibdprintf("First thres (from low): %u, approx %umV\r\n", oe_calib_data.calib_first_thres_down, compute_voltage_from_se_adc_val(oe_calib_data.calib_first_thres_down));
        calibdprintf("Second thres (from high): %u, approx %umV\r\n", oe_calib_data.calib_second_thres_up, compute_voltage_from_se_adc_val(oe_calib_data.calib_second_thres_up));
        calibdprintf("Second thres (from low): %u, approx %umV\r\n", oe_calib_data.calib_second_thres_down, compute_voltage_from_se_adc_val(oe_calib_data.calib_second_thres_down));
        for (uint8_t i = 0; i <= CUR_MES_64X; i++)
        {
            calibdprintf("Offset for ampl %u : %u\r\n", 1 << i, oe_calib_data.cur_measurement_offsets[i]);
        }
        calib_ok = TRUE;
    }
    else
    {
        calibdprintf_P(PSTR("Platform not calibrated\r\n"));
    }
}