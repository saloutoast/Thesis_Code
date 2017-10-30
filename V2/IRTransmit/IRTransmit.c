#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRTransmit.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

static volatile int cycle = 0;
static char toSend = a;
static volatile int bits_sent = 0;
static volatile int next_bit = 0;
static volatile int sending = 0;
static volatile int pausing = 0;

int main(void) {
	
	// enable pin C5 as output
	DDRC |= (1<<DDC5);

	// Initialize timer2
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
	OCR2A = 50; // compare every 50 counts (every 50us, 1/10 frequency of communication bits)
	TCCR2B |= (0<<CS22)|(1<<CS21)|(0<<CS20); // prescaler of 1/8: count every 1us

	sei(); // Enable interrupts

	while(1) {	
		// loop
	}
}

ISR(TIMER2_COMPA_vect) {
	
	if (pausing==0) { // if not pausing
		if (sending==0) { // send start bit
			PORTC |= (1<<PORTC5);
			cycle = 0;
			sending = 1;
		}
		else { // sending = 1
			if (cycle==10) {

				if (bits_sent<8) { // first message
					next_bit = (toSend & (1<<(7-bits_sent))) >> (7-bits_sent);

					if (next_bit==1) { PORTC |= (1<<PORTC5); } // if bit is 1, set C5
					else { PORTC &= ~(1<<PORTC5); } // if bit is 0, clear C5

					bits_sent += 1;

				} else if (bits_sent < 16) { // second message
					next_bit = (toSend & (1<<(15-bits_sent))) >> (15-bits_sent);

					if (next_bit==1) { PORTC |= (1<<PORTC5); } // if bit is 1, set C5
					else { PORTC &= ~(1<<PORTC5); } // if bit is 0, clear C5

					bits_sent += 1;

				} else { // two messages have been sent, bits_sent=16
					sending = 0;
					pausing = 1;
					bits_sent = 0;
				}
				cycle = 0;

			} else if (cycle==2) {
				PORTC &= ~(1<<PORTC5); // always clear bit after 2 cycles
				cycle += 1;
			} else { 
				cycle += 1; // Increment edge counter
			}
		}	
	}
	else { // if pausing
		cycle +=1; 
		if (cycle==200) { // pause for 10ms after sending message
			cycle=0;
			pausing=0;
		}
	}
}
