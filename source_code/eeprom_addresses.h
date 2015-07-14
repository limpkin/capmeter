/*
 * eeprom_addresses.h
 *
 * Created: 01/07/2015 16:07:03
 *  Author: limpkin
 */ 


#ifndef EEPROM_ADDRESSES_H_
#define EEPROM_ADDRESSES_H_

// Bool define
#define EEPROM_BOOL_OK_VAL      0xBA

// Addresses
#define CALIB_DIFF_OFFS_VAL_START   0       // 0nA offsets for different amplifications
#define CALIB_DIFF_OFFS_DATA_BOOL   14      // Boolean indicating storage
#define CALIB_THRESHOLD_BOOL        15      // Boolean indicating thresholds storage
#define CALIB_VUP                   16      // Opamp vup
#define CALIB_VDOWN                 18      // Opamp vlow
#define CALIB_CUR_GAIN_VBIAS        20      // Vbias values for current measurements gain correction computation
#define CALIB_CUR_GAIN_ADC_VAL      34      // Current ADC values for current measurements gain correction computation
#define CALIB_CUR_OFFS_VAL_START    48      // Offsets for current measurements
#define CALIB_CUR_DATA_BOOL         62      // Boolean indicating current calibration values storage
#define CALIB_FIRST_THRES_DOWN      64      // First threshold level, ramping up
#define CALIB_FIRST_THRES_UP        66      // First threshold level, ramping down
#define CALIB_SECOND_THRES_DOWN     68      // Second threshold level, ramping up
#define CALIB_SECOND_THRES_UP       70      // Second threshold level, ramping down

#endif /* EEPROM_ADDRESSES_H_ */