/*
 * tests.c
 *
 * Created: 24/07/2015 18:11:32
 *  Author: limpkin
 */ 
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include "conversions.h"
#include "calibration.h"
#include "measurement.h"
#include "meas_io.h"
#include "utils.h"
#include "vbias.h"
#include "tests.h"
#include "dac.h"
#include "adc.h"

/*
 * To measure set voltages
 */
void functional_test(void)
{
    testdprintf_P(PSTR("\r\n\r\n\r\n\r\n--------------------------\r\n"));
    testdprintf_P(PSTR("--- FUNCTIONAL TESTING ---\r\n"));
    testdprintf_P(PSTR("-\r\n"));
    uint8_t test_passed = TRUE;
    
    // Check the single ended offset
    if (check_value_range(get_single_ended_offset(), 170, 200) == FALSE)
    {
        testdprintf("- OFFSET PROBLEM: %u\r\n", get_single_ended_offset());
        test_passed = FALSE;
    } 
    else
    {
        testdprintf("- OFFSET OK: %u\r\n", get_single_ended_offset());
    }
    
    // Check the max voltage
    if (get_max_vbias_voltage() < 15000)
    {
        testdprintf("- MAX VOLTAGE PROBLEM: %u\r\n", get_max_vbias_voltage());
        test_passed = FALSE;
    } 
    else
    {
        testdprintf("- MAX VOLTAGE OK: %u\r\n", get_max_vbias_voltage());
    }
    
    // Check thresholds
    // Thres1: Vthres = 3.3V * 2.15 / (2.15 + 10)
    // Taking into account 1% tolerances, but not 1.24V ref tolerance
    // low: 3.3*0.99*2.15*0.99 / (2.15 * 0.99 + 10 * 1.01)
    // high: 3.3*1.01*2.15*1.01 / (2.15 * 1.01 + 10 * 0.99)
    // dac values: Vdac = Val(DAC) * 1.24 / 4095 >> Val(DAC) = Vdac * 4095 / 1.24
    // low: 3.3*0.99*2.15*0.99*4095 / ((2.15 * 0.99 + 10 * 1.01) * 1.24) = 1878
    // high: 3.3*1.01*2.15*1.01*4095 / ((2.15 * 1.01 + 10 * 0.99) * 1.24) = 1980
    if (check_value_range(get_calib_first_thres_down(), 1878, 1980) == FALSE)
    {
        testdprintf("- FIRST THRES DOWN PROBLEM: %u\r\n", get_calib_first_thres_down());
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- FIRST THRES DOWN OK: %u\r\n", get_calib_first_thres_down());
    }
    if (check_value_range(get_calib_first_thres_up(), 1878, 1980) == FALSE)
    {
        testdprintf("- FIRST THRES UP PROBLEM: %u\r\n", get_calib_first_thres_up());
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- FIRST THRES UP OK: %u\r\n", get_calib_first_thres_up());
    }
    
    // Thres1: Vthres = 3.3V * 10 / (10 + 20.5)
    // Taking into account 1% tolerances, but not 1.24V ref tolerance
    // low: 3.3*0.99*10*0.99 / (10 * 0.99 + 20.5 * 1.01)
    // high: 3.3*1.01*10*1.01 / (10 * 1.01 + 20.5 * 0.99)
    // dac values: Vdac = Val(DAC) * 1.24 / 4095 >> Val(DAC) = Vdac * 4095 / 1.24
    // low: 3.3*0.99*10*0.99*4095 / ((10 * 0.99 + 20.5 * 1.01) * 1.24) = 3490
    // high: 3.3*1.01*10*1.01*4095 / ((10 * 1.01 + 20.5 * 0.99) * 1.24) = 3658
    if (check_value_range(get_calib_second_thres_down(), 3490, 3658) == FALSE)
    {
        testdprintf("- SECOND THRES DOWN PROBLEM: %u\r\n", get_calib_second_thres_down());
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- SECOND THRES DOWN OK: %u\r\n", get_calib_second_thres_down());
    }
    if (check_value_range(get_calib_second_thres_up(), 3490, 3658) == FALSE)
    {
        testdprintf("- SECOND THRES UP PROBLEM: %u\r\n", get_calib_second_thres_up());
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- SECOND THRES UP OK: %u\r\n", get_calib_second_thres_up());
    }
    
    // Check AVCC, select ADC channel for AVCC / 10
    // Vadc = Val(ADC) * (1.24 / 4095)
    // Val(ADC) = Vadc * (4095 / 1.24+-0.25%)
    // Between 1087 and 1093 for 0.33V >> setting +-17 LSB, which is +-50mV
    configure_adc_channel(ADC_CHANNEL_AVCCDIV10, 0, FALSE);
    uint16_t avcc = get_averaged_adc_value(13);
    if (check_value_range(avcc, 1070, 1110) == FALSE)
    {
        testdprintf("- AVCC PROBLEM: %u (~%umV)\r\n", avcc, compute_voltage_from_se_adc_val(avcc)*10);
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- AVCC OK: %u (~%umV)\r\n", avcc, compute_voltage_from_se_adc_val(avcc)*10);
    }
    
    // Check AREF    
    // Vadc ~= Val(ADC) * 3.3 / (1.6 * 4095)
    // Val(ADC) = Vadc * (6552 / 3.3)
    // Val(ADC) = 1.24 * (6552 / 3.3)
    // Approximately 2462 +-50LSB (2.5mV)
    delete_single_ended_offset();
    configure_adc_channel(ADC_CHANNEL_GND_EXT_VCCDIV16, 0, FALSE);
    uint16_t aref_offset = get_averaged_adc_value(13);
    configure_adc_channel(ADC_CHANNEL_AREF, 0, FALSE);
    uint16_t aref = get_averaged_adc_value(13);
    if (check_value_range(aref-aref_offset, 2412, 2512) == FALSE)
    {
        testdprintf("- AREF PROBLEM: %u (~%umV)\r\n", aref-aref_offset, compute_voltage_from_se_adc_val_with_avcc_div16_ref(aref-aref_offset));
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- AREF OK: %u (~%umV)\r\n", aref-aref_offset, compute_voltage_from_se_adc_val_with_avcc_div16_ref(aref-aref_offset));
    }
    calibrate_single_ended_offset();
    
    // Check Vbias generation (LDO)
    // Vref = VR2 + Vdac
    // VR2 = (Vout - Vdac) * R2 / (R1 + R2)
    // Vout = Vref * (R1 + R2) / R2 - Vdac * R1 / R2
    // Vout = 1.188 * 16.2 / 1.2 - Vdac * 15 / 1.2
    // Vout = 16.038 - Val(DAC) * (1.24 / 4095) * (15 / 1.2)
    // Vout = 16,038 - Val(DAC) * 0,0037851
    // For 3210 : 3.888
    setup_vbias_dac(3210);
    enable_ldo();
    _delay_ms(200);
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
    uint16_t voltage = compute_vbias_for_adc_value(get_averaged_adc_value(14));
    if (check_value_range(voltage, 3738, 4038) == FALSE)
    {
        testdprintf("- VBIAS GENERATION (LDO) PROBLEM: %u\r\n", voltage);
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- VBIAS GENERATION (LDO) OK: %u\r\n", voltage);
    }
    
    // Check Vbias generation (stepup)
    // For 1234 : 11.367
    setup_vbias_dac(1234);
    enable_stepup();
    _delay_ms(200);
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
    voltage = compute_vbias_for_adc_value(get_averaged_adc_value(14));
    if (check_value_range(voltage, 11217, 11517) == FALSE)
    {
        testdprintf("- VBIAS GENERATION (STEPUP) PROBLEM: %u\r\n", voltage);
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- VBIAS GENERATION (STEPUP) OK: %u\r\n", voltage);
    }
    
    // Check vbias quenching
    RTC.CNT = 0;                                                    // Reset counter
    RTC.INTCTRL = 0;                                                // Disable Interrupts
    RTC.PER = 0xFFFF;                                               // Set max period for RTC
    RTC.CTRL = RTC_PRESCALER_DIV1024_gc;                            // Set 32768 / 1024 = 32Hz base frequency
    CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;              // Select 32kHz crystal for the RTC, enable it    
    disable_ldo();                                                  // Disable LDO
    disable_stepup();                                               // Disable stepup
    disable_vbias_dac();                                            // Disable DAC controlling the ldo    
    enable_vbias_quenching();                                       // Enable vbias quenching
    configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);              // Set ADC Channel to vbias
    uint16_t measured_vbias = 2000;
    while (measured_vbias > 100)                                    // Wait until voltage is around 400mV
    {
        measured_vbias = get_averaged_adc_value(6);
    }
    uint16_t time_to_quench = RTC.CNT;                              // Measure the time it took to quench vbias
    if (RTC.CNT > 100)
    {
        testdprintf("- VBIAS QUENCHING PROBLEM: %u/32 secs\r\n", time_to_quench);
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- VBIAS QUENCHING OK: %u/32 secs\r\n", time_to_quench);
    }    
    disable_vbias_quenching();
    
    // Check current measurement
    uint16_t cur_measure;
    enable_bias_voltage(4420);
    set_current_measurement_mode(CUR_MES_1X);
    cur_measure = cur_measurement_loop(15);
    if (check_value_range(cur_measure, 11200, 11550) == FALSE)
    {
        testdprintf("- CUR MEASUREMENT PROBLEM: %u\r\n", cur_measure);
        print_compute_cur_formula(cur_measure);
        test_passed = FALSE;
    }
    else
    {
        testdprintf("- CUR MEASUREMENT OK: %u\r\n", cur_measure);
    }
    
    while(1);    
}

