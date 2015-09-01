/*
 *  usb_types
 *  Author: Miguel
 */
/*
 * Data types used in usb communication protocol
 */
#ifndef _USB_TYPES_H_
#define _USB_TYPES_H_

#include <stdio.h>

    #define ATTR_PACKED __attribute__ ((packed))

 /* Token Packets */
    /* Setup - Used to begin control transfers.*/
    typedef struct USB_Request_Header
    {
        uint8_t  bmRequestType; /**< Type of the request. */
        uint8_t  bRequest; /**< Request command code. */
        uint16_t wValue; /**< wValue parameter of the request. */
        uint16_t wIndex; /**< wIndex parameter of the request. */
        uint16_t wLength; /**< Length of the data to transfer in bytes. */
    } ATTR_PACKED USB_Request_Header_t;

    /* RequestType Definition */
    #define CONTROL_REQTYPE_DIRECTION   (0x80)
        #define REQDIR_HOSTTODEVICE     (0 << 7)
        #define REQDIR_DEVICETOHOST     (1 << 7)

    #define CONTROL_REQTYPE_TYPE        (0x60)
        #define REQTYPE_STANDARD        (0 << 5)
        #define REQTYPE_CLASS           (1 << 5)
        #define REQTYPE_VENDOR          (2 << 5)

    #define CONTROL_REQTYPE_RECIPIENT   (0x1F)
        #define REQREC_DEVICE           (0 << 0)
        #define REQREC_INTERFACE        (1 << 0)
        #define REQREC_ENDPOINT         (2 << 0)
        #define REQREC_OTHER            (3 << 0)

/* REQTYPE_STANDARD */
    /* Host to Device Request Definition */
    #define CLEAR_FEATURE               (0x01)
    #define SET_FEATURE                 (0x03)
    #define SET_ADDRESS                 (0x05)
    #define SET_DESCRIPTOR              (0x07)
    #define SET_CONFIGURATION           (0x09)

    /* Device to Host Request Definition */
    #define GET_STATUS                  (0x00)
    #define GET_DESCRIPTOR              (0x06)
    #define GET_CONFIGURATION           (0x08)

/* REQTYPE_CLASS */
    // HID (human interface device)
    #define HID_GET_REPORT              (0x01)
    #define HID_GET_IDLE                (0x02)
    #define HID_GET_PROTOCOL            (0x03)
    #define HID_SET_REPORT              (0x09)
    #define HID_SET_IDLE                (0x10)
    #define HID_SET_PROTOCOL            (0x11)

    /* Device descriptor List and Strings */
    typedef struct USB_Descriptor_List
    {
        /**
         * High byte - Descriptor Type
         * Low byte  - Index
         */
        uint16_t        wValue;
        /* Language ID - usually 0 */
        uint16_t        wIndex;
        /* Pointer to descriptor data */
        const uint8_t*  addr;
        /* Length of the data to return */
        uint8_t         length;
    }USB_Descriptor_List_T;


    typedef struct USB_SrtingDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        int16_t wString[];
    }USB_SrtingDescriptor_T;

#endif /* _USB_TYPES_H_ */
