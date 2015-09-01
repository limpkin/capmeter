/*
 *  usb.h
 *  Author: Miguel
 */


#ifndef USB_H_
#define USB_H_

/* Function prototypes */
void init_usb(void);
void USB_Task(void);

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
#define USB_FRAMENUM_ENABLE     (0u)
#define USB_FIFO_ENABLE         (0u)

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

#endif
