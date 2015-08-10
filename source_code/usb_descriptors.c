/*
 *  usb_descriptors.c
 *  Author: Miguel
 */

#include "usb.h"
#include "usb_descriptors.h"

// Device descriptor
const PROGMEM uint8_t device_descriptor[] =
{
    18,       // bLength
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

// configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
// Configuration descriptor
#define RAWHID_INTERFACE    0                   // Interface for the raw HID
#define RAWHID_TX_ENDPOINT  2                   // Raw HID TX endpoint
#define RAWHID_RX_ENDPOINT  1                   // Raw HID RX endpoint
#define RAWHID_TX_SIZE      64                  // Raw HID transmit packet size
#define RAWHID_RX_SIZE      64                  // Raw HID receive packet size
#define RAWHID_USAGE_PAGE   0xFF31              // HID usage page, after 0xFF00: vendor-defined
#define RAWHID_USAGE        0x0074              // HID usage

#define KEYBOARD_ENDPOINT   3                   // Endpoint number for keyboard
#define KEYBOARD_SIZE       8                   // Endpoint size for keyboard

#define CONFIG_DESC_SIZE        (9+9+9+7+7) //+9+9+7)
#define RAWHID_HID_DESC_OFFSET   (9+9)
#define KEYBOARD_HID_DESC_OFFSET (9+9+9+7+7+9)


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

// Keyboard HID descriptor, Keyboard Protocol 1, HID 1.11 spec, Appendix B, page 59-60
const uint8_t PROGMEM keyboard_hid_report_desc[] =
{
    0x05, 0x01,                         // Usage Page (Generic Desktop),
    0x09, 0x06,                         // Usage (Keyboard),
    0xA1, 0x01,                         // Collection (Application),
    0x75, 0x01,                         //   Report Size (1),
    0x95, 0x08,                         //   Report Count (8),
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0xE0,                         //   Usage Minimum (224),
    0x29, 0xE7,                         //   Usage Maximum (231),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0x01,                         //   Logical Maximum (1),
    0x81, 0x02,                         //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x08,                         //   Report Size (8),
    0x81, 0x03,                         //   Input (Constant),                 ;Reserved byte
    0x95, 0x05,                         //   Report Count (5),
    0x75, 0x01,                         //   Report Size (1),
    0x05, 0x08,                         //   Usage Page (LEDs),
    0x19, 0x01,                         //   Usage Minimum (1),
    0x29, 0x05,                         //   Usage Maximum (5),
    0x91, 0x02,                         //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,                         //   Report Count (1),
    0x75, 0x03,                         //   Report Size (3),
    0x91, 0x03,                         //   Output (Constant),                 ;LED report padding
    0x95, 0x06,                         //   Report Count (6),
    0x75, 0x08,                         //   Report Size (8),
    0x15, 0x00,                         //   Logical Minimum (0),
    0x25, 0x68,                         //   Logical Maximum(104),
    0x05, 0x07,                         //   Usage Page (Key Codes),
    0x19, 0x00,                         //   Usage Minimum (0),
    0x29, 0x68,                         //   Usage Maximum (104),
    0x81, 0x00,                         //   Input (Data, Array),
    0xc0                                // End Collection
};

// Configuration descriptor
const PROGMEM uint8_t config_descriptor[] =
{
    // configuration descriptor, USB spec 9.6.3, page 264-266, Table 9-10
    9,                                  // bLength;
    CONFIGURATION,                      // bDescriptorType;
    LSB(CONFIG_DESC_SIZE),              // wTotalLength
    MSB(CONFIG_DESC_SIZE),
    1,                                  // bNumInterfaces
    1,                                  // bConfigurationValue
    0,                                  // iConfiguration
    0b10000000,                         // bmAttributes NO-SELFPOWER, NO REMOTE WAKEUP
    50,                                 // bMaxPower 2mA * 50 = 100mA

    // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
    9,                                  // bLength
    INTERFACE,                          // bDescriptorType
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
    ENDPOINT,                           // bDescriptorType
    RAWHID_RX_ENDPOINT,                 // bEndpointAddress
    0x03,                               // bmAttributes (0x03=intr)
    RAWHID_RX_SIZE, 0,                  // wMaxPacketSize
    10,                                 // bInterval

    // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
    7,                                  // bLength
    ENDPOINT,                           // bDescriptorType
    RAWHID_TX_ENDPOINT | 0x80,          // bEndpointAddress
    0x03,                               // bmAttributes (0x03=intr)
    RAWHID_TX_SIZE, 0,                  // wMaxPacketSize
    10,                                 // bInterval
#if 0
    // interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
    9,                                  // bLength
    INTERFACE,                          // bNumEndpoints
    0x03,                               // bInterfaceClass (0x03 = HID)
    0x01,                               // bInterfaceSubClass (0x01 = Boot)
    0x01,                               // bInterfaceProtocol (0x01 = Keyboard)
    0,                                  // iInterface

    // HID interface descriptor, HID 1.11 spec, section 6.2.1
    9,                                  // bLength
    0x21,                               // bDescriptorType
    0x11, 0x01,                         // bcdHID
    0,                                  // bCountryCode
    1,                                  // bNumDescriptors
    0x22,                               // bDescriptorType
    sizeof(keyboard_hid_report_desc),   // wDescriptorLength
    0,

    // endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
    7,                                  // bLength
    5,                                  // bDescriptorType
    KEYBOARD_ENDPOINT | 0x80,           // bEndpointAddress
    0x03,                               // bmAttributes (0x03=intr)
    KEYBOARD_SIZE, 0,                   // wMaxPacketSize
    1                                   // bInterval
#endif
};


// USB strings
/* String0 is the one that tells the Language */
const PROGMEM USB_SrtingDescriptor_T String0 =
{
    4,
    STRING,
    {0x0409}
};
const PROGMEM USB_SrtingDescriptor_T ManufacturerStr =
{
    sizeof(STR_MANUFACTURER),
    STRING,
    STR_MANUFACTURER
};
const PROGMEM USB_SrtingDescriptor_T VendorStr =
{
    sizeof(STR_PRODUCT),
    STRING,
    STR_PRODUCT
};


USB_Descriptor_List_T descriptor_list[USB_DESCRIPTOR_LIST_LENGTH] =
{
    {(DEVICE << 8)+ 0,          0x0000, device_descriptor,                  sizeof(device_descriptor)},
    {(CONFIGURATION << 8)+ 0,   0x0000, config_descriptor,                  sizeof(config_descriptor)},
    {(0x22 << 8)+0,             0x0000, rawhid_hid_report_desc,             sizeof(rawhid_hid_report_desc)},
    {(STRING << 8)+ 0,          0x0409, (const uint8_t*)&String0,           4},
    {(STRING << 8)+ 1,          0x0409, (const uint8_t*)&ManufacturerStr,   sizeof(STR_MANUFACTURER)},
    {(STRING << 8)+ 2,          0x0409, (const uint8_t*)&VendorStr,         sizeof(STR_PRODUCT)}
};
