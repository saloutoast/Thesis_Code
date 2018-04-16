#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:EPMtest.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7);
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	DDRC=0;
	PORTC=0;
	DDRC = (1<<DDC3);

	// test power by turning on LEDs
	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	
	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	
	int ii=0;
	while(1) {
		// loop
		
		//switch E.P.M. direction 1 (detach)
		PORTB |= (1<<PORTB0); // set inner LED, indicating direction 1
		PORTB |= (1<<PORTB1); // set middle LED
		PORTB |= (1<<6);//activate E.P.M direction 1
		_delay_us(120);//leave on for 120us
		PORTB &=~(1<<6);//deactivate E.P.M
		PORTB &=~(1<<7);//deactivate E.P.M
		PORTB &= ~(1<<PORTB1); //clear middle LED
		ii = 0;
		while (ii<10) {
			_delay_ms(100);//delay 1s
			ii++;
		}		
		
		//switch E.P.M. direction 2 (re-attach)
		PORTB &= ~(1<<PORTB0); // clear inner LED, indicating direction 2
		PORTB |= (1<<PORTB2); // set outer LED
		PORTB |= (1<<7);//activate E.P.M direction 2
		_delay_us(120);//leave on for 120us
		PORTB &=~(1<<6);//deactivate E.P.M
		PORTB &=~(1<<7);//deactivate E.P.M
		PORTB &= ~(1<<PORTB2); // clear outer LED
		ii = 0;
		while (ii<10) {
			_delay_ms(100);//delay 1s
			ii++;
		}

	}

}

// "move" by detaching magnet for a specified time in ms
void detach(int time) {

	//switch E.P.M. direction 1 (detach)
	PORTB |= (1<<PORTB0); // set inner LED, indicating direction 1
	PORTB |= (1<<6);//activate E.P.M direction 1
	_delay_us(120);//leave on for 120us
	PORTB &=~(1<<6);//deactivate E.P.M
	PORTB &=~(1<<7);//deactivate E.P.M

	_delay_ms(time); // stay detached for desired time

	//switch E.P.M. direction 2 (re-attach)
	PORTB &= ~(1<<PORTB0); // clear inner LED, indicating direction 2
	PORTB |= (1<<7);//activate E.P.M direction 2
	_delay_us(120);//leave on for 120us
	PORTB &=~(1<<6);//deactivate E.P.M
	PORTB &=~(1<<7);//deactivate E.P.M

	return;
}
