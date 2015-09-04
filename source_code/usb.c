/*
*  usb.c
*  Author: Miguel
*/
/* Features:
* 31 configurable endpoints and 1 control endpoint
*/
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <stdio.h>
#include "usb_descriptors.h"
#include "usb_types.h"
#include "utils.h"
#include "usb.h"
/* Data structure required by the USB controller */
volatile USB_EP_pair_t endpoints[USB_MAXEP+1] __attribute__((section (".data,\"aw\",@progbits\n.p2align 1;")));
/* Buffers where to store EP specific data */
volatile uint8_t ep0_out[EP0SIZE];
volatile uint8_t ep0_in[EP0SIZE];
volatile uint8_t ep1_out[EP1SIZE_OUT];
volatile uint8_t ep2_in[EP2SIZE_IN];
/* Zero when we are not configured, non-zero when enumerated */
volatile uint8_t usb_configuration = 0;


void init_usb_clock(void)
{	
	/* When arriving here, we are already running at 32MHz */
	OSC.PLLCTRL = OSC_PLLSRC_RC32M_gc | 0x06;               // 32MHz/4*6 = 48MHz for USB
	CCP = CCP_IOREG_gc;                                     // Inform process we change a protected register
	OSC.CTRL |= OSC_PLLEN_bm;                               // Enable PLL
	while((OSC.STATUS & OSC_PLLRDY_bm) == 0);               // Wait until PLL is locked
	CLK.USBCTRL = CLK_USBSEN_bm;                            // Enable the USB clock, source is PLL    
}

void init_usb_configuration(void)
{
    /* USB pins factory calibration */
    USB.CAL0 = ReadCalibrationByte(PROD_SIGNATURES_START+USBCAL0_offset);
    USB.CAL1 = ReadCalibrationByte(PROD_SIGNATURES_START+USBCAL1_offset);

    /* Configure endpoint configuration table */
    USB_EPPTR = (unsigned)endpoints;

	/* Endpoint 0 configuration */
	/* EP0 Out */
	endpoints[0].out.STATUS = 0;
	endpoints[0].out.CTRL = USB_EP_TYPE_CONTROL_gc | USB_EP_BUFSIZE_64_gc;
	endpoints[0].out.DATAPTR = (unsigned)&ep0_out;
	/* EP0 In */
	endpoints[0].in.STATUS = USB_EP_BUSNACK0_bm;
	endpoints[0].in.CTRL = USB_EP_TYPE_CONTROL_gc | USB_EP_BUFSIZE_64_gc;
	endpoints[0].in.DATAPTR = (unsigned)&ep0_in;
    
	/* USB Address, 0 by default until addressed */
	USB_ADDR = 0x0000;
	
    /* Enable USB controller, full speed, specify number of endpoints */
	USB_CTRLA = (USB_ENABLE_bm | USB_SPEED_bm | USB_MAXEP);

	/* Attach to USB bus */
	USB_CTRLB = USB_ATTACH_bm;  
}

void init_usb_interrupts(void)
{
	/* USB interrupt enable */
	USB_INTCTRLA = USB_BUSEVIE_bm | USB_INTLVL_MED_gc;
	USB_INTCTRLB = USB_TRNIE_bm | USB_SETUPIE_bm;    
}

void init_usb(void)
{
	/* Initialize clock settings */
	init_usb_clock();
	
	/* Initialize endpoint0 conf */
	init_usb_configuration();
	
	/* Initialize USB interrupts */
	init_usb_interrupts();
}

ISR(USB_BUSEVENT_vect)
{
    usbdprintf("<%02x>", USB_INTFLAGSACLR);
	if (USB_INTFLAGSACLR & USB_SOFIF_bm)
	{
		// Start of frame
		USB_INTFLAGSACLR = USB_SOFIF_bm;
		usbdprintf_P(PSTR(".|"));
	}
	else if (USB_INTFLAGSACLR & (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm))
	{
		// CRC error, Underflow, Overflow
		USB_INTFLAGSACLR = (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm);
		usbdprintf_P(PSTR("E|"));
	}
	else if (USB_INTFLAGSACLR & USB_STALLIF_bm)
	{
		// Stall
		USB_INTFLAGSACLR = USB_STALLIF_bm;
		usbdprintf_P(PSTR("S|"));
	}
	else
	{
		// Suspend, resume, reset
		if (USB_INTFLAGSACLR != 0)
		{
			usbdprintf("B%02x|", USB_INTFLAGSACLR);
		}
		USB_INTFLAGSACLR = USB_SUSPENDIF_bm | USB_RESUMEIF_bm | USB_RSTIF_bm;
		if (USB.STATUS & USB_BUSRST_bm)
		{
			USB.STATUS &= ~USB_BUSRST_bm;
			usbdprintf_P(PSTR("R|"));
			init_usb_configuration();
		}
	}
}

