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
// Current ADC current values for gain computation
uint16_t adc_cur_values_for_gain_correction[7];
// Vbias values for gain computation
uint16_t vbias_for_gain_correction[7];
// Opamp 0v output voltages at vbias 3.3v depending on resistance
uint16_t opamp_0v_outputs_for_3v[4];
// Opamp 0v output voltage when no load attached to vout
uint16_t opamp_0v_output_no_load;
// Calibration 0V value for single ended measurement
uint16_t calib_0v_value_se = 0;
// 0nA ADC values for different amplifications
int16_t zero_na_outputs[7];
// Calibrated vlow
uint16_t calib_vlow;
// Calibrated vup
uint16_t calib_vup;


/*
 * Get ADC value for single ended offset
 * @return  ADC value
 */
uint16_t get_single_ended_offset(void)
{
    return calib_0v_value_se;
}

/*
 * Get ADC value for differential offset
 * @param   ampl    Selected amplification
 * @return  ADC value
 */
int16_t get_differential_offset_for_ampl(uint8_t ampl)
{
    return zero_na_outputs[ampl];
}

/*
 * Get DAC value for opamp vlow
 * @return  DAC value
 */
uint16_t get_calib_vlow(void)
{
    return calib_vlow;
}

/*
 * Get DAC value for opamp vup
 * @return  DAC value
 */
uint16_t get_calib_vup(void)
{
    return calib_vup;
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
    
    // Fetch differential calibration data if stored
    if (eeprom_read_byte((uint8_t*)CALIB_DIFF_OFFS_DATA_BOOL) == EEPROM_BOOL_OK_VAL)
    {
        eeprom_read_block((void*)zero_na_outputs, (void*)CALIB_DIFF_OFFS_VAL_START, sizeof(zero_na_outputs));
        #ifdef CALIB_PRINTF
            calibprintf_P(PSTR("Differential calibration values fetched from EEPROM\r\n"));
            for (uint8_t i = 0; i <= CUR_MES_64X; i++)
            {
                calibprintf("For ampl %u : %u\r\n", 1 << i, zero_na_outputs[i]);
            }
        #endif
    }
    
    // Fetch vup vlow calibration data if stored
    if (eeprom_read_byte((uint8_t*)CALIB_VUP_VLOW_BOOL) == EEPROM_BOOL_OK_VAL)
    {
        calib_vup = eeprom_read_word((uint16_t*)CALIB_VUP);
        calib_vlow = eeprom_read_word((uint16_t*)CALIB_VDOWN);
        #ifdef CALIB_PRINTF
            calibprintf_P(PSTR("Vup/Vlow calibration values fetched from EEPROM\r\n"));
            calibprintf("Vup: %u\r\n", calib_vup);
            calibprintf("Vlow: %u\r\n", calib_vlow);
        #endif
    }
}

