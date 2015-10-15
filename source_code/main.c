/*
 * capmeter.c
 *
 * Created: 11/04/2015 22:34:20
 *  Author: limpkin
 */
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/io.h>
#include <stdio.h>
#include "automated_testing.h"
#include "eeprom_addresses.h"
#include "conversions.h"
#include "measurement.h"
#include "calibration.h"
#include "interrupts.h"
#include "meas_io.h"
#include "serial.h"
#include "utils.h"
#include "vbias.h"
#include "tests.h"
#include "main.h"
#include "dac.h"
#include "adc.h"
#include "usb.h"
// Define the bootloader function
bootloader_f_ptr_type start_bootloader = (bootloader_f_ptr_type)(BOOT_SECTION_START/2);
// Bootloader start key variable
uint16_t bootloader_start_var __attribute__ ((section (".noinit")));
// RC Calibration values
#define RCOSC32M_offset  0x03
#define RCOSC32MA_offset 0x04
// Capacitance report
capacitance_report_t cap_report;
// USB message
usb_message_t usb_packet;


/*
 * Switch to 32MHz clock
 */
void switch_to_32MHz_clock(void)
{
    DFLLRC32M.CALA = ReadCalibrationByte(PROD_SIGNATURES_START + RCOSC32MA_offset); // Load calibration value for 32M RC Oscillator
    DFLLRC32M.CALB = ReadCalibrationByte(PROD_SIGNATURES_START + RCOSC32M_offset);  // Load calibration value for 32M RC Oscillator
    OSC.CTRL |= OSC_RC32MEN_bm;                                                     // Enable 32MHz oscillator
    while((OSC.STATUS & OSC_RC32MRDY_bm) == 0);                                     // Wait for stable oscillator
    CCP = CCP_IOREG_gc;                                                             // Inform process we change a protected register
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;                                                // Switch to 32MHz    
    OSC.CTRL &= (~OSC_RC2MEN_bm);                                                   // Disable the default 2Mhz oscillator    
    OSC.XOSCCTRL = OSC_XOSCSEL_32KHz_gc;                                            // Choose 32kHz external crystal
    OSC.CTRL |= OSC_XOSCEN_bm;                                                      // Enable external oscillator
    while((OSC.STATUS & OSC_XOSCRDY_bm) == 0);                                      // Wait for stable 32kHz clock
    OSC.DFLLCTRL = OSC_RC32MCREF_XOSC32K_gc;                                        // Select the 32kHz clock as calibration ref for our 32M
    DFLLRC32M.CTRL = DFLL_ENABLE_bm;                                                // Enable DFLL for RC32M
}

/*
 * Our main
 */