// Enable the OUT stage on the default control pipe
inline void enable_ep0_out(void)
{
    LACR16(&endpoints[0].out.STATUS, USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);
}

void send_usb_packet(uint8_t endpoint_number, volatile uint8_t* addr, uint16_t size)
{
    //TODO: the OVF, STALL, and TRNCOMPL flags are in b->STATUS. Clear them if anyone cares.
    usbdprintf("%04x|", endpoints[endpoint_number].in.STATUS);   
    endpoints[endpoint_number].in.DATAPTR = (unsigned)addr;                                      // Load pointer to the data to send
    endpoints[endpoint_number].in.AUXDATA = 0;                                                   // Trigger message sending
    endpoints[endpoint_number].in.CNT = size;                                                    // Set correct data size
    LACR16(&endpoints[endpoint_number].in.STATUS, USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm);     // Clear correct flags
    usbdprintf("%04x|", endpoints[endpoint_number].in.STATUS);
    //_delay_us(150);
}

void flush_ep0_endpoint_contents(uint8_t size)
{
    send_usb_packet(0, ep0_in, size);
}

void wait_for_endpoint_packet_send(uint8_t endpoint_number)
{
    while(((endpoints[endpoint_number].in.STATUS) & USB_EP_TRNCOMPL0_bm) == 0);
}