void current_measurement_calibration(void)
{
    // Calibration Methodology
    //
    // 1) Adjust Vbias to get exactly one LSB for all amplifications
    // X1:  1LSB = 6,057645335nA
    // X2:  1LSB = 3,02734375nA
    // X4:  1LSB = 1,513671875nA
    // X8:  1LSB = 0,7568359375nA
    // X16: 1LSB = 0,37841796875nA
    // X32: 1LSB = 0,189208984375nA
    // X64: 1LSB = 0,0946044921875nA
    // >> Chosen Resistor until X16: 2Gohms
    //
    // 2) Adjust Vbias to get exactly the max ADC value
    // X1:  1LSB = 12393,9453125nA
    // X2:  1LSB = 6196,97265625nA
    // X4:  1LSB = 3098,486328125nA
    // X8:  1LSB = 1549,2431640625nA
    // X16: 1LSB = 774,62158203125nA
    // X32: 1LSB = 387,310791015625nA
    // X64: 1LSB = 193,6553955078125nA
    // >> Chosen Resistor until X16: 1Mohms
    //
    //
    // Measured from the ADC for current:
    // I(nA) = Val(ADC) * 6,057645335 / ampl
    //
    // Measured from the ADC for precision resistor and vbias
    // I(A) = Vbias / (1.01M)
    // I(nA) = Vbias * 1000 / 1.01
    // I(nA) = (VALadc2 * 4 + VALadc2 * 16 / 182) / 1.01
    //
    // Correcting for gain on ADC for current:
    // VAL(ADCcur) * 6,0546875 * X / ampl = (VAL(ADCbias) * 4 + VAL(ADCbias) * 16 / 182) / 1.01
    // X = (VAL(ADCbias) * 4 + VAL(ADCbias) * 16 / 182) * ampl / (1.01 * VAL(ADCcur) * 6,057645335)
    // X = compute_vbias_for_adc_value(VAL(ADCbias)) * ampl / (1.01 * compute_cur_mes_numerator_from_adc_val(VAL(ADCcur)))
    
    calibprintf_P(PSTR("-----------------------\r\n"));
    calibprintf_P(PSTR("Advanced current calibration\r\n"));
    
    uint16_t cur_measure;                                   // Var containing our measured current
    enable_bias_voltage(VBIAS_MIN_V);                       // Enable bias voltage at its minimum
    uint16_t dac_val = VBIAS_MIN_DAC_VAL;                   // Dac val for minimum bias voltage
    uint8_t current_cur_mes_mode = CUR_MES_16X;             // Start with max ampl
    set_current_measurement_ampl(current_cur_mes_mode);     // Set meas mode
    
    while(dac_val >= DAC_MIN_VAL)
    {
        // Start increasing the voltage
        update_vbias_dac(--dac_val);
        _delay_us(50);
        // Measure the current
        cur_measure = get_averaged_adc_value(13);
        // Check if we reached the max val
        if (cur_measure >= get_max_value_for_diff_channel(current_cur_mes_mode))
        {
            // Set ADC channel to vbias measurement
            configure_adc_channel(ADC_CHANNEL_VBIAS, 0, TRUE);
            // Compute vbias
            uint16_t measured_vbias = compute_vbias_for_adc_value(get_averaged_stabilized_adc_value(14, 12 + (CUR_MES_16X - current_cur_mes_mode), FALSE));
            // Stored measured current
            adc_cur_values_for_gain_correction[current_cur_mes_mode] = cur_measure;
            // Store measured vbias
            vbias_for_gain_correction[current_cur_mes_mode] = measured_vbias;
            
            #ifdef CALIB_PRINTF
                calibprintf("Found max value for ampl %u at dac value %u\r\n", 1 << current_cur_mes_mode, dac_val);
                calibprintf("Quiescent ADC value: %u, approx %u/%unA at approx %umV\r\n", cur_measure, compute_cur_mes_numerator_from_adc_val(cur_measure), 1 << current_cur_mes_mode, measured_vbias);
                calibprintf("Additional gain factor: %u / (1.01 * %u)\r\n", measured_vbias * (1 << current_cur_mes_mode), compute_cur_mes_numerator_from_adc_val(cur_measure));
                _delay_ms(5000);
            #endif
            // Check if we are at the min amplification
            if (current_cur_mes_mode == CUR_MES_1X)
            {
                break;
            } 
            else
            {
                current_cur_mes_mode--;
                set_current_measurement_ampl(current_cur_mes_mode);
                // Accelerate voltage ramping
                uint16_t skip_steps;
                switch(current_cur_mes_mode)
                {
                    case CUR_MES_8X : skip_steps = 180; break;
                    case CUR_MES_4X : skip_steps = 340; break;
                    case CUR_MES_2X : skip_steps = 740; break;
                    case CUR_MES_1X : skip_steps = 1540; break;
                    default: break;
                }
                for (uint16_t i = 0; i < skip_steps; i++)
                {
                    update_vbias_dac(--dac_val);
                    _delay_us(200);
                    // Enable stepup when needed
                    if (dac_val == STEPUP_ACTIV_DAC_V)
                    {
                        enable_stepup();
                        _delay_ms(20);
                    }               
                }
            }
        }
        // Enable stepup when needed
        if (dac_val == STEPUP_ACTIV_DAC_V)
        {
            enable_stepup();
            _delay_ms(20);
        }
    }
    
    disable_bias_voltage();
}

/*
 * Measure the offset for single ended conversions
 */
void calibrate_single_ended_offset(void)
{    
    // Calibrate 0V, average on more than a 1/50Hz period
    calibprintf_P(PSTR("Measuring external 0V value, single ended...\r\n"));
    configure_adc_channel(ADC_CHANNEL_GND_EXT, 0, TRUE);
    calib_0v_value_se = get_averaged_stabilized_adc_value(12, 12, TRUE);
    calibprintf("0V ADC value: %u, approx %umV\r\n", calib_0v_value_se, compute_voltage_from_se_adc_val(calib_0v_value_se));    
}

/*
 * Find 0nA ADC values for different amplifications
 */
