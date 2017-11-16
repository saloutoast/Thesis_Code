#define F_CPU 8000000UL
//  C:\Users\mike r\Dropbox\flipbot\flippy2\flippy02_code\default>avrdude -c usbtiny -B 1 -p m328p -e -U flash:w:shake.hex -F
//write fuses: -U lfuse:w:0xE2:m
//avrdude -c stk500v2 -P usb -p m328p -e -U flash:w:flippy02.hex

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#define tbi(x,y) x ^= _BV(y) //toggle bit - using bitwise XOR operator

void set_up_led(int);
void set_out_led(int);

int main(void)
{
	DDRB=0xff; // PortB as output
	PORTB=0x00; // All pins low
	
	unsigned int i;
	int state = 0;

	set_out_led(1);
	set_up_led(1);
	_delay_ms(1000);
	set_up_led(0);	

	while(1==1) // infinite loop sending one bits
	{
		for(i=0;i<65535;i++);
		for(i=0;i<65535;i++);		
		//PORTB |= (1<<PB0); // always set to 1
		tbi(PORTB,PB0); // toggle
		set_out_led(state);
		if (state==1) { state=0; }
		if (state==0) { state=1; }	
	}		
}

void set_out_led(int state)
{
	DDRD |= (1<<3);
	PORTD |= (1<<3);
	if(state==1)
	{
		PIND |= (1<<3);
	}
	else
	{
		PIND &= ~(1<<3);
	}

}
void set_up_led(int state)
{
	DDRD |= (1<<4);
	PORTD |= (1<<4);
	if(state==1)
	{
		PIND |= (1<<4);
	}
	else
	{
		PIND &= ~(1<<4);
	}

}


