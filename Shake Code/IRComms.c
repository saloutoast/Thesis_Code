#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRComms.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

void setLED(unsigned char red, unsigned char green, unsigned char blue);



int main(void)
{
	//setup code, do not change
	DDRC=0;
	PORTC=0;
	DDRC |= (1<<5);
	DDRD=0;
	PORTD=0;
	DDRD |= (1<<7);
	DDRD |= (1<<6);
	PORTD &=~(1<<6);
	PORTD &=~(1<<7);

	//end setup code

	//user loop forever
	while(1)
	{

		//set led (R,G,B) 0-255 for each
		

	/*	//switch E.P.M.
		PORTD |= (1<<6);//activate E.P.M direction 1
		_delay_us(80);//leave on for 80us
		PORTD &=~(1<<6);//deactivate E.P.M
		PORTD &=~(1<<7);//deactivate E.P.M
		setLED(0,0,50);//set led to green
		_delay_ms(140);//delay 140 ms
	*/	


		setLED(0,0,50);//set led to green
		PORTD |= (1<<6);//activate E.P.M direction 2
		_delay_us(80);//leave on for 80us
		PORTD &=~(1<<6);//deactivate E.P.M
		PORTD &=~(1<<7);//deactivate E.P.M
		setLED(0,0,0);//set led red
		_delay_ms(140);//delay 140ms

	}
	//end user loop
}


void setLED(unsigned char red, unsigned char green, unsigned char blue)
{
	//Bit banging 20-600kHz
	//Send LSB first
	
	unsigned char array[4] = {0x3A, red, blue,green};

	for(char byte = 0; byte <= 3; byte++)
	{
		for(unsigned char bit=0; bit<=7; bit++)
		{
			//bit initiation
			PORTC |= (1<<5);
			_delay_us(2);
			PORTC &=~(1<<5);

			_delay_us(3);

			if(array[byte] & (0x80>>bit))
				PORTC |= (1<<5);
			else
				PORTC &=~(1<<5);

			_delay_us(10); 
			PORTC &=~(1<<5);

			_delay_us(5);  //~50kHz	
		}
	}
	_delay_us(80);//End of Sequence
}
