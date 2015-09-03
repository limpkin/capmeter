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

// Usb printf
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

// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
// Configuration descriptor
#define RAWHID_INTERFACE    0                   // Interface for the raw HID
#define RAWHID_RX_ENDPOINT  1                   // Raw HID RX endpoint
#define RAWHID_TX_ENDPOINT  2                   // Raw HID TX endpoint
#define RAWHID_TX_SIZE      64                  // Raw HID transmit packet size
#define RAWHID_RX_SIZE      64                  // Raw HID receive packet size
#define RAWHID_RX_INTERVAL  10                  // RX interval
#define RAWHID_TX_INTERVAL  10                  // TX interval
#define RAWHID_USAGE_PAGE   0xFF31              // HID usage page, after 0xFF00: vendor-defined
#define RAWHID_USAGE        0x0074              // HID usage

/* USB CONFIGURATIION DEFINES */
#define USB_MAXEP               (2u)
//#define USB_FRAMENUM_ENABLE     (0u)
//#define USB_FIFO_ENABLE         (0u)

/* EP0 configuration */
#define EP0SIZE                 (64u)
#define EP0CTRL                 (USB_EP_TYPE_CONTROL_gc | \
                                 USB_EP_BUFSIZE_64_gc       )

#define EP1SIZE_OUT             (64u)
#define EP1CTRL_OUT             (USB_EP_TYPE_BULK_gc | \
                                 USB_EP_BUFSIZE_64_gc)
#define EP2SIZE_IN              (64u)
#define EP2CTRL_IN              (USB_EP_TYPE_BULK_gc | \
                                 USB_EP_BUFSIZE_64_gc)

/* USB Calibration Offsets */
#define USBCAL0_offset         (0x1A)
#define USBCAL1_offset         (0x1B)

/// From Atmel: Macros for XMEGA instructions not yet supported by the toolchain
// Load and Clear 
#ifdef __GNUC__
#define LACR16(addr,msk) \
	__asm__ __volatile__ ( \
	"ldi r16, %1" "\n\t" \
	".dc.w 0x9306" "\n\t"\
	::"z" (addr), "M" (msk):"r16")
#else
	#define LACR16(addr,msk) __lac((unsigned char)msk,(unsigned char*)addr)
#endif

#endif
