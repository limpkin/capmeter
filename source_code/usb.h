/*
 *  usb.h
 *  Author: Miguel
 */


#ifndef USB_H_
#define USB_H_

/* Function prototypes */
void USB_Init(void);
void USB_Task(void);

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
