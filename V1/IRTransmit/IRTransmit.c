#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRTransmit.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

int main(void) {
	
	// enable pin C5 as output
	DDRC |= (1<<DDC5);

	int cycle = 0;

	while(1) {

		if (cycle < 5) { // send a '1' communication bit

			// LED on
			PORTC |= (1<<PORTC5);
			_delay_us(100);

			// LED off
			PORTC &= ~(1<<PORTC5);
			_delay_us(400);

			cycle += 1;

		} else { // send a '0' communication bit

			// LED off
			PORTC &= ~(1<<PORTC5);
			_delay_us(500);

			cycle = 0;

		}			
		
	}

	return(0);

}
