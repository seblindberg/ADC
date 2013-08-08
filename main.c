/* Name: main.c
 * Author: Sebastian Lindberg
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"


#define LED_INIT()		DDRB  |= (1 << 5);
#define LED_ON() 			PORTB |= (1 << 5);
#define LED_OFF() 		PORTB &=~(1 << 5);
#define LED_TOGGLE()	PORTB ^= (1 << 5);


ISR_EMPTY (TIMER0_OVF_vect);

ISR (ADC_vect)
{		
	LED_TOGGLE();
	printf("%u, ", ADCW);
}

int main(void)
{
	LED_INIT();
	
	// Setup ADC
	ADMUX	=
		// AVCC with external capacitor at AREF pin
		(0 << REFS1) | (1 << REFS0) |
		// Left adjust (no), MUX channel 0
		(0 << ADLAR) | 0;
	// Enable ADC, Autotrigger, enable interrupt, divisionfactor 64
	ADCSRA	=	(1 << ADEN) | (1 << ADATE) | (1 << ADIF) | (1 << ADIE) | 6;
	// Timer/Counter1 Overflow
	ADCSRB	=	(1 << ADTS2) | (1 << ADTS1) | (0 << ADTS0);
	// Digital Input Disable on pin 0
	DIDR0	=	(1 << ADC0D);
	
	// Timer 1
	TCCR1B	=	(0 <<  WGM12) | 3;
	
	OCR1A	=	32000/2;
	// Overflow Interrupt
	TIMSK1	=	(1 << TOIE1);

	serial_init();
	
	// Enable global interrupts
	sei();

  for (;;) 
  {
		// Do nothing
  }
  
  return 0;
}
