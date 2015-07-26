/*
 * conversions.c
 *
 * Created: 25/07/2015 22:09:40
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include "measurement.h"
#include "conversions.h"
#include "calibration.h"
#include "meas_io.h"
#include "adc.h"


/*
 * Compute real vbias value from adc value
 * @param   adc_val     ADC value
 * @return  Actual mV value
 */
uint16_t compute_vbias_for_adc_value(uint16_t adc_val)
{    
    /******************* MATHS *******************/
    // Vadc = Vbias * 1.2 / (1.2+15)
    // Vadc = Vbias * 1.2 / 16.2
    // Vbias = Vadc * 16.2 / 1.2
    // Vbias(mV) = VALadc * Vref / 4095 * 16.2 / 1.2
    // Vbias(mV) = VALadc * 1240 / 4095 * 16.2 / 1.2
    // Vbias(mV) = VALadc * 20088 / 4914
    // Vbias(mV) = VALadc * 4,0879120879120879120879120879121
    // Vbias(mV) = VALadc * (4 + 0,0879120879120879120879120879121)
    // Vbias(mV) = VALadc * 4 + VALadc * 16 / 182
    /******************* INTERVAL ARITHMETIC *******************/
    // 0.1% resistors
    // Vadc = Vbias * [1198.8,1201.2] / ([1198.8,1201.2]+[14985,15015])
    // Vbias = Vadc * ([1198.8,1201.2]+[14985,15015]) / [1198.8,1201.2]
    // Vbias = Vadc * ([1198.8,1201.2]+[14985,15015]) * [1/1201.2, 1/1198.8] (for same resistor)
    // Multiplication: [x1,x2]*[y2,y1] = [min(x1y1, x1y2, x2y1, x2y2), max(x1y1, x1y2, x2y1, x2y2)]
    // Vbias = Vadc * [13.475, 13.525]
    // 1.24V 0.25% voltage reference
    // Vbias = VALadc * [1243.1, 1236.9] * [13.475, 13.525] / 4095
    // Vbias = VALadc * [16667,2275, 16812,9275] / 4095
    // Vbias = VALadc * 16740,0775 (+-0.435%) / 4095
    // Taking into account a +-3lsb INL
    // Vbias = VALadc * 16740.0775 (+-0.435%) / 4095 +- 3 * 16740.0775 (+-0.435%) / 4095
    // Vbias = VALadc * 16740.0775 (+-0.435%) / 4095 +- 12mV (+-0.435%)
    
    uint16_t return_value = (adc_val * 16 / 182);
    return_value += (adc_val * 4);
    return return_value;
}

/*
 * Compute current measurement numerator from adc value (den is the ampl)
 * @param   adc_val     ADC value
 * @return  numerator
 * @note    Only precise at 0.118%
 */
uint16_t compute_cur_mes_numerator_from_adc_val(uint16_t adc_val)
{    
    /******************* MATHS *******************/    
    // Vadc = I(A) * 1k * 100 * ampl
    // Vadc = I(A) * 100k * ampl
    // I(A) = Vadc / (100k * ampl)
    // I(A) = Val(ADC) * (1.24 / 2047) / (100k * ampl)
    // I(A) = Val(ADC) * 1.24 / (204.7M * ampl)
    // I(nA) = Val(ADC) * 1.24 / (0.2047 * ampl)
    // I(nA) = Val(ADC) * 6,057645335 / ampl
    // I(nA) = Val(ADC)*5 + Val(ADC)*1,057645335 / ampl
    // I(nA) ~ Val(ADC)*5 + Val(ADC)*(18/17) / ampl
    
    uint16_t return_value = (adc_val * 18) / 17;
    return_value += (adc_val * 5);
    return return_value;
}

/*
 * Compute voltage from single ended adc measurement
 * @param   adc_val     ADC value
 * @return  the voltage
 * @note    Only precise at 0.07%
 */
uint16_t compute_voltage_from_se_adc_val(uint16_t adc_val)
{    
    /******************* MATHS *******************/    
    // Vadc = Val(ADC) * (1.24 / 4095)
    // Vadc(mV) ~ Val(ADC) * 10/33
    
    return (adc_val * 10)/33;
}

/*
 * Compute voltage from single ended adc measurement when vref selected is avcc/2
 * @param   adc_val     ADC value
 * @return  the voltage
 * @note    Only precise at 0.013%
 */
