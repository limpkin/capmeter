/*
 * adc.c
 *
 * Created: 14/05/2015 20:48:38
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "measurement.h"
#include "adc.h"
// Calibration 0V value
uint16_t calib_0v_value_se;
 
void configure_adc_channel(uint8_t channel, uint8_t ampl)
{
    if (channel == ADC_CHANNEL_COMPOUT)
    {
        ADCA.CTRLB = 0;                                                         // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                     // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;                                // Divide clock by 64 (arbitrary)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;    // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN6_gc;                               // Channel 6
        PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;                             // Disable digital input buffer
        adcprintf_P(PSTR("ADC COMPOUT channel set\r\n"));
    }
    else if (channel == ADC_CHANNEL_VBIAS)
    {
        ADCA.CTRLB = 0;                                                         // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                     // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;                                // Divide clock by 64 (arbitrary)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;    // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc;                               // Channel 7
        PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;                             // Disable digital input buffer
        adcprintf_P(PSTR("ADC VBIAS channel set\r\n"));
    }
    else if (channel == ADC_CHANNEL_GND_EXT)
    {
        ADCA.CTRLB = 0;                                                         // No current limit, high impedance, unsigned mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                     // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;                                // Divide clock by 64 (arbitrary)
        ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;    // Single ended input, no gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN5_gc;                               // Channel 5
        PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;                             // Disable digital input buffer
        adcprintf_P(PSTR("ADC GND EXT channel set\r\n"));
    }
    else if (channel == ADC_CHANNEL_CUR)
    {
        ADCA.CTRLB = ADC_CONMODE_bm;                                            // No current limit, high impedance, signed mode, 12-bit right adjusted
        ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;                                     // External 1.24V ref
        ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;                                // Divide clock by 64 (arbitrary)
        ADCA.CH0.CTRL = ampl | ADC_CH_INPUTMODE_DIFFWGAIN_gc;                   // Differential input with gain
        ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc|ADC_CH_MUXNEG_PIN5_gc;         // Channel 1 pos, Channel 5 neg
        PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;                             // Disable digital input buffer
        adcprintf("ADC quiesc cur channel set, gain %uX\r\n", 1 << ampl);
    }
    
    // Launch dummy conversion
    ADCA.CH0.INTFLAGS = 1;                                                      // Clear conversion flag
    ADCA.CTRLA |= ADC_CH0START_bm;                                              // Start channel 0 conversion
    while(ADCA.CH0.INTFLAGS == 0);                                              // Wait for conversion to finish
    ADCA.CH0RES;                                                                // Dummy read
} 

void disable_adc_channel(uint8_t channel)
{
    ADCA.CTRLB = 0;                                                             // No current limit, high impedance, unsigned mode, 12-bit right adjusted
    
    if (channel == ADC_CHANNEL_COMPOUT)
    {
        PORTA.PIN6CTRL = PORT_ISC_RISING_gc;                                    // Enable digital input buffer
        adcprintf_P(PSTR("ADC COMPOUT channel disabled\r\n"));
    }    
}

uint16_t start_and_wait_for_adc_conversion(void)
{
    uint16_t return_value;
    
    ADCA.CH0.INTFLAGS = 1;                                                      // Clear conversion flag
    ADCA.CTRLA |= ADC_CH0START_bm;                                              // Start channel 0 conversion
    while(ADCA.CH0.INTFLAGS == 0);                                              // Wait for conversion to finish
    return_value = ADCA.CH0RES;                                                 // Store conversion result
    
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
    else
    {
        return return_value;    
    }
}

void init_adc(void)
{
    ADCA.CTRLA = ADC_ENABLE_bm;                                                 // Enable ADC
    adcprintf_P(PSTR("-----------------------\r\n"));
    adcprintf_P(PSTR("ADC Init\r\n"));
    
    // Calibrate 0V
    adcprintf_P(PSTR("Measuring external 0V value, single ended...\r\n"));
    configure_adc_channel(ADC_CHANNEL_GND_EXT, 0);
    calib_0v_value_se = get_averaged_stabilized_adc_value(10, 8, FALSE);
    adcprintf("0V ADC value: %u, approx %umV\r\n", calib_0v_value_se, (calib_0v_value_se*10)/33);
}