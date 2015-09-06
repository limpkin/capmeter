/*
 * calibration.h
 *
 * Created: 02/07/2015 15:55:23
 *  Author: limpkin
 */ 


#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include "defines.h"
#include "printf_override.h"

// Debug printf
#ifdef CALIB_PRINTF
    #define calibdprintf   printf
    #define calibdprintf_P printf_P
#else
    #define calibdprintf
    #define calibdprintf_P
#endif

// Defines
#define THRESHOLD_AVG_BIT_SHIFT     6

// Typedefs
typedef struct oe_calib_data_struct
{
    uint16_t cur_measurement_offsets[7];    // Current measurement offsets
    uint16_t calib_second_thres_down;       // Calibrated second threshold, ramping up
    uint16_t calib_first_thres_down;        // Calibrated first threshold, ramping up
    uint16_t calib_second_thres_up;         // Calibrated second threshold, ramping down
    uint16_t calib_first_thres_up;          // Calibrated first threshold, ramping down
    uint16_t calib_0v_value_vbias;          // Single ended measurement offset for vbias
    uint16_t calib_0v_value_se;             // Single ended measurement offset
    uint16_t calib_osc_low_v;               // Calibrated low voltage oscillator value
    uint16_t max_voltage;                   // Maximum voltage that can be generated
    uint8_t year;                           // Calibration data year
    uint8_t month;                          // Calibration data month
    uint8_t day;                            // Calibration data day
} oe_calib_data_t;

// Prototypes
void start_openended_calibration(uint8_t day, uint8_t month, uint8_t year);
uint16_t get_offset_for_current_measurement(uint8_t ampl);
uint16_t get_single_ended_offset(uint8_t current_channel);
uint8_t get_openended_calibration_data(uint8_t* buffer);
uint16_t get_calib_second_thres_down(void);
uint16_t get_calib_first_thres_down(void);
uint16_t get_calib_second_thres_up(void);
uint16_t get_calib_first_thres_up(void);
void delete_cur_measurement_offsets(void);
void calibrate_single_ended_offset(void);
void delete_single_ended_offset(void);
uint8_t is_platform_calibrated(void);
uint16_t get_max_vbias_voltage(void);
uint16_t get_calib_osc_low_v(void);
void init_calibration(void);

#endif /* CALIBRATION_H_ */