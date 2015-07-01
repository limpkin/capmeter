/*
 * adc.c
 *
 * Created: 14/05/2015 20:48:38
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
#include "adc.h"
// 0nA ADC values for different amplifications
int16_t zero_na_outputs[7];
// Calibration 0V value
uint16_t calib_0v_value_se = 0;

/*
 * Initialize ADC
 */
void init_adc(void)
{
    ADCA.CTRLA = ADC_ENABLE_bm;                                                 // Enable ADC
    adcprintf_P(PSTR("-----------------------\r\n"));
    adcprintf_P(PSTR("ADC Init\r\n"));
    
    // Calibrate 0V, average on more than 1/50Hz
    adcprintf_P(PSTR("Measuring external 0V value, single ended...\r\n"));
    configure_adc_channel(ADC_CHANNEL_GND_EXT, 0);
    calib_0v_value_se = get_averaged_stabilized_adc_value(12, 12, TRUE);
    adcprintf("0V ADC value: %u (should be 180), approx %umV\r\n", calib_0v_value_se, (calib_0v_value_se*10)/33);
    
    // Fetch differential calibration data if stored
    if (eeprom_read_byte((uint8_t*)CALIB_DIFF_DATA_BOOL) == EEPROM_BOOL_OK_VAL)
    {
        eeprom_read_block((void*)zero_na_outputs, (void*)CALIB_DIFF_VAL_START, sizeof(zero_na_outputs));
        adcprintf_P(PSTR("Differential calibration values fetched from EEPROM\r\n"));
    }
}
 

/*
 * Configure an ADC channel
 * @param   channel     The channel (see defines)
 * @param   ampl        The amplification (see defines)
 */
void configure_adc_channel(uint8_t channel, uint8_t ampl)
{
    if (channel == ADC_CHANNEL_COMPOUT)
    {
        ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;                                    // Divide clock by 64 (arbitrary)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;        // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN6_gc;                                   // Channel 6
        PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        adcprintf_P(PSTR("ADC COMPOUT channel set\r\n"));
    }
    else if (channel == ADC_CHANNEL_VBIAS)
    {
        ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV16_gc;                                    // Divide clock by 16 (maximum)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;        // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc;                                   // Channel 7
        PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        adcprintf_P(PSTR("ADC VBIAS channel set\r\n"));
    }
    else if (channel == ADC_CHANNEL_GND_EXT)
    {
        ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV128_gc;                                   // Divide clock by 128 so a call to get_averaged_stabilized_adc_value(10...) takes more than 1/50Hz to complete
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;        // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN5_gc;                                   // Channel 5
        PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        adcprintf_P(PSTR("ADC GND EXT channel set\r\n"));
    }
    else if (channel == ADC_CHANNEL_CUR)
    {
        ADCA.CTRLB = ADC_CONMODE_bm;                                                // No current limit, high impedance, signed mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV128_gc;                                   // Divide clock by 128 so a call to get_averaged_stabilized_adc_value(10...) takes more than 1/50Hz to complete
        ADCA.CH0.CTRL = (ampl << ADC_CH_GAIN_gp) | ADC_CH_INPUTMODE_DIFFWGAIN_gc;   // Differential input with gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN5_gc;           // Channel 1 pos, Channel 5 neg
        PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        adcprintf("ADC quiesc cur channel set, gain %uX\r\n", 1 << ampl);
    }
    
    // Launch dummy conversion
    ADCA.CH0.INTFLAGS = 1;                                                          // Clear conversion flag
    ADCA.CTRLA |= ADC_CH0START_bm;                                                  // Start channel 0 conversion
    while(ADCA.CH0.INTFLAGS == 0);                                                  // Wait for conversion to finish
    ADCA.CH0RES;                                                                    // Dummy read
} 

/*
 * Disable ADC channel
 */
void disable_adc_channel(uint8_t channel)
{
    ADCA.CTRLB = 0;                                                                 // No current limit, high impedance, unsigned mode, 12-bit right adjusted
    
    if (channel == ADC_CHANNEL_COMPOUT)
    {
        PORTA.PIN6CTRL = PORT_ISC_RISING_gc;                                        // Enable digital input buffer
        adcprintf_P(PSTR("ADC COMPOUT channel disabled\r\n"));
    }    
}

/*
 * Start and wait for an ADC conversion to finish
 * @return the ADC value corrected for offsets
 */
int16_t start_and_wait_for_adc_conversion(void)
{
    int16_t return_value;
    
    ADCA.CH0.INTFLAGS = 1;                                                          // Clear conversion flag
    ADCA.CTRLA |= ADC_CH0START_bm;                                                  // Start channel 0 conversion
    while(ADCA.CH0.INTFLAGS == 0);                                                  // Wait for conversion to finish
    return_value = ADCA.CH0RES;                                                     // Store conversion result
    
    // Depending on input mode, apply correction
    if ((ADCA.CH0.CTRL & ADC_CH_INPUTMODE_gm) == ADC_CH_INPUTMODE_SINGLEENDED_gc)
    {
        // Single ended, check we're not under 0v
        if (return_value < calib_0v_value_se)
        {
            return 0;
        } 
        else
        {
            return return_value-calib_0v_value_se;
        }
    }
    else if ((ADCA.CH0.CTRL & ADC_CH_INPUTMODE_gm) == ADC_CH_INPUTMODE_DIFFWGAIN_gc)
    {
        uint8_t current_ampl = (ADCA.CH0.CTRL & ADC_CH_GAIN_gm) >> ADC_CH_GAIN_gp;
        
        // Differential with gain, check we're not under 0v and that it isn't the max val
        if (return_value == 2047)
        {
            return return_value;
        }
        else
        {
            return return_value-zero_na_outputs[current_ampl];
        }
    }
    else
    {
        return return_value;    
    }
}

