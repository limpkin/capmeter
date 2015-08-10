/*
 *  usb.c
 *  Author: Miguel
 */
/* Features:
 * 31 configurable endpoints and 1 control endpoint
 */

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>

#include "usb_types.h"
#include "usb_descriptors.h"
#include "usb.h"
#include "utils.h"

/* SRAM Memmory Mapping - XMEGA AU User Manual */
struct
{
    #if USB_FIFO_ENABLE
        uint8_t  fifo[(USB_MAXEP+1)*4];
    #endif
        USB_EP_t epCfgTable[2*(USB_MAXEP+1)];
    #if USB_FRAMENUM_ENABLE
        uint16_t frameNumber;
    #endif
} USB_SRAM __attribute__ ((aligned (2)));

/* Buffers where to store EP specific data */
volatile uint8_t ep0_out[EP0SIZE];
volatile uint8_t ep0_in[EP0SIZE];
volatile uint8_t ep1_out[EP1SIZE_OUT];
volatile uint8_t ep2_in[EP2SIZE_IN];

// Zero when we are not configured, non-zero when enumerated
static volatile uint8_t usb_configuration = 0;

/* Prototype functions */
void USB_sendDescriptor(uint16_t wValue, uint16_t wLength);
void USB_setAddress(uint8_t address);
void USB_getConfiguration(void);
void USB_setConfiguration(void);
void USB_doNothing(void);
void USB_HidGetReport(uint16_t wLength);

void USB_Init(void)
{
    /* START PLL to 48Mhz for USB full speed
     * Internal 2 MHz RC Oscillator
     * Multiplied by 24
     * */
    OSC_CTRL |= OSC_RC2MEN_bm;
    /* Wait until 2Mhz oscillator is stable */
    while( (OSC_STATUS & OSC_RC2MRDY_bm) == 0 );

    OSC_PLLCTRL = OSC_PLLSRC_RC2M_gc | (24u);
    OSC_CTRL |= OSC_PLLEN_bm;
    /* wait until pll is locked */
    while( (OSC_STATUS & OSC_PLLRDY_bm) == 0 );

    /* Select the 32kHz external clock as calibration ref for our 2M */
    OSC_DFLLCTRL |= OSC_RC2MCREF_XOSC32K_gc;
    DFLLRC2M_CTRL = DFLL_ENABLE_bm;

    /* CLOCK USB Control register
     * USB uses PLL
     * No preescaler
     * Enable USB clock
     */
    CLK_USBCTRL = CLK_USBSEN_bm;

    /* USB CTRLA:
     * Bit 7 – ENABLE: USB Enable
     * Bit 6 – SPEED: Speed Select
     * Bit 5 – FIFOEN: USB FIFO Enable
     * Bit 4 – STFRNUM: Store Frame Number Enable
     * Bit 3:0 – MAXEP[3:0]: Maximum Endpoint Address
     */
    USB_CTRLA = ( /*USB_ENABLE_bm  |*/
                  USB_SPEED_bm   |
#if USB_FIFO_ENABLE
                  USB_FIFOEN_bm  |
#endif
#if USB_FRAMENUM_ENABLE
                  USB_STFRNUM_bm |
#endif
                  USB_MAXEP   );

    /* USB_ADDR
     * Bit 7 – Reserved
     * Bit 6:0 – ADDR[6:0]: Device Address
     */
     /* Any device which is not yet assigned an address
      * must respond to packets sent to address zero
      */
    USB_ADDR = 0u;

    /* Configure ENDPOINT configuration table */
    USB_EPPTR = &USB_SRAM.epCfgTable[0];

    /* Endpoint 0 configuration */
    /* EP0 Out */
    USB_SRAM.epCfgTable[0].STATUS = 0u;
    USB_SRAM.epCfgTable[0].CTRL = EP0CTRL;
    USB_SRAM.epCfgTable[0].DATAPTR = &ep0_out;

    /* EP0 In */
    USB_SRAM.epCfgTable[1].STATUS = 0u;
    USB_SRAM.epCfgTable[1].CTRL = EP0CTRL;
    USB_SRAM.epCfgTable[1].DATAPTR = &ep0_in;

    /* USB CTRLB:
     * Bit 7:5 – Reserved
     * Bit 4 – PULLRST: Pull during Reset
     * Bit 3 – Reserved
     * Bit 2 – RWAKEUP: Remote Wake-up
     * Bit 1 – GNACK: Global NACK
     * Bit 0 – ATTACH: Attach
     */
    USB_CTRLB = USB_ATTACH_bm;

    /* USB interrupt enable */
    // Enable USB interrupts
    USB_INTCTRLA = USB_BUSEVIE_bm | USB_INTLVL_MED_gc;
    USB_INTCTRLB = USB_TRNIE_bm | USB_SETUPIE_bm;



    /* USB pins factory calibration apply */
    USB.CAL0 = ReadCalibrationByte(PROD_SIGNATURES_START+USBCAL0_offset);
    USB.CAL1 = ReadCalibrationByte(PROD_SIGNATURES_START+USBCAL1_offset);


    USB_CTRLA |= USB_ENABLE_bm;
}

