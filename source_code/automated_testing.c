/*
 * automated_testing.c
 *
 * Created: 23/07/2015 19:48:22
 *  Author: limpkin
 */
 #include <avr/interrupt.h>
 #include <avr/pgmspace.h>
 #include <avr/eeprom.h>
 #include <util/delay.h>
 #include <avr/io.h>
 #include <stdio.h>
 #include "automated_testing.h"
 #include "eeprom_addresses.h"
 #include "measurement.h"
 #include "calibration.h"
 #include "meas_io.h"
 #include "vbias.h"
 #include "main.h"
 #include "dac.h"
 #include "adc.h"


/*
 * Automate vbias testing
 */
void automated_vbias_testing(void)
{
    char received_char = 0;
    uint8_t restart;
    
    //set_measurement_mode_io(RES_10K);
    
    while(1)
    {
        restart = FALSE;
        calibrate_single_ended_offset();
        for (uint16_t added_v = 0; (added_v < 100) && (restart == FALSE); added_v++)
        {
            enable_bias_voltage(VBIAS_MIN_V);
            for (uint16_t main_v = VBIAS_MIN_V; (main_v <= get_max_vbias_voltage()) && (restart == FALSE); main_v+=100)
            {
                update_bias_voltage(main_v + added_v);
                printf("%d\r\n", main_v + added_v);
                // Wait for input from the script
                scanf("%c", &received_char);
                if (received_char == '!')
                {
                    restart = TRUE;
                }
            }
            disable_bias_voltage();
        }        
    }
    
    //disable_measurement_mode_io();    
}

/*
 * Ramp voltage and measure the current
 */
void automated_current_testing(void)
{
    char received_char = 0;
    uint16_t cur_measure;
    uint8_t restart;
    
    set_current_measurement_ampl(CUR_MES_1X);
    setup_vbias_dac(VBIAS_MIN_DAC_VAL);
    enable_ldo();
    
    while (1)
    {
        restart = FALSE;
        for (uint16_t dac_val = VBIAS_MIN_DAC_VAL; (dac_val != 0) && (restart == FALSE); dac_val--)
        {
            update_vbias_dac(dac_val);
            _delay_us(10);
            cur_measure = quiescent_cur_measurement_loop(15);
            printf("%d|%d\r\n", dac_val, cur_measure);
            // Wait for input from the script
            scanf("%c", &received_char);
            if (received_char == '!')
            {
                restart = TRUE;
            }
        }
    }
    
    disable_bias_voltage();
    disable_current_measurement_mode();
}