/*
 * Get an averaged ADC value
 * @param   avg_bit_shift   Bit shift for our averaging (1 for 2 samples, 2 for 4, etc etc)
 */
uint16_t get_averaged_adc_value(uint8_t avg_bit_shift)
{
    uint32_t i = (1UL << avg_bit_shift);
    uint16_t return_value;
    int32_t avg = 0;
    
    // Add to avg until we looped enough times
    while(i--)
    {
        avg += start_and_wait_for_adc_conversion();
    }
    
    // Compute return value
    return_value = (uint16_t)(avg >> avg_bit_shift);
    
    // Don't return a negative value
    if (return_value > MAX_ADC_VAL)
    {
        return 0;
    } 
    else
    {
        return return_value;
    }
}

/*
 * Wait for a stabilized adc value
 * @param   avg_bit_shift   Bit shift for our averaging (1 for 2 samples, 2 for 4, etc etc, max 15!)
 * @param   max_pp          Max peak to peak value we accept
 * @note    When DIV64 is used for the ADC channel prescaler, avg_bit_shift of 8 (eg 256) leads to 4.6ms
 */
uint16_t get_averaged_stabilized_adc_value(uint8_t avg_bit_shift, uint16_t max_pp, uint8_t debug)
{
    int16_t min_val = 0, max_val = 0, temp_val;
    uint8_t loop_running = TRUE;
    uint16_t return_value;
    int32_t avg = 0;
    
    if (debug == TRUE)
    {
        adcprintf("Getting averaged value for %u samples, with less than %u LSB pp\r\n", (1 << avg_bit_shift), max_pp);
    }
        
    while (loop_running == TRUE)
    {
        for (uint16_t i = 0; i < (1 << (uint16_t)avg_bit_shift); i++)
        {
            // Get one val
            temp_val = start_and_wait_for_adc_conversion();
            
            // If it is the first iteration
            if (i == 0)
            {
                min_val = temp_val;
                max_val = temp_val;
                avg = 0;
            }
            
            // Add sample
            avg += temp_val;
            
            // Check min/max
            if (temp_val > max_val)
            {
                max_val = temp_val;
            }
            else if (temp_val < min_val)
            {
                min_val = temp_val;
            }
            
            // Check current peak to peak, leave loop
            if ((max_val - min_val) > max_pp)
            {
                //adcprintf("%u ", (max_val - min_val));
                break;
            }
        }
        
        // Loop over, check peak to peak
        if ((max_val - min_val) <= max_pp)
        {
            loop_running = FALSE;
        }
    }
    
    if (debug == TRUE)
    {
        adcprintf("Averaged value found: %u, pp of %u LSB\r\n", (uint16_t)(avg >> avg_bit_shift), (max_val - min_val));
    }
    
    // Compute return value
    return_value = (uint16_t)(avg >> avg_bit_shift);
    
    // Don't return a negative value
    if (return_value > MAX_ADC_VAL)
    {
        return 0;
    }
    else
    {
        return return_value;
    }
}

/*
 * Find 0nA point
 */
void calibrate_cur_mos_0nA(void)
{
    int16_t cur_0nA_val = INT16_MAX;
    uint8_t cur_ampl = CUR_MES_1X;
    
    adcprintf(PSTR("-----------------------\r\n"));
    adcprintf(PSTR("Measuring 0nA outputs\r\n"));
    
    // Clear current values
    memset((void*)zero_na_outputs, 0x00, sizeof(zero_na_outputs));
    
    // Set a bias voltage to make sure that nothing is connected between the leads
    enable_bias_voltage(4000);
    
    // Calibrate all 0nA points for different amplifications, start with the smaller ampl
    set_current_measurement_ampl(cur_ampl);
    while(1)
    {        
        // Average on more than 1/50Hz
        cur_0nA_val = get_averaged_adc_value(18);
        // Only take result into account when nothing is connected to the leads
        if (cur_0nA_val < (16*(1 << cur_ampl)))
        {
            // Only possible because of the ampl registers
            adcprintf("Zero quiescent current for ampl %u: %d, approx %d*10/%unA\r\n", 1 << cur_ampl, cur_0nA_val, ((cur_0nA_val)*20)/33, 1 << cur_ampl);
            zero_na_outputs[cur_ampl++] = cur_0nA_val;
            if (cur_ampl <= CUR_MES_64X)
            {
                set_current_measurement_ampl(cur_ampl);
                _delay_ms(10);
            }
            else
            {
                break;
            }
        }
    }
    
    // Store values in eeprom
    eeprom_write_block((void*)zero_na_outputs, (void*)CALIB_DIFF_VAL_START, sizeof(zero_na_outputs));
    eeprom_write_byte((void*)CALIB_DIFF_DATA_BOOL, EEPROM_BOOL_OK_VAL);
    adcprintf(PSTR("Values stored in EEPROM\r\n"));
    
    disable_current_measurement_mode();
    disable_bias_voltage();
}