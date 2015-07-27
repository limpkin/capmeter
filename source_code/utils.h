/*
 * utils.h
 *
 * Created: 01/07/2015 23:19:26
 *  Author: limpkin
 */ 


#ifndef UTILS_H_
#define UTILS_H_

#include "defines.h"
#include "printf_override.h"

uint8_t check_value_range_uint32(uint32_t val, uint32_t min, uint32_t max);
uint8_t check_value_range(uint16_t val, uint16_t min, uint16_t max);
uint8_t ReadCalibrationByte(uint8_t index);

#endif /* UTILS_H_ */