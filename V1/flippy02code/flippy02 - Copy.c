#define F_CPU 8000000UL
//  C:\Users\mike r\Dropbox\flipbot\flippy2\flippy02_code\default>avrdude -c usbtiny -B 1 -p m328p -e -U flash:w:flippy02.hex
//write fuses: -U lfuse:w:0xE2:m

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include <stdlib.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <math.h>
#include <stdio.h>
#include <avr/interrupt.h>
#define FOSC 8000000
#define BAUD 38400
#define MYUBRR (((((FOSC * 10) / (16L * BAUD)) + 5) / 10))

int ofc;
//Overflow ISR
ISR(TIMER1_OVF_vect)
{
//increment overflow counter

ofc++;

}

//setup for printf
int uart_putchar(char c, FILE *stream) { 
    if (c == '\n') 
        uart_putchar('\r', stream); 
    loop_until_bit_is_set(UCSR0A, UDRE0); 
    UDR0 = c; 

    return 0; 
}
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

void ioinit (void) {
    UBRR0H = MYUBRR >> 8;
    UBRR0L = MYUBRR;
    UCSR0B = (1<<TXEN0);
    
    fdev_setup_stream(&mystdout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;
}


void set_out_led(int);
void set_up_led(int);

int get_adc(int ADCchannel)
{
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADC;


}

		
int main(void)
{
ofc=0;
	DDRC=0;
	PORTC=0;
	ioinit();	
	printf("wakup\n\r");
	//adc_init
	 // Select Vref=AVcc
	ADMUX |= (1<<REFS0);
	//set prescaller to 128 and enable ADC 
	ADCSRA = (1<<ADEN);    

	ADCSRB=0;
		ADCSRA |= (1<<ADSC);
		TCCR1B=0;
	TCCR1B |= (1 << CS11);//|(1 << CS10); // Set up timer 64 prescale
	TCNT1 = 0; // Reset timer value
	TIMSK1|=(1<<ICIE1)|(1<<TOIE1);
	sei();


	int sensor_max=0;
	int sensor_max_time=0;
	int trigger_set=0;
	int triggered=0;
	int value=0;
	int prev_time=0;

	int pulse_min=1024;
	int pulse_max=0;
	int prev_value;

	int quarter_visited=0;
	int quarterp_visited=0;
	int last_quarter=0;

	int inhibit_on;
	int inhibit_off;
	
	int oldmax=0;

	OSCCAL =107;
	while(1)
	{

/*	while(1)
	{

	for(int i=0;i<255;i++)
	{
printf("osc %d \n\r",OSCCAL );
OSCCAL ++;
_delay_ms(10);
}

}*/

printf("explode yet??\n\r");

_delay_ms(10);
//printf("looping %d\n\r",TCNT1);
		/*if(TCNT1>200)
		{

		set_up_led(0);

		}*/

		prev_value=value;
		value=get_adc(0);
		
		if(pulse_min>value)
		{
			pulse_min=value;

		}

		if(pulse_max<value)
		{

			pulse_max=value;
		}

		if((pulse_max-pulse_min)>30)
		{
			
			int quarter=(pulse_max-pulse_min)/4+pulse_min;
			int quarterp=pulse_max-(pulse_max-pulse_min)/4;
			int half=(pulse_max-pulse_min)/2+pulse_min;
			
			if(value>quarterp)
			{
				quarterp_visited=1;
				last_quarter=1;
			

			}
		


			if(value<quarter)
			{
				quarter_visited=1;
				last_quarter=2;
			

			}


			if(quarter_visited==1&&quarterp_visited==1&&last_quarter==2)
			{
				if(value>half)
				{
					set_up_led(1);	
					pulse_min=value;
					pulse_max=value;
					quarter_visited=0;
					quarterp_visited=0;
					last_quarter=0;
					//_delay_ms(10);
					

					prev_time=TCNT1;

					inhibit_on=prev_time*.8;
					inhibit_off=prev_time*.2;
				
			//		printf("%d %d %d\n\r",TCNT1,prev_time,sensor_max);
				//	printf("trigger time %d ofc %d\n\r",TCNT1,ofc);
			//		printf("sensor max time %d value %d\n\r\n\r",sensor_max_time,sensor_max);
					int loop=TCNT1;
					TCNT1=0;
					sensor_max=0;

					oldmax=sensor_max_time;
					
					sensor_max_time=0;
					//printf("%d %d %d\r\n",prev_time,inhibit_on,inhibit_off);
					
					set_up_led(0);
				}


			}
		



		}
		

		int temp=get_adc(4);
//		printf("%d\n\r",temp);
		/*if(temp>70)
		{
			set_out_led(1);

		}
		else
		{
			set_out_led(0);

		}*/


		/*if((TCNT1<oldmax+2000)&&(TCNT1>oldmax))
		{
			set_out_led(1);

		}
		else
		{
			set_out_led(0);

		}*/


	

//		int temp=get_adc(4);
		if((TCNT1>inhibit_off ) && (TCNT1<inhibit_on))//hackey!
		{
			if(temp>sensor_max)
			{
				sensor_max=temp;
				sensor_max_time=TCNT1;
			//	printf("nt %d\n\r",TCNT1);
		//	set_out_led(1);

			}
			else
			{
				//set_out_led(1);
			}
		}
		else
		{

	//	set_out_led(0);

		}

		if((TCNT1>oldmax-1000)&&(TCNT1<oldmax))		
		{
			set_out_led(1);
		}
		else
		{
			set_out_led(0);
		}
		
	
		/*if(value<(pulse_min+20))
		{
			trigger_set=1;
			set_up_led(1);
		
		}
		if((value>(pulse_max/2))&&trigger_set==1)
		{
		
			trigger_set=0;

			 pulse_min=1024;
			pulse_max=0;
	
			set_up_led(0);
			prev_time=sensor_max_time;
				
			printf("%d %d %d\n\r",TCNT1,prev_time,sensor_max);
		//	printf("trigger time %d ofc %d\n\r",TCNT1,ofc);
		//	printf("sensor max time %d value %d\n\r\n\r",sensor_max_time,sensor_max);
			TCNT1=0;
			sensor_max=0;
			sensor_max_time=0;
		//	set_up_led(0);

		}


		int temp=get_adc(4);
/*
		/*if(temp>130)
		{
	set_out_led(1);

		}
		else
		{

	set_out_led(0);
		}*/
/*
		if((TCNT1>prev_time-4000)&&(TCNT1<prev_time))		
		{
			set_out_led(1);
		}
		else
		{
			set_out_led(0);
		}
	
		//printf("val %d\n\r",temp);
		if(temp>sensor_max)
		{
			sensor_max=temp;
			sensor_max_time=TCNT1;
		//	set_out_led(1);

		}
		else
		{

			//set_out_led(0);
		}
		
		


*/
		
	
//		_delay_ms(68);
	//	printf("timervalue for 1ms is =%d \n\r",TCNT1);

		



	//	adcvalue=get_adc(0);
	//	timer11=TCNT1;
		
	//	printf(" %d\n\r",ofc);
	//	printf("adc1= %d \n\r",	get_adc(1));
	//	printf("adc2= %d \n\r",	get_adc(2));
	//	printf("adc3= %d \n\r",	get_adc(3));
	//	printf("adc4= %d \n\r",	get_adc(4));
	

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

