/*
 *  usb.h
 *  Author: Miguel
 */


#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

/* Includes */
#include "usb_types.h"
#include <avr/pgmspace.h>

/* Defines and Macros */
#define MSB(x)  ( (x >> 8) & 0xFFu)
#define LSB(x)  ( (x)      & 0xFFu)
#define USB_DESCRIPTOR_LIST_LENGTH (6u)

/* Configurable Defines */
#define VID     (0x1209u)
#define PID     (0x0001u)

#define STR_MANUFACTURER    L"ManufacturerStr"      // Manufacturer string
#define STR_PRODUCT         L"ProductStr"           // Product string

/* Descriptors */
extern USB_Descriptor_List_T descriptor_list[USB_DESCRIPTOR_LIST_LENGTH];

#endif