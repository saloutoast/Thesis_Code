#define F_CPU 8000000UL
//  C:\Users\mike r\Dropbox\flipbot\flippy2\flippy02_code\default>avrdude -c usbtiny -B 1 -p m328p -e -U flash:w:shake.hex -F
//write fuses: -U lfuse:w:0xE2:m
//avrdude -c stk500v2 -P usb -p m328p -e -U flash:w:flippy02.hex

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#define tbi(x,y) x ^= _BV(y) //toggle bit - using bitwise XOR operator

int main(void)
{
	DDRB=0xff; // PortB as output
	PORTB=0x00; // All pins low
	
	unsigned int i;
	while(1==1) // infinite loop sending one bits
	{
		for(i=0;i<65535;i++);
		for(i=0;i<65535;i++);		
		tbi(PORTB,PB0); // toggle	
	}		
}


