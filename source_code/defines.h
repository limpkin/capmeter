/*
 * defines.h
 *
 * Created: 25/04/2015 19:16:15
 *  Author: limpkin
 */ 


#ifndef DEFINES_H_
#define DEFINES_H_

// Val defines
#define FALSE           0
#define TRUE            (!FALSE)
#define CAPMETER_VER    "v0.1"

// enums
enum fw_mode_t     {MODE_IDLE, MODE_CURRENT_MES, MODE_CAP_MES};

// Typedefs
typedef void (*bootloader_f_ptr_type)(void);
typedef union usb_message
{
    union
    {
        struct
        {
            uint8_t length;
            uint8_t command_id;
            uint8_t payload[62];
        };
       uint8_t data[64];
    };
} __attribute__ ((packed)) usb_message_t;

// IO debug printf
#define IO_PRINTF
// Serial debug printf
#define SERIAL_PRINTF
// Measurement debug printf
#define MEAS_PRINTF
// Dac debug printf
#define DAC_PRINTF
// Adc debug printf
#define ADC_PRINTF
// Calib debug printf
#define CALIB_PRINTF
// Vbias debug printf
#define VBIAS_PRINTF
// Tests debug printf
#define TESTS_PRINTF
// Conversions debug printf
#define CONV_PRINTF
// Automated testing printf
#define AUTOMATED_TESTS_PRINTF
// USB printf
//#define USB_PRINTF

// Check if we need to enable printf
#if defined(USB_PRINTF) || defined(AUTOMATED_TESTS_PRINTF) || defined(CONV_PRINTF) || defined(TESTS_PRINTF) || defined(IO_PRINTF) || defined(VBIAS_PRINTF) || defined(CALIB_PRINTF) || defined(SERIAL_PRINTF) || defined(MEAS_PRINTF) || defined(DAC_PRINTF) || defined(ADC_PRINTF)
    #define PRINTF_ENABLED
#endif

#endif /* DEFINES_H_ */