uint16_t compute_voltage_from_se_adc_val_with_avcc_div2_ref(uint16_t adc_val)
{    
    /******************* MATHS *******************/    
    // Vadc = Val(ADC) * (3.3 / 2*4095)
    // Vadc(mV) ~ Val(ADC) * 27/67
    
    uint16_t temp_val = adc_val * 13 / 67;
    adc_val = adc_val * 14 / 67;    
    return temp_val + adc_val;
}

/*
 * Compute voltage from single ended adc measurement when vref selected is avcc/1.6
 * @param   adc_val     ADC value
 * @return  the voltage
 * @note    Only precise at 0.73%
 */
uint16_t compute_voltage_from_se_adc_val_with_avcc_div16_ref(uint16_t adc_val)
{    
    /******************* MATHS *******************/    
    // Vadc = Val(ADC) * (3.3 / 1.6*4095)
    // Vadc(mV) ~ Val(ADC) / 2
    
    return adc_val/2;
}

/*
 * Get value for freq measurement define
 * @param   define  The define
 * @return  the value
 */
uint16_t get_val_for_freq_define(uint16_t define)
{
    switch(define)
    {
        case FREQ_1HZ:      return 1;
        case FREQ_32HZ:     return 32;
        case FREQ_64HZ:     return 64;
        case FREQ_128HZ:    return 128;
    }
    return 0;
}

/*
 * Get value for counter divider
 * @param   divider  The divider
 * @return  the value
 */
uint16_t get_val_for_counter_divider(uint8_t divider)
{
    switch(divider)
    {
        case TC_CLKSEL_DIV1_gc : return 1;
        case TC_CLKSEL_DIV2_gc : return 2;
        case TC_CLKSEL_DIV4_gc : return 4;
        case TC_CLKSEL_DIV8_gc : return 8;
        case TC_CLKSEL_DIV64_gc : return 64;
        case TC_CLKSEL_DIV256_gc : return 256;
        case TC_CLKSEL_DIV1024_gc : return 1024;
    }
    return 0;
}

/*
 * Get value for res mux measurement define
 * @param   define  The define
 * @return  the value divided by two
 */
uint16_t get_half_val_for_res_mux_define(uint16_t define)
{
    switch(define)
    {
        case RES_270:       return 135;
        case RES_100K:      return 50000;
        case RES_1K:        return 500;
        case RES_10K:       return 5000;
    }
    return 0;
}

/*
 * Print the formula to compute the capacitance
 */

/*
 * Print the formula to compute the capacitance
 * @param   aggregate           Aggregate rise/fall times
 * @param   counter             Number of rise/falls
 * @param   counter_divider     Counter divider used for TC
 * @param   res_mux             Resistor used
 */
void print_compute_c_formula(uint32_t aggregate, uint32_t counter, uint16_t counter_divider, uint8_t res_mux)
{
    // C = T / 2 * half_r * (ln(3300-Vl/3300-vh) + ln(vh/vl))
    // C = counter divider * aggregate / 32M * counter * 2 * half_r * (ln(3300-Vl/3300-vh) + ln(vh/vl))
    convdprintf("(%u * %lu) / (32000000 * %lu * 2 * %u * (ln((3300-%u)/(3300-%u)) + ln(%u/%u)))\r\n", get_val_for_counter_divider(counter_divider), aggregate, counter, get_half_val_for_res_mux_define(res_mux), compute_voltage_from_se_adc_val(get_calib_first_thres_down()), compute_voltage_from_se_adc_val(get_calib_second_thres_down()), compute_voltage_from_se_adc_val(get_calib_second_thres_down()), compute_voltage_from_se_adc_val(get_calib_first_thres_down()));
}

/*
 * Print current computation formula
 * @param   adc_val     Current ADC val for current measurement
 */
void print_compute_cur_formula(uint16_t adc_val)
{    
    if (adc_val >= 2047)
    {
        convdprintf_P(PSTR("ADC val too high, measurement invalid\r\n"));
        return;
    }
    /******************* MATHS *******************/
    // Vadc = I(A) * 1k * 100 * ampl
    // Vadc = I(A) * 100k * ampl
    // I(A) = Vadc / (100k * ampl)
    // I(A) = Val(ADC) * (1.24 / 2047) / (100k * ampl)
    // I(A) = Val(ADC) * 1.24 / (204.7M * ampl)
    // I(nA) = Val(ADC) * 1.24 / (0.2047 * ampl)
    convdprintf("Measured current in nA: %u * 1.24 / (0.2047 * %u)\r\n", adc_val, 1 << get_configured_adc_ampl()); 
}