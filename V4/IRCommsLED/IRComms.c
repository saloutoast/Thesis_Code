#define F_CPU 8000000UL
//avrdude -c usbtiny -B 1000 -p m328p -e -U flash:w:IRComms.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

int main(void) {
	
	// enable pin C5 and pin C4 as outputs
	DDRC = (1<<DDC5) | (1<<DDC4) | (1<<DDC3) | (1<<DDC2); // C5: start bit, C4: communication bits, C3: IR LED
	
	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7);
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	PORTB |= (1<<PORTB1); // turn on middle LED
	PORTB |= (1<<PORTB2);	
	
	int ii=0;
	while(1) {
		// loop
		
		/*	//switch E.P.M.
		PORTB &= ~(1<<PORTB1); // clear middle LED
		PORTB |= (1<<6);//activate E.P.M direction 1
		_delay_us(80);//leave on for 80us
		PORTB &=~(1<<6);//deactivate E.P.M
		PORTB &=~(1<<7);//deactivate E.P.M
		PORTB |= (1<<PORTB1); //set middle LED
		_delay_ms(140);//delay 140 ms
		
		

		PORTB &= ~(1<<PORTB2); // clear outer LED
		PORTB |= (1<<6);//activate E.P.M direction 2
		_delay_us(80);//leave on for 80us
		PORTB &=~(1<<6);//deactivate E.P.M
		PORTB &=~(1<<7);//deactivate E.P.M
		PORTB |= (1<<PORTB2); // set outer LED
		_delay_ms(140);//delay 140ms */

		PORTB |= (1<<PORTB1);
		PORTB |= (1<<PORTB2);
		
		while(ii<10) {
			_delay_ms(30);
			ii++;
		}
		ii=0;

		PORTB &= ~(1<<PORTB1);
		PORTB &= ~(1<<PORTB2);

		while(ii<10) {
			_delay_ms(30);
			ii++;
		}
		ii=0;
	}

}