ISR(USB_TRNCOMPL_vect)
{
    usbdprintf("!%02x!", USB_INTFLAGSBCLR);
	// Clear Interrupt flags
    USB.FIFOWP = 0;
	USB_INTFLAGSBCLR = USB_SETUPIF_bm | USB_TRNIF_bm;
	
	// Read once to prevent race condition where SETUP packet is interpreted as OUT
	USB_Request_Header_t* usbMsg = (USB_Request_Header_t*)endpoints[0].out.DATAPTR;
	uint8_t ep0status = endpoints[0].out.STATUS;
    uint8_t ep1status = endpoints[1].out.STATUS;
    uint8_t ep2status = endpoints[2].in.STATUS;
	enable_ep0_out();
	
    printf("%02x %02x %02x %02x ", endpoints[1].out.STATUS, endpoints[1].in.STATUS, endpoints[2].out.STATUS,endpoints[2].in.STATUS);
    // Endpoint 2 handling
    if (ep2status & USB_EP_TRNCOMPL0_bm)
    {
        //endpoints[2].in.CNT = 64;
        //endpoints[2].in.STATUS &= USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm;
        printf("EP2|");
    }
    // Endpoint 1 handling
    if (ep1status & USB_EP_TRNCOMPL0_bm)
    {
        USB_Rx(ep1_out, endpoints[1].out.CNT);
        printf("EP1|");
    }
    printf("%02x %02x %02x %02x\r\n", endpoints[1].out.STATUS, endpoints[1].in.STATUS, endpoints[2].out.STATUS,endpoints[2].in.STATUS);
	// Endpoint0 handling
	if (ep0status & USB_EP_SETUP_bm)
	{
		if((usbMsg->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_STANDARD)
		{
			switch(usbMsg->bRequest)
			{
			    case GET_STATUS:
				{
    				usbdprintf_P(PSTR("s|"));
                    ep0_in[0] = 0;
                    ep0_in[1] = 0;
                    flush_ep0_endpoint_contents(2);
					break;                    
				}
			    case CLEAR_FEATURE:
                {
                    usbdprintf_P(PSTR("f|"));
                    flush_ep0_endpoint_contents(0);
                    break;
                }          
			    case SET_FEATURE:
			    {
    			    usbdprintf_P(PSTR("f|"));
    			    flush_ep0_endpoint_contents(0);
    			    break;
			    }
			    case SET_ADDRESS:
			    {
    			    usbdprintf("a%02X|", usbMsg->wValue & 0x7F);
                    enable_ep0_out();
                    flush_ep0_endpoint_contents(0);
                    wait_for_endpoint_packet_send(0);
    			    USB.ADDR = usbMsg->wValue & 0x7F;
    			    break;
			    }      
			    case GET_DESCRIPTOR:
			    {
    			    usbdprintf_P(PSTR("d"));
    			    usbdprintf("/%04x", usbMsg->wValue);
    			    usbdprintf("/%04x", usbMsg->wIndex);
    			    usbdprintf("/%d", usbMsg->wLength);
    			    const uint8_t* dataSrc = NULL;
    			    uint8_t dataLen = 0;

    			    for(uint8_t i=0; i<USB_DESCRIPTOR_LIST_LENGTH; i++)
    			    {
        			    if(descriptor_list[i].wValue == usbMsg->wValue && descriptor_list[i].wIndex == usbMsg->wIndex)
        			    {
            			    dataSrc = descriptor_list[i].addr;
            			    dataLen = descriptor_list[i].length;
        			    }
    			    }

                    // If we found the descriptor, otherwise stall
    			    if(dataSrc != NULL)
    			    {
        			    memcpy_P((void*)ep0_in, dataSrc, dataLen);

        			    /* Return the same bytes requested or less */
        			    if(dataLen > usbMsg->wLength)
        			    {
            			    dataLen = usbMsg->wLength;
        			    }
        			    
        			    usbdprintf("/%d", dataLen);
        			    usbdprintf("/%d", ep0_in[0]);
        			    usbdprintf("/%d|", ep0_in[1]);

        			    /* Send USB Msg */
        			    flush_ep0_endpoint_contents(dataLen);
    			    }
                    else
                    {
                        usbdprintf_P(PSTR("PB Desc|"));
                        endpoints[0].in.CTRL |= USB_EP_STALL_bm;
                        endpoints[0].out.CTRL |= USB_EP_STALL_bm;                        
                    }    
                    //_delay_ms(2000);              
    			    break;
			    }
			    case GET_CONFIGURATION:
			    {
    			    usbdprintf_P(PSTR("c|"));
                    ep0_in[0] = usb_configuration;
    			    flush_ep0_endpoint_contents(1);
    			    break;
			    }
			    case SET_CONFIGURATION:
			    {
    			    usbdprintf_P(PSTR("C|"));
    			    usb_configuration = usbMsg->wValue;
                    endpoints[1].out.STATUS = 0;
                    endpoints[1].out.CTRL = EP1CTRL_OUT;
                    endpoints[1].out.DATAPTR = (unsigned)ep1_out;
                    endpoints[1].in.STATUS = USB_EP_BUSNACK0_bm;
                    endpoints[1].in.CTRL = 0;
                    endpoints[1].in.DATAPTR = 0;
                    endpoints[2].out.STATUS = USB_EP_BUSNACK0_bm;
                    endpoints[2].out.CTRL = 0;
                    endpoints[2].out.DATAPTR = 0;
                    endpoints[2].in.STATUS = USB_EP_BUSNACK0_bm;
                    endpoints[2].in.CNT = 0;
                    endpoints[2].in.CTRL = EP2CTRL_IN;
                    endpoints[2].in.DATAPTR = (unsigned)ep2_in;
    			    flush_ep0_endpoint_contents(0);
    			    break;
			    }
			    case SET_DESCRIPTOR:
			    {
    			    usbdprintf_P(PSTR("D|"));
    			    flush_ep0_endpoint_contents(0);
    			    break;
			    }
			    default:
			    {
                    // Stall
                    usbdprintf("x%02x|", usbMsg->bRequest);
    			    endpoints[0].in.CTRL |= USB_EP_STALL_bm;
    			    endpoints[0].out.CTRL |= USB_EP_STALL_bm;
    			    break;
			    }
			}
		}
		else if((usbMsg->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_CLASS)
		{
            switch (usbMsg->bRequest)
            {
                case HID_GET_REPORT:
                {
                    usbdprintf_P(PSTR("r|"));
                    memset((void*)ep0_in, 0, EP0SIZE);
                    flush_ep0_endpoint_contents(EP0SIZE);
                    break;
                }
                case HID_SET_REPORT:
                {
                    usbdprintf_P(PSTR("R|"));
                    flush_ep0_endpoint_contents(0);
                    break;
                }    
                case HID_SET_IDLE:
                {
                    usbdprintf_P(PSTR("I|"));
                    flush_ep0_endpoint_contents(0);
                    break;
                }                   
			    default:
			    {
                    // Stall
                    usbdprintf("X%02x|", usbMsg->bRequest);
    			    endpoints[0].in.CTRL |= USB_EP_STALL_bm;
    			    endpoints[0].out.CTRL |= USB_EP_STALL_bm;
    			    break;
			    }
            }
		}
	}
 	else if(ep0status & USB_EP_TRNCOMPL0_bm)
 	{
 	}
}

/* RX callback function, here we receive HID messages from HOST */
void USB_Rx(uint8_t* buffer, uint8_t size)
{
    /* Ping function, always reply the same */
    USB_Tx(buffer, size);
}

/* TX function, call to send messages to HOST */
void USB_Tx(uint8_t* buffer, uint8_t size)
{
    memcpy(ep2_in,buffer, size);
    endpoints[2].in.CNT = size;
    endpoints[2].in.STATUS &= USB_EP_TRNCOMPL0_bm;
}