int main(void)
{
    // Check if we need to go to the bootloader
    if (bootloader_start_var == 0xBEEF)
    {
        bootloader_start_var = 0x0000;
        wdt_disable();
        start_bootloader();
    }
    switch_to_32MHz_clock();                        // Switch to 32MHz
    _delay_ms(1000);                                // Wait for power to settle
    init_serial_port();                             // Initialize serial port    
    init_dac();                                     // Init DAC
    init_adc();                                     // Init ADC
    init_ios();                                     // Init IOs
    init_calibration();                             // Init calibration
    enable_interrupts();                            // Enable interrupts
    init_usb();                                     // Init USB comms
    functional_test();                              // Functional test if started for the first time

    uint8_t current_fw_mode = MODE_IDLE;
    while(1)
    {
        if (current_fw_mode == MODE_CAP_MES)
        {
            // If we are in cap measurement mode and have a report to send
            if(cap_measurement_loop(&cap_report) == TRUE)
            {
                maindprintf_P(PSTR("*"));
                usb_packet.length = sizeof(cap_report);
                usb_packet.command_id = CMD_CAP_MES_REPORT;
                memcpy((void*)usb_packet.payload, (void*)&cap_report, sizeof(cap_report));
                usb_send_data((uint8_t*)&usb_packet);
            }
        }
        
        // USB command parser
        if (usb_receive_data((uint8_t*)&usb_packet) == TRUE)
        {
            switch(usb_packet.command_id)
            {
                case CMD_BOOTLOADER_START:
                {
                    maindprintf_P(PSTR("USB- Bootloader start"));
                    bootloader_start_var = 0xBEEF;
                    wdt_enable(WDTO_1S);
                    while(1);
                }
                case CMD_PING: 
                {
                    maindprintf_P(PSTR("."));
                    // Ping packet, resend the same one
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_VERSION:
                {
                    maindprintf_P(PSTR("USB- Version\r\n"));
                    // Version request packet
                    strcpy((char*)usb_packet.payload, CAPMETER_VER);
                    usb_packet.length = sizeof(CAPMETER_VER);
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_OE_CALIB_STATE:
                {
                    maindprintf_P(PSTR("USB- Calib state\r\n"));
                    // Get open ended calibration state.
                    if (is_platform_calibrated() == TRUE)
                    {
                        // Calibrated, return calibration data
                        usb_packet.length = get_openended_calibration_data(usb_packet.payload);
                    } 
                    else
                    {
                        // Not calibrated, return 0
                        usb_packet.length = 1;
                        usb_packet.payload[0] = 0;
                    }
                    usb_send_data((uint8_t*)&usb_packet);    
                    break;                
                }
                case CMD_OE_CALIB_START:
                {
                    maindprintf_P(PSTR("USB- Calib start\r\n"));
                    // Check if we are in idle mode
                    if (current_fw_mode == MODE_IDLE)
                    {
                        // Calibration start
                        start_openended_calibration(usb_packet.payload[0], usb_packet.payload[1], usb_packet.payload[2]);
                        usb_packet.length = get_openended_calibration_data(usb_packet.payload);
                    }
                    else
                    {
                        usb_packet.length = 1;
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    usb_send_data((uint8_t*)&usb_packet);    
                    break;                
                }
                case CMD_GET_OE_CALIB:
                {
                    maindprintf_P(PSTR("USB- Calib data\r\n"));
                    // Get calibration data
                    usb_packet.length = get_openended_calibration_data(usb_packet.payload);
                    usb_send_data((uint8_t*)&usb_packet);    
                    break;                
                }
                case CMD_SET_VBIAS:
                {
                    // Check that we are not measuring anything and if so, skip samples and stop oscillation
                    if (current_fw_mode == MODE_CAP_MES)
                    {
                        pause_capacitance_measurement_mode();
                    }
                    
                    // Enable and set vbias... can also be called to update it
                    uint16_t* temp_vbias = (uint16_t*)usb_packet.payload;
                    uint16_t set_vbias = enable_bias_voltage(*temp_vbias);
                    uint16_t cur_dacv = get_current_vbias_dac_value();
                    usb_packet.length = 4;
                    memcpy((void*)usb_packet.payload, (void*)&set_vbias, sizeof(set_vbias));
                    memcpy((void*)&usb_packet.payload[2], (void*)&cur_dacv, sizeof(cur_dacv));
                    usb_send_data((uint8_t*)&usb_packet);
                    
                    // If we are measuring anything, resume measurements
                    if (current_fw_mode == MODE_CAP_MES)
                    {
                        resume_capacitance_measurement_mode();
                    }                    
                    break;
                }
                case CMD_DISABLE_VBIAS:
                {
                    // Disable vbias
                    usb_packet.length = 0;
                    disable_bias_voltage();
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_CUR_MES_MODE:
                {
                    // Enable current measurement or start another measurement
                    if (current_fw_mode == MODE_IDLE)
                    {
                        set_current_measurement_mode(usb_packet.payload[0]);
                        current_fw_mode = MODE_CURRENT_MES;
                    } 
                    // Check if we are in the right mode to start a measurement
                    if (current_fw_mode == MODE_CURRENT_MES)
                    {
                        // We either just set current measurement mode or another measurement was requested
                        if (get_configured_adc_ampl() != usb_packet.payload[0])
                        {
                            // If the amplification isn't the same one as requested
                            set_current_measurement_mode(usb_packet.payload[0]);
                        }
                        // Start measurement
                        usb_packet.length = 2;
                        uint16_t return_value = cur_measurement_loop(usb_packet.payload[1]);
                        memcpy((void*)usb_packet.payload, (void*)&return_value, sizeof(return_value));
                        usb_send_data((uint8_t*)&usb_packet);
                    }
                    else
                    {
                        usb_packet.length = 1;
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                        usb_send_data((uint8_t*)&usb_packet);
                    }
                    break;
                }
                case CMD_CUR_MES_MODE_EXIT:
                {
                    if (current_fw_mode == MODE_CURRENT_MES)
                    {
                        usb_packet.payload[0] = USB_RETURN_OK;
                        disable_current_measurement_mode();
                        current_fw_mode = MODE_IDLE;
                    }
                    else
                    {
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    usb_packet.length = 1;
                    usb_send_data((uint8_t*)&usb_packet);
                    break;                    
                }
                case CMD_CAP_REPORT_FREQ:
                {
                    if (current_fw_mode == MODE_IDLE)
                    {
                        set_capacitance_report_frequency(usb_packet.payload[0]);
                        usb_packet.payload[0] = USB_RETURN_OK;
                    }
                    else
                    {
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    usb_packet.length = 1;
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_CAP_MES_START:
                {
                    if (current_fw_mode == MODE_IDLE)
                    {
                        current_fw_mode = MODE_CAP_MES;
                        set_capacitance_measurement_mode();
                        usb_packet.payload[0] = USB_RETURN_OK;
                    }
                    else
                    {
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    usb_packet.length = 1;
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_CAP_MES_EXIT:
                {
                    if (current_fw_mode == MODE_CAP_MES)
                    {
                        current_fw_mode = MODE_IDLE;
                        disable_capacitance_measurement_mode();
                        usb_packet.payload[0] = USB_RETURN_OK;
                    }
                    else
                    {
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    usb_packet.length = 1;
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_SET_VBIAS_DAC:
                {
                    uint16_t* requested_dac_val = (uint16_t*)usb_packet.payload;
                    uint16_t* requested_wait = (uint16_t*)&usb_packet.payload[2];
                    
                    usb_packet.length = 2;
                    if (is_ldo_enabled() == TRUE)
                    {
                        uint16_t set_vbias = force_vbias_dac_change(*requested_dac_val, *requested_wait);
                        memcpy((void*)usb_packet.payload, (void*)&set_vbias, sizeof(set_vbias));
                    } 
                    else
                    {
                        usb_packet.payload[0] = 0;
                        usb_packet.payload[1] = 0;
                    }        
                    usb_send_data((uint8_t*)&usb_packet);            
                    break;
                }
                case CMD_RESET_STATE:
                {
                    maindprintf_P(PSTR("USB- Reset\r\n"));
                    usb_packet.length = 1;
                    current_fw_mode = MODE_IDLE;
                    if(is_platform_calibrated() == TRUE)
                    {
                        disable_bias_voltage();
                        disable_current_measurement_mode();
                        disable_capacitance_measurement_mode();
                        usb_packet.payload[0] = USB_RETURN_OK;                        
                    }
                    else
                    {
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_SET_EEPROM_VALS:
                {
                    uint16_t* addr = (uint16_t*)usb_packet.payload;
                    uint16_t size = usb_packet.payload[2];
                    if(((*addr) + size > APP_STORED_DATA_MAX_SIZE) || (size > (RAWHID_RX_SIZE-5)))
                    {
                         usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    else
                    {
                        eeprom_write_block((void*)&usb_packet.payload[3], (void*)(EEP_APP_STORED_DATA + (*addr)), size);
                        usb_packet.payload[0] = USB_RETURN_OK;
                    }
                    usb_packet.length = 1;
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                case CMD_READ_EEPROM_VALS:
                {
                    uint16_t* addr = (uint16_t*)usb_packet.payload;
                    uint16_t size = usb_packet.payload[2];
                    if(((*addr) + size > APP_STORED_DATA_MAX_SIZE) || (size > (RAWHID_TX_SIZE-2)))
                    {
                        usb_packet.length = 1;
                        usb_packet.payload[0] = USB_RETURN_ERROR;
                    }
                    else
                    {
                        usb_packet.length = size;
                        eeprom_read_block(usb_packet.payload, (void*)(EEP_APP_STORED_DATA + (*addr)), size);
                    }
                    usb_send_data((uint8_t*)&usb_packet);
                    break;
                }
                default: break;
            }
        }
    }
    
    // Left here for reference
    //enable_bias_voltage(850);while(1);
    //automated_current_testing();
    //ramp_bias_voltage_test();
    //voltage_settling_test();
    //automated_vbias_testing();
    //automated_current_testing();
    //peak_to_peak_adc_noise_measurement_test();
    //ramp_bias_voltage_test();
    //printf("blah\r\n");_delay_ms(3333);
    //ramp_current_test();
    //functional_test();
    //while(1);
    //calibrate_thresholds();                         // Calibrate vup vlow & thresholds
    //calibrate_cur_mos_0nA();                        // Calibrate 0nA point and store values in eeprom
    //calibrate_current_measurement();                // Calibrate the ADC for current measurements
}