ISR(USB_TRNCOMPL_vect)
{
    USB_INTFLAGSBCLR = USB_SETUPIF_bm | USB_TRNIF_bm;
    USB_INTFLAGSACLR = USB_INTFLAGSACLR;
    USB_Task();
}

/* SOF, suspend, resume, reset bus event interrupts, crc, underflow, overflow and stall error interrupts */
ISR(USB_BUSEVENT_vect)
{
    if (USB_INTFLAGSACLR & USB_SOFIF_bm){
        USB_INTFLAGSACLR = USB_SOFIF_bm;
    }else if (USB_INTFLAGSACLR & (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm)){
        USB_INTFLAGSACLR = (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm);
    }else if (USB_INTFLAGSACLR & USB_STALLIF_bm){
        USB_INTFLAGSACLR = USB_STALLIF_bm;
    }else{
        USB_INTFLAGSACLR = USB_SUSPENDIF_bm | USB_RESUMEIF_bm | USB_RSTIF_bm;
    }
}

void USB_Task(void)
{

    // Read once to prevent race condition where SETUP packet is interpreted as OUT
    uint8_t status = USB_SRAM.epCfgTable[0].STATUS;
    USB_Request_Header_t* usbMsg = USB_SRAM.epCfgTable[0].DATAPTR;


    USB_SRAM.epCfgTable[0].STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL1_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);
    USB_SRAM.epCfgTable[0].AUXDATA = 0;
    USB_SRAM.epCfgTable[0].CNT = 0;

    if( (usbMsg->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_STANDARD)
    {
        /* HOST to DEVICE */
        if( (usbMsg->bmRequestType & CONTROL_REQTYPE_DIRECTION) == REQDIR_HOSTTODEVICE)
        {
            switch(usbMsg->bRequest)
            {
                case CLEAR_FEATURE    :
                    USB_doNothing();
                    break;
                case SET_FEATURE      :
                    USB_doNothing();
                    break;
                case SET_ADDRESS      :
                    printf("Set address -> %02X", usbMsg->wValue);
                    USB_setAddress(usbMsg->wValue);
                    break;
                case SET_DESCRIPTOR   :
                    USB_doNothing();
                    break;
                case SET_CONFIGURATION:
                    /* We only have one configuration */
                    USB_setConfiguration();
                    break;
            }
        }
        else /* DEVICE to HOST */
        {
            switch(usbMsg->bRequest)
            {
                case GET_STATUS:
                    USB_doNothing();
                    break;
                case GET_DESCRIPTOR:
                    USB_sendDescriptor(usbMsg->wValue, usbMsg->wLength);
                    break;
                case GET_CONFIGURATION:
                    printf("Get Configuration\n");
                    USB_getConfiguration();
                    break;
                default:
                    USB_doNothing();
                    break;
            }
        }
    }
    else if( (usbMsg->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_CLASS)
    {
        /* HOST to DEVICE */
        if( (usbMsg->bmRequestType & CONTROL_REQTYPE_DIRECTION) == REQDIR_HOSTTODEVICE)
        {
            USB_doNothing();
        }
        else /* DEVICE to HOST */
        {
            switch(usbMsg->bRequest)
            {
                case (HID_GET_REPORT):
//                    USB_HidGetReport(usbMsg->wLength);
                      USB_doNothing();
                    break;
                default:
                    USB_doNothing();
                    break;
            }
        }
    }
}

/* Send Data throught the control endpoint */
void USB_sendDescriptor(uint16_t wValue, uint16_t wLength)
{
    uint8_t i;
    USB_EP_t *inEP0 = &USB_SRAM.epCfgTable[1];
    uint8_t *dataDst = inEP0->DATAPTR;
    const uint8_t *dataSrc = NULL;
    uint8_t dataLen = 0;

    for(i=0; i<(USB_DESCRIPTOR_LIST_LENGTH); i++ )
    {
        if(descriptor_list[i].wValue == wValue)
        {
            dataSrc = descriptor_list[i].addr;
            dataLen = descriptor_list[i].length;
        }
    }

    if( dataSrc != NULL)
    {
        memcpy_P(dataDst, dataSrc, dataLen);
    }

    /* Return the same bytes requested or less */
    if(dataLen > wLength)
    {
        dataLen = wLength;
    }

    /* Send USB Msg */
    inEP0->AUXDATA = 0;
    inEP0->CNT = dataLen;
    inEP0->STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);

    while( (inEP0->STATUS & USB_EP_BUSNACK0_bm) == 0);
}

