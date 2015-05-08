/*
 * capmeter.c
 *
 * Created: 11/04/2015 22:34:20
 *  Author: limpkin
 */
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "measurement.h"
#include "interrupts.h"
#include "meas_io.h"
#include "serial.h"
#include "dac.h"


/*
 * Switch to 32MHz clock
 */
void switch_to_32MHz_clock(void)
{
    OSC.CTRL |= OSC_RC32MEN_bm;                     // Enable 32MHz oscillator
    while((OSC.STATUS & OSC_RC32MRDY_bm) == 0);     // Wait for stable oscillator
    CCP = CCP_IOREG_gc;                             // Inform process we change a protected register
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;                // Switch to 32MHz    
    OSC.CTRL &= (~OSC_RC2MEN_bm);                   // Disable the default 2Mhz oscillator    
    OSC.XOSCCTRL = OSC_XOSCSEL_32KHz_gc;            // Choose 32kHz external crystal
    OSC.CTRL |= OSC_XOSCEN_bm;                      // Enable external oscillator
    while((OSC.STATUS & OSC_XOSCRDY_bm) == 0);      // Wait for stable 32kHz clock
    OSC.DFLLCTRL = OSC_RC32MCREF_XOSC32K_gc;        // Select the 32kHz clock as calibration ref for our 32M
}

/*
 * Our main
 */
int main(void)
{
    switch_to_32MHz_clock();                        // Switch to 32MHz
    _delay_ms(1000);                                // Wait for power to settle
    init_serial_port();                             // Initialize serial port    
    init_dac();                                     // Init DAC
    init_ios();                                     // Init IOs
    enable_interrupts();                            // Enable interrupts
    calibrate_vup_vlow();                           // Calibrate vup vlow
    
    set_measurement_frequency(FREQ_1HZ);            // Set measurement frequency
    set_measurement_mode_io(RES_270);
    enable_bias_voltage(VBIAS_MAX_DAC_VAL);
    
    while(1)
    {
        measurement_loop(0);
    }
}