void calibrate_cur_mos_0nA(void)
{
    int16_t cur_0nA_val = INT16_MAX;
    uint8_t cur_ampl = CUR_MES_1X;
    
    calibprintf_P(PSTR("-----------------------\r\n"));
    calibprintf_P(PSTR("Measuring 0nA outputs\r\n"));
    
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
        if (cur_0nA_val <= (17*(1 << cur_ampl)))
        {
            // Only possible because of the ampl registers
            adcprintf("Zero quiescent current for ampl %u: %d, approx %u/%unA\r\n", 1 << cur_ampl, cur_0nA_val, compute_cur_mes_numerator_from_adc_val(cur_0nA_val), 1 << cur_ampl);
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
    eeprom_write_block((void*)zero_na_outputs, (void*)CALIB_DIFF_OFFS_VAL_START, sizeof(zero_na_outputs));
    eeprom_write_byte((void*)CALIB_DIFF_OFFS_DATA_BOOL, EEPROM_BOOL_OK_VAL);
    adcprintf(PSTR("Values stored in EEPROM\r\n"));
    
    disable_current_measurement_mode();
    disable_bias_voltage();
}

/*
 * Measure Vup/Vlow of our opamp
 */
void calibrate_vup_vlow(void)
{
    calibprintf_P(PSTR("-----------------------\r\n"));
    calibprintf_P(PSTR("Vup/Vlow calibration\r\n"));
    
    // Set bias voltage above vcc so Q1 comes into play if there's a cap between the terminals
    disable_feedback_mos();
    _delay_ms(10);
    enable_bias_voltage(4000);
    
    // Leave time for a possible cap to charge
    _delay_ms(1000);
    setup_opampin_dac(DAC_MIN_VAL);
    calib_vlow = DAC_MAX_VAL;
    calib_vup = DAC_MIN_VAL;
    
    // Ramp up, wait for toggle
    while((PORTA_IN & PIN6_bm) != 0)
    {
        update_opampin_dac(++calib_vup);
        _delay_us(10);
    }
    
    calibprintf("Vhigh found: %u, approx %umV\r\n", calib_vup, compute_voltage_from_se_adc_val(calib_vup));
    update_opampin_dac(calib_vlow);
    _delay_us(10);
        
    // Ramp low, wait for toggle
    while((PORTA_IN & PIN6_bm) == 0)
    {
        update_opampin_dac(--calib_vlow);
        _delay_us(10);
    }
    
    calibprintf("Vlow found: %u, approx %umV\r\n", calib_vlow, compute_voltage_from_se_adc_val(calib_vlow));
    
    eeprom_write_word((uint16_t*)CALIB_VUP, calib_vup);
    eeprom_write_word((uint16_t*)CALIB_VDOWN, calib_vlow);
    eeprom_write_byte((uint8_t*)CALIB_VUP_VLOW_BOOL, EEPROM_BOOL_OK_VAL);
    calibprintf_P(PSTR("Values stored in EEPROM\r\n"));
    
    disable_opampin_dac();
    disable_bias_voltage();
}

/*
 * Measure the opamp internal resistance
 */
void calibrate_opamp_internal_resistance(void)
{
    uint8_t cur_cal_res_mux = RES_270;    
    measdprintf_P(PSTR("-----------------------\r\n"));
    measdprintf_P(PSTR("Measuring opamp internal resistance\r\n"));
                
    configure_adc_channel(ADC_CHANNEL_COMPOUT, 0, TRUE);                                // Configure ADC to measure the opamp output
    disable_feedback_mos();                                                             // Disable feedback mosfet
    disable_res_mux();                                                                  // Disable resistor mux
    _delay_ms(10);                                                                      // Wait before test
    setup_opampin_dac(calib_vup+100);                                                   // Force opamp output to 0 by setting IN- above IN+
    opamp_0v_output_no_load = get_averaged_stabilized_adc_value(8, 8, FALSE);           // Run averaging on 128 samples, max 8 lsb peak to peak noise    
    measdprintf("Opamp 0v output: %u, approx %umV\r\n", opamp_0v_output_no_load, compute_voltage_from_se_adc_val(opamp_0v_output_no_load));
    
    // Vbias should be short with the other terminal here
    enable_bias_voltage(3300);                                                          // Enable vbias at 3.3v to know its resistance during oscillations!
    enable_res_mux(cur_cal_res_mux);                                                    // Try all resistors one by one
    configure_adc_channel(ADC_CHANNEL_COMPOUT, 0, TRUE);                                // Required as setting vbias uses the adc
    while (cur_cal_res_mux <= RES_10K)
    {
        opamp_0v_outputs_for_3v[cur_cal_res_mux] = get_averaged_stabilized_adc_value(7, 8, FALSE);  // Measure
        
        // Only pass through if the user shorted the terminals
        if (!((cur_cal_res_mux == RES_270) && (opamp_0v_outputs_for_3v[cur_cal_res_mux] < 300)) && !((cur_cal_res_mux == RES_1K) && (opamp_0v_outputs_for_3v[cur_cal_res_mux] < 80)))
        {
            measdprintf("Opamp 0v output at 3.3V: %u, approx %umV\r\n", opamp_0v_outputs_for_3v[cur_cal_res_mux], compute_voltage_from_se_adc_val(opamp_0v_outputs_for_3v[cur_cal_res_mux]));
            
            #ifdef MEAS_PRINTF
                uint16_t voltage_mv = compute_voltage_from_se_adc_val(opamp_0v_outputs_for_3v[cur_cal_res_mux]);
                if (cur_cal_res_mux == RES_270)
                {
                    measdprintf("Approx R of %umOhms\r\n", (voltage_mv*270) / (3300 - voltage_mv));
                }
                else if (cur_cal_res_mux == RES_1K)
                {
                    measdprintf("Approx R of %umOhms\r\n", (voltage_mv*1000) / (3300 - voltage_mv));
                }
            #endif
            
            enable_res_mux(++cur_cal_res_mux);
        }        
    }

    // We're done    
    disable_res_mux();
    disable_opampin_dac();
    disable_bias_voltage();
}