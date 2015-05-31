/*
 * serial.c
 *
 * Created: 25/04/2015 19:13:04
 *  Author: limpkin
 */
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>
#include "serial.h"


#ifdef PRINTF_ENABLED
/*
 * Putchar function for printf
 */
static int uart_putchar(char c, FILE* stream)
{    
    // Wait for the transmit buffer to be empty
    while (!(USARTC0_STATUS & USART_DREIF_bm));
    
    // Put our character into the transmit buffer
    USARTC0_DATA = c;
    
    return 0;
}

/*
 * getchar function for scanf
 */
int uart_getchar(FILE* stream)
{
    // Wait until data has been received.
    while(!(USARTC0_STATUS & USART_RXCIF_bm)); 
    
    // Temporarily store received data
    char data = USARTC0_DATA;
    
    // Send to console what has been received, so we can see when typing
    uart_putchar(data, stream); 
    
    return data;
}

FILE usart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
#endif

/*
 * Send char through usart
 * @param   c   The char
 */
void usart_send_char(char c)
{
    // Wait until DATA buffer is empty
    while(!(USARTC0_STATUS & USART_DREIF_bm));
    USARTC0_DATA = c;
}

/*
 * Init our serial port
 */
void init_serial_port(void)
{
#ifdef PRINTF_ENABLED
    stdout = stdin = &usart_str;                            // setup printf
#endif
    PORTC_DIRSET = PIN3_bm;                                 // PC3 is TX
    PORTC_DIRCLR = PIN2_bm;                                 // PC2 is RX
    USARTC0.BAUDCTRLA = 131;                                // 115k2 baudrate
    USARTC0.BAUDCTRLB = 0xD0;                               // 115k2 baudrate
    USARTC0.CTRLC = USART_CHSIZE_8BIT_gc;                   // Asynchronous 8N1 mode
    USARTC0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;          // Enable USARTC0 TX & RX
    serialdprintf_P(PSTR("-----------------------\r\n"));   // Debug string
    serialdprintf_P(PSTR("Serial port initialized\r\n"));   // Debug string
}