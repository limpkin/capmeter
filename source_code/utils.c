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