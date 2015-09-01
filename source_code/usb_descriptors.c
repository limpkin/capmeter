/*
 *  usb_descriptors.c
 *  Author: Miguel
 */

#include "usb.h"
#include "usb_descriptors.h"

// Device descriptor
const PROGMEM uint8_t device_descriptor[] =
{
    18,                                 // bLength
    1,                                  // bDescriptorType
    0x00, 0x02,                         // bcdUSB, USB 1.0
    0,                                  // bDeviceClass, see interface descriptors
    0,                                  // bDeviceSubClass
    0,                                  // bDeviceProtocol
    EP0SIZE,                            // bMaxPacketSize0
    LSB(VID), MSB(VID),                 // idVendor
    LSB(PID), MSB(PID),                 // idProduct
    0x00, 0x01,                         // bcdDevice
    1,                                  // iManufacturer
    2,                                  // iProduct
    0,                                  // iSerialNumber
    1                                   // bNumConfigurations
};

// Raw HID descriptor
const uint8_t PROGMEM rawhid_hid_report_desc[] =
{
    0x06, LSB(RAWHID_USAGE_PAGE), MSB(RAWHID_USAGE_PAGE),
    0x0A, LSB(RAWHID_USAGE), MSB(RAWHID_USAGE),
    0xA1, 0x01,                         // Collection 0x01
    0x75, 0x08,                         // report size = 8 bits
    0x15, 0x00,                         // logical minimum = 0
    0x26, 0xFF, 0x00,                   // logical maximum = 255
    0x95, RAWHID_TX_SIZE,               // report count
    0x09, 0x01,                         // usage
    0x81, 0x02,                         // Input (array)
    0x95, RAWHID_RX_SIZE,               // report count
    0x09, 0x02,                         // usage
    0x91, 0x02,                         // Output (array)
    0xC0                                // end collection
};

#define CONFIG_DESC_SIZE        (9+9+9+7+7)
#define RAWHID_HID_DESC_OFFSET   (9+9)

// Configuration descriptor
const PROGMEM uint8_t config_descriptor[] =
{
    // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
    9,                                  // bLength;
    2,                                  // bDescriptorType;
    LSB(CONFIG_DESC_SIZE),              // wTotalLength
    MSB(CONFIG_DESC_SIZE),
    1,                                  // bNumInterfaces
    1,                                  // bConfigurationValue
    0,                                  // iConfiguration
    0b10000000,                         // bmAttributes NO-SELFPOWER, NO REMOTE WAKEUP
    50,                                 // bMaxPower 2mA * 50 = 100mA

    // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
    9,                                  // bLength
    4,                                  // bDescriptorType
    RAWHID_INTERFACE,                   // bInterfaceNumber
    0,                                  // bAlternateSetting
    2,                                  // bNumEndpoints
    0x03,                               // bInterfaceClass (0x03 = HID)
    0x00,                               // bInterfaceSubClass (0x01 = Boot)
    0x00,                               // bInterfaceProtocol (0x01 = Keyboard)
    0,                                  // iInterface

    // HID interface descriptor, HID 1.11 spec, section 6.2.1
    9,                                  // bLength
    0x21,                               // bDescriptorType
    0x11, 0x01,                         // bcdHID
    0,                                  // bCountryCode
    1,                                  // bNumDescriptors
    0x22,                               // bDescriptorType
    sizeof(rawhid_hid_report_desc),     // wDescriptorLength
    0,

    // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
    7,                                  // bLength
    5,                                  // bDescriptorType
    RAWHID_RX_ENDPOINT,                 // bEndpointAddress
    0x03,                               // bmAttributes (0x03=intr)
    RAWHID_RX_SIZE, 0,                  // wMaxPacketSize
    RAWHID_RX_INTERVAL,                 // bInterval

    // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
    7,                                  // bLength
    5,                                  // bDescriptorType
    RAWHID_TX_ENDPOINT | 0x80,          // bEndpointAddress
    0x03,                               // bmAttributes (0x03=intr)
    RAWHID_TX_SIZE, 0,                  // wMaxPacketSize
    RAWHID_TX_INTERVAL                  // bInterval
};


// USB strings
/* String0 is the one that tells the Language */
const PROGMEM USB_SrtingDescriptor_T String0 =
{
    4,
    3,
    {0x0409}
};
const PROGMEM USB_SrtingDescriptor_T ManufacturerStr =
{
    sizeof(STR_MANUFACTURER),
    3,
    STR_MANUFACTURER
};
const PROGMEM USB_SrtingDescriptor_T VendorStr =
{
    sizeof(STR_PRODUCT),
    3,
    STR_PRODUCT
};

USB_Descriptor_List_T descriptor_list[USB_DESCRIPTOR_LIST_LENGTH] =
{
    {0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
    {0x0200, 0x0000, config_descriptor, sizeof(config_descriptor)},
    {0x2200, RAWHID_INTERFACE, rawhid_hid_report_desc, sizeof(rawhid_hid_report_desc)},
    {0x2100, RAWHID_INTERFACE, config_descriptor+RAWHID_HID_DESC_OFFSET, 9},
    {0x0300, 0x0000, (const uint8_t*)&String0, 4},
    {0x0301, 0x0409, (const uint8_t*)&ManufacturerStr, sizeof(STR_MANUFACTURER)},
    {0x0302, 0x0409, (const uint8_t*)&VendorStr, sizeof(STR_PRODUCT)}
};
