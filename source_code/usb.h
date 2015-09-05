/*
 *  usb.h
 *  Author: Miguel
 */
#ifndef USB_H_
#define USB_H_

#include <avr/io.h>
#include "defines.h"
#include "printf_override.h"

/* Function prototypes */
void init_usb(void);
void usb_send_data(uint8_t* data);
uint8_t usb_receive_data(uint8_t* data);

// USB printf
#ifdef USB_PRINTF
    #define usbdprintf   printf
    #define usbdprintf_P printf_P
#else
    #define usbdprintf
    #define usbdprintf_P
#endif

// Typedefs
typedef union USB_EP_pair
{
    union
    {
        struct
        {
            USB_EP_t out;
            USB_EP_t in;
        };
        USB_EP_t ep[2];
    };
} __attribute__ ((packed)) USB_EP_pair_t;

// Configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
#define RAWHID_INTERFACE    0                   // Interface for the raw HID
#define RAWHID_RX_ENDPOINT  1                   // Raw HID RX endpoint
#define RAWHID_TX_ENDPOINT  2                   // Raw HID TX endpoint
#define RAWHID_TX_SIZE      64                  // Raw HID transmit packet size
#define RAWHID_RX_SIZE      64                  // Raw HID receive packet size
#define RAWHID_RX_INTERVAL  10                  // RX interval
#define RAWHID_TX_INTERVAL  10                  // TX interval
#define RAWHID_USAGE_PAGE   0xFF31              // HID usage page, after 0xFF00: vendor-defined
#define RAWHID_USAGE        0x0074              // HID usage
#define RAWHID_EP0_SIZE     64                  // Endpoint 0 size

// Command IDs defines
#define CMD_DEBUG           0x00
#define CMD_PING            0x01
#define CMD_VERSION         0x02

/* USB Calibration Offsets */
#define USBCAL0_offset      (0x1A)
#define USBCAL1_offset      (0x1B)

#endif