void USB_setAddress(uint8_t address)
{
    /* USB_ADDR
     * Bit 7 – Reserved
     * Bit 6:0 – ADDR[6:0]: Device Address
     */
     /* Any device which is not yet assigned an address
      * must respond to packets sent to address zero
      */
    USB_EP_t *inEP0 = &USB_SRAM.epCfgTable[1];

    /* Send USB Msg */
    inEP0->AUXDATA = 0;
    inEP0->CNT = 0;
    inEP0->STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);

    while( (inEP0->STATUS & USB_EP_BUSNACK0_bm) == 0);

    USB_ADDR = address;
}

void USB_getConfiguration(void)
{

    USB_EP_t *inEP0 = &USB_SRAM.epCfgTable[1];
    uint8_t *dataPtr = inEP0->DATAPTR;

    *dataPtr = usb_configuration;

    /* Send USB Msg */
    inEP0->AUXDATA = 0;
    inEP0->CNT = 1;
    inEP0->STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);
}

void USB_setConfiguration(void)
{
    USB_EP_t *inEP0 = &USB_SRAM.epCfgTable[1];

    usb_configuration = 0;

    /* Set EndPoint Configuration Also... */
    /* EP1 Out */
    USB_SRAM.epCfgTable[2].STATUS = USB_EP_BUSNACK1_bm;
    USB_SRAM.epCfgTable[2].CTRL = EP1CTRL_OUT;
    USB_SRAM.epCfgTable[2].DATAPTR = &ep1_out;

    /* EP1 In */
    USB_SRAM.epCfgTable[3].STATUS = USB_EP_BUSNACK0_bm;
    USB_SRAM.epCfgTable[3].CTRL = 0u;
    USB_SRAM.epCfgTable[3].DATAPTR = 0u;

    /* EP2 Out */
    USB_SRAM.epCfgTable[4].STATUS = USB_EP_BUSNACK0_bm;
    USB_SRAM.epCfgTable[4].CTRL = 0u;
    USB_SRAM.epCfgTable[4].DATAPTR = 0u;

    /* EP2 In */
    USB_SRAM.epCfgTable[5].STATUS = USB_EP_BUSNACK1_bm;
    USB_SRAM.epCfgTable[5].CTRL = EP2CTRL_IN;
    USB_SRAM.epCfgTable[5].DATAPTR = &ep2_in;

    /* Send USB Msg */
    inEP0->AUXDATA = 0;
    inEP0->CNT = 0;
    inEP0->STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);

    while( (inEP0->STATUS & USB_EP_BUSNACK0_bm) == 0);
}

void USB_doNothing(void)
{
    USB_EP_t *inEP0 = &USB_SRAM.epCfgTable[1];

    /* Send USB Msg */
    inEP0->AUXDATA = 0;
    inEP0->CNT = 0;
    inEP0->STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);

    while( (inEP0->STATUS & USB_EP_BUSNACK0_bm) == 0);
}

void USB_HidGetReport(uint16_t wLength){

    USB_EP_t *inEP0 = &USB_SRAM.epCfgTable[1];

    /* answer with all 0 */
    memset(inEP0->DATAPTR, 0, wLength);

    /* Send USB Msg */
    inEP0->AUXDATA = 0;
    inEP0->CNT = wLength;
    inEP0->STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);

    while( (inEP0->STATUS & USB_EP_BUSNACK0_bm) == 0);
}