/*
 * To measure set voltages
 */
void ramp_bias_voltage_test(void)
{
    // This is the routine to check that we actually can provide a vbias within +-0.5% specifications
    // We are running this scenario in the worst case: 270Ohms resistor, ~17uF cap connected
    // We want to make sure that the oscillations induced to vbias via the capacitor don't
    // break our vbias setting algorithm.
    testdprintf_P(PSTR("-----------------------\r\n"));
    testdprintf_P(PSTR("Ramp Voltage Test\r\n\r\n"));
        
    uint16_t set_voltage, correct_voltage, agg_error = 0;    
    enable_bias_voltage(15000);
    _delay_ms(5000);
    disable_bias_voltage();
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
    {
        set_voltage = update_bias_voltage(i);
        _delay_ms(1500);
        correct_voltage = compute_vbias_for_adc_value(get_averaged_adc_value(18));
        if (i > correct_voltage)
        {
            agg_error += i - correct_voltage;
        }
        else
        {
            agg_error += correct_voltage - i;
        }
        testdprintf("Call: %u, Set: %u, Measured: %u, Error Set: %d, Error Measured: %d\r\n", i, set_voltage, correct_voltage, correct_voltage - set_voltage, correct_voltage - i);
    }
    testdprintf_P(PSTR("-----------------------\r\n"));
    testdprintf("Accumulated error : %d\r\n", agg_error);
    testdprintf_P(PSTR("-----------------------\r\n"));    
    disable_bias_voltage();
}

void peak_to_peak_adc_noise_measurement_test(void)
{
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
    {
        update_bias_voltage(i);
        _delay_ms(100);
        measure_peak_to_peak_on_channel(BIT_AVG_FINE, ADC_CHANNEL_VBIAS, 0);
    }
    
    disable_bias_voltage();
}

/*
 * Ramp voltage and measure the current
 */
void ramp_current_test(void)
{
    uint16_t cur_measure;
    
    testdprintf_P(PSTR("-----------------------\r\n"));
    testdprintf_P(PSTR("Ramp Current Test\r\n\r\n"));
    
    set_current_measurement_mode(CUR_MES_1X);
    enable_bias_voltage(VBIAS_MIN_V);
    for (uint16_t i = VBIAS_MIN_V; i <= get_max_vbias_voltage(); i+= 50)
    {
        update_bias_voltage(i);
        _delay_ms(10);
        cur_measure = cur_measurement_loop(16);
        print_compute_cur_formula(cur_measure);
        if (cur_measure >= 2047)
        {
            break;
        }        
    }
    
    disable_bias_voltage();
    disable_current_measurement_mode();
}