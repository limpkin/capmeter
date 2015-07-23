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
#include "calibration.h"
#include "measurement.h"
#include "utils.h"
#include "adc.h"
// Currently configured channel
uint8_t current_channel;
// Current ampl
uint8_t current_ampl;


/*
 * Initialize ADC
 */
void init_adc(void)
{
    /* Get ADCACAL0 from production signature . */
    ADCA.CALL = ReadCalibrationByte(PROD_SIGNATURES_START + ADCACAL0_offset);   // Set correct calibration values
    ADCA.CALH = ReadCalibrationByte(PROD_SIGNATURES_START + ADCACAL1_offset);   // Set correct calibration values
    adcprintf_P(PSTR("-----------------------\r\n"));                           // Debug
    adcprintf_P(PSTR("ADC Init\r\n"));                                          // Debug
    ADCA.CTRLA = ADC_ENABLE_bm;                                                 // Enable ADC
}

/*
 * Get configured channel
 * @return  the channel
 */
uint8_t get_configured_adc_channel(void)
{
    return current_channel;
}

/*
 * Get configured amplification
 * @return  the amplification
 */
uint8_t get_configured_adc_ampl(void)
{
    return current_ampl;
}

/*
 * Change the adc vbias channel clock divider
 * @param   divider     The divider (see defines)
 */
void set_adc_vbias_channel_clock_divider(uint8_t divider)
{
    ADCA.PRESCALER = divider;
    
    #ifdef ADC_PRINTF
        uint16_t temp = 1 << ((uint16_t)divider + 2);
        adcprintf("ADC Vbias channel clock divider set to %d\r\n", temp);
    #endif
}

/*
 * Configure an ADC channel
 * @param   channel     The channel (see defines)
 * @param   ampl        The amplification (see defines)
 * @param   debug       If we want the debug info
 */
void configure_adc_channel(uint8_t channel, uint8_t ampl, uint8_t debug)
{
    current_channel = channel;                                                      // Store current channel
    
    // Wait for a possible conversion to finish
    if (ADCA.CTRLA & ADC_CH0START_bm)
    {
        while(ADCA.CH0.INTFLAGS == 0);
        ADCA.CH0.INTFLAGS = 1;
        ADCA.CH0RES;
    }    
    
    if (channel == ADC_CHANNEL_COMPOUT)
    {
        ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV128_gc;                                   // Divide clock by 128, resulting in fadc of 250Hz (best accuracy)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;        // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN6_gc;                                   // Channel 6
        PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        if (debug)
        {
            adcprintf_P(PSTR("ADC COMPOUT channel set\r\n"));
        }
    }
    else if (channel == ADC_CHANNEL_VBIAS)
    {
        ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;                                   // Divide clock by 256, resulting in fadc of 125Hz (best accuracy)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;        // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc;                                   // Channel 7
        PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        if (debug)
        {
            adcprintf_P(PSTR("ADC VBIAS channel set\r\n"));
        }
    }
    else if (channel == ADC_CHANNEL_GND_EXT)
    {
        ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV128_gc;                                   // Divide clock by 128, resulting in fadc of 250Hz (best accuracy)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;        // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN5_gc;                                   // Channel 5
        PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        if (debug)
        {
            adcprintf_P(PSTR("ADC GND EXT channel set\r\n"));
        }
    }
    else if (channel == ADC_CHANNEL_CUR)
    {
        current_ampl = ampl;                                                        // Store current amplification
        ADCA.CTRLB = ADC_CONMODE_bm;                                                // No current limit, high impedance, signed mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                         // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV128_gc;                                   // Divide clock by 128, resulting in fadc of 250Hz (best accuracy)
        ADCA.CH0.CTRL = (ampl << ADC_CH_GAIN_gp) | ADC_CH_INPUTMODE_DIFFWGAIN_gc;   // Differential input with gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN5_gc;           // Channel 1 pos, Channel 5 neg
        PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;                                 // Disable digital input buffer
        if (debug)
        {
            adcprintf("ADC quiesc cur channel set, gain %uX\r\n", 1 << ampl);
        }
    }
    
    // Launch dummy conversion
    ADCA.CTRLA |= ADC_CH0START_bm;
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
    
    while(ADCA.CH0.INTFLAGS == 0);                                                  // Wait for conversion to finish
    return_value = ADCA.CH0RES;                                                     // Store conversion result
    ADCA.CH0.INTFLAGS = 1;                                                          // Clear conversion flag
    ADCA.CTRLA |= ADC_CH0START_bm;                                                  // Start channel 0 conversion
    
    // Depending on input mode, apply correction
    if ((ADCA.CH0.CTRL & ADC_CH_INPUTMODE_gm) == ADC_CH_INPUTMODE_SINGLEENDED_gc)
    {
        // Single ended, check we're not under 0v
        if ((uint16_t)return_value < get_single_ended_offset())
        {
            return 0;
        } 
        else
        {
            return (uint16_t)return_value - get_single_ended_offset();
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
 * @return  the averaged ADC value
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
    
    // Add 0.5 of LSB to total, compute return value
    avg += (1UL << (uint16_t)(avg_bit_shift-1));
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
 * @return  the averaged ADC value
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
    
    // Add 0.5 of LSB to total, compute return value
    avg += (1 << (uint16_t)(avg_bit_shift-1));
    return_value = (uint16_t)(avg >> avg_bit_shift);
    
    if (debug == TRUE)
    {
        adcprintf("Averaged value found: %u, pp of %u LSB\r\n", return_value, (max_val - min_val));
    }
    
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