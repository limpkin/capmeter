/*
 * dac.c
 *
 * Created: 30/04/2015 20:23:03
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>
#include "dac.h"


/*
 * Set vbias dac value
 * @param   val     DAC value
 */
void setup_vbias_dac(uint16_t val)
{
    PORTB_DIRSET = PIN3_bm;
    DACB.CTRLA |= DAC_CH1EN_bm;
    DACB.CH1DATA = val;
    dacdprintf("Initializing DAC Val for Vbias to %u\r\n", val);
}

/*
 * Update vbias dac value
 * @param   val     DAC value
 */
void update_vbias_dac(uint16_t val)
{
    DACB.CH1DATA = val;
}

/*
 * Disable DAC channel for vbias
 */
void disable_vbias_dac(void)
{
    PORTB_DIRCLR = PIN3_bm;
    DACB.CTRLA &= ~DAC_CH1EN_bm;
    dacdprintf_P(PSTR("Disabling vbias DAC channel\r\n"));
}

/*
 * Set opampin dac value
 * @param   val     DAC value
 */
void setup_opampin_dac(uint16_t val)
{
    PORTB_DIRSET = PIN2_bm;
    DACB.CTRLA |= DAC_CH0EN_bm;    
    DACB.CH0DATA = val;
    dacdprintf("Setting DAC Val for opampin to %u\r\n", val);
}

/*
 * Update vbias dac value
 * @param   val     DAC value
 */
void update_opampin_dac(uint16_t val)
{
    DACB.CH0DATA = val;
}

/*
 * Disable DAC channel for opampin
 */
void disable_opampin_dac(void)
{
    PORTB_DIRCLR = PIN2_bm;
    DACB.CTRLA &= ~DAC_CH0EN_bm;
    dacdprintf_P(PSTR("Disabling opampin DAC channel\r\n"));
}

/*
 * Initialize the DAC
 */
void init_dac(void)
{
    dacdprintf_P(PSTR("-----------------------\r\n"));
    dacdprintf_P(PSTR("DAC init\r\n"));
    disable_vbias_dac();                // Disable VBIAS output
    disable_opampin_dac();              // Disable OPAMPIN output
    DACB.CTRLA = DAC_ENABLE_bm;         // Enable DAC
    DACB.CTRLB = DAC_CHSEL_DUAL_gc;     // Enable both channels
    DACB.CTRLC = DAC_REFSEL_AREFB_gc;   // Use external VREF (1.25V)
    dacdprintf_P(PSTR("DAC initialized\r\n"));
}