#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

#define SERIAL_RX
#define SERIAL_TX

#define RX_BUFFER_SIZE	32
#define TX_BUFFER_SIZE  8

#ifdef SERIAL_RX
unsigned volatile char 
	RX_buffer[RX_BUFFER_SIZE],
	RX_buffer_write_pos = 0,
	RX_buffer_read_pos = 0,
	RX_new_data = 0;
#endif

#ifdef SERIAL_TX
unsigned volatile char 
	TX_buffer[TX_BUFFER_SIZE],
	TX_buffer_write_pos = 0,
	TX_buffer_read_pos = 0,
	TX_new_data = 0,
	TX_active = 0;

static int serial_putchar(char c, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);
#endif

void serial_init()
{
	// Baudrate
	UBRR0H	=	0;
	UBRR0L	=	16;

#ifdef SERIAL_RX
	UCSR0B	|=
		// Receiver Enable
		(1 << RXEN0) |
		// RX Complete Interrupt Enable
		(1 << RXCIE0);
#endif
#ifdef SERIAL_TX
	UCSR0B	|=
		// Transmitter Enable
		(1 << TXEN0) |
		// TX Complete Interrupt Enable
		(1 << TXCIE0);
	
	// Tie in with standard out (printf)
	stdout	=	&mystdout;
#endif 
	// 8-bit mode
	UCSR0C	=	(1 << UCSZ01) | (1 << UCSZ00);
	// Double speed
	UCSR0A |= 	(1 << U2X0);
	
	sei();
}

#ifdef SERIAL_TX
// Done transmitting
ISR (USART_TX_vect)
{
	if (TX_new_data != 0)
	{
		UDR0 = TX_buffer[TX_buffer_read_pos];
		TX_new_data --;
		TX_active = 1;
		
		if (++ TX_buffer_read_pos == TX_BUFFER_SIZE)
		{
			TX_buffer_read_pos = 0;
		}
	}
	else
	{
		TX_active = 0;
	}
}

static int serial_putchar(char c, FILE *stream)
{
    if (c == '\n') serial_putchar('\r', stream);
	// Cold start
	if (!TX_active)
	{
		// Wait until register is empty
		while ( !(UCSR0A & (1 << UDRE0)) );
		// Trigger transmission
		UDR0	=	c;
		TX_active = 1;
	}
	// Add c to buffer
	else
	{
		// Wait until there is room in the buffer
		while (TX_new_data >= TX_BUFFER_SIZE - 1);
		
		TX_buffer[TX_buffer_write_pos] = c;
		TX_new_data ++;
		
		if (++ TX_buffer_write_pos == TX_BUFFER_SIZE)
		{
			TX_buffer_write_pos = 0;
		}
	}

    return 0;
}
#endif

#ifdef SERIAL_RX
// Received new byte
ISR (USART_RX_vect)
{	
	RX_buffer[RX_buffer_write_pos] = UDR0;
	RX_new_data ++;
	
	if (++ RX_buffer_write_pos == RX_BUFFER_SIZE)
	{
		RX_buffer_write_pos = 0;
	}
}

uint8_t serial_getchar(void)
{	
	// Wait for data
	while (RX_new_data == 0);
	
	uint8_t value;
	// Get the value
	value = RX_buffer[RX_buffer_read_pos];
	RX_new_data --;
	
	// Next read position
	// Increment and overflow
    if (++ RX_buffer_read_pos == RX_BUFFER_SIZE)
	{
		RX_buffer_read_pos = 0;
	}
	
	return value;
}

uint8_t serial_new_data(void)
{
	return RX_new_data;
}

uint8_t serial_get_until(uint8_t match, uint8_t *output, uint8_t max_length)
{
	uint8_t 
		i = 0,
		c;
	while ((c = serial_getchar()) != match && i < max_length)
	{
		output[i ++] = c;
	}
	// Terminate string
	output[i] = match;
	// Total length
	return i;
}

uint8_t serial_line(uint8_t *output)
{
	uint8_t i = 0;
	// Capture what has been received so far
	while (serial_new_data())
	{
		output[i ++] = serial_getchar();
	}
	// Terminate string
	output[i] = '\0';
	// Total length
	return i;
}

uint8_t serial_peek(void)
{
	return RX_buffer[RX_buffer_write_pos];
}
#endif