/*
 * utils.c
 *
 * Created: 01/07/2015 23:19:16
 *  Author: limpkin
 */
 #include <avr/pgmspace.h>
 #include <string.h>
 #include <avr/io.h>
 #include <stdio.h>
 #include "defines.h"


/*! \brief Function for GCC to read out calibration byte.
 *  \param index The index to the calibration byte.
 *  \return Calibration byte.
 */
uint8_t ReadCalibrationByte(uint8_t index)
{
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return result;
}

/*! \brief Check a value's range
 *  \param val  The value
 *  \param min  The min
 *  \param max  The max
 *  \return TRUE or FALSE
 */
uint8_t check_value_range(uint16_t val, uint16_t min, uint16_t max)
{
    if ((val < min) || (val > max))
    {
        return FALSE;
    } 
    else
    {
        return TRUE;
    }    
}

/*! \brief Check a value's range
 *  \param val  The value
 *  \param min  The min
 *  \param max  The max
 *  \return TRUE or FALSE
 */
uint8_t check_value_range_uint32(uint32_t val, uint32_t min, uint32_t max)
{
    if ((val < min) || (val > max))
    {
        return FALSE;
    } 
    else
    {
        return TRUE;
    }    
}