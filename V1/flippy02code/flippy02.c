#define F_CPU 8000000UL
//  C:\Users\mike r\Dropbox\flipbot\flippy2\flippy02_code\default>avrdude -c usbtiny -B 1 -p m328p -e -U flash:w:flippy02.hex
//write fuses: -U lfuse:w:0xE2:m
//avrdude -c stk500v2 -P usb -p m328p -e -U flash:w:flippy02.hex

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
#define BAUD 460800 //230400
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
	ADMUX =(ADMUX & 0xF0) | (ADCchannel & 0x0F);
//	ADMUX =(1<<7)|(1<<6)| (ADMUX & 0xF0) | (ADCchannel & 0x0F);

//ADMUX = (ADCchannel & 0x0F);

	//single conversion mode
//	ADCSRA |= (1<<ADSC);//|(1<<1);
	// wait until ADC conversion is complete
//	while( ADCSRA & (1<<ADSC) );
	
	//single conversion mode
	ADCSRA |= (1<<ADSC);//|(1<<1);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADCW;


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
//	ADMUX |= (1<<7)|(1<<6)|(1<<REFS0);
	//set prescaller to 128 and enable ADC 
	ADCSRA = (1<<ADEN);    

//	ADCSRB=0;
//	ADCSRA |= (1<<ADSC);


	TCCR1B=0;
	TCCR1B |= (1 << CS11); // Set up timer 64 prescale
	TCNT1 = 0; // Reset timer value
	TIMSK1|=(1<<ICIE1)|(1<<TOIE1);
	sei();


	int sensor_max=0;
	uint16_t sensor_max_time=0;
	int trigger_set=0;
	int triggered=0;
	int value=0;
	uint16_t prev_time=0;

	int pulse_min=1024;
	int pulse_max=0;
	int prev_value;

	int pulse_half=0;
	int quarter_visited=0;
	int quarterp_visited=0;
	int last_quarter=0;

	uint16_t inhibit_on;
	uint16_t inhibit_off;
	
	uint16_t oldmax=0;

	int old_elevation;
	//OSCCAL=106;
	OSCCAL =140;
	int elevation;

	printf("wakup!!\n\r");



	int temp;
	int temp1;

	
	//float temp;

	//float temp1;
	_delay_ms(1000);
	while(1)
	{

		int up_val,down_val;
		int up_val_max=0;
		int down_val_max=0;
		int count=0;
		int output=0;
		int output_filter=0;
		while(1)
		{
		
			down_val=get_adc(1);
			up_val=get_adc(4);

			if(up_val>up_val_max)
			{
				up_val_max=up_val;

			}
			if(down_val>down_val_max)
			{

				down_val_max=down_val;
			}
			if(TCNT1>10)
			{
				TCNT1=0;
			
				count++;

			}
			if(count>900)
			{
				count=0;
				output=output*.9+(up_val_max-down_val_max)*.1;
				int outputa=150*(float)((float)output/((float)up_val_max+(float)down_val_max));
				output_filter=((float)output_filter*.9+(float)outputa*.1);
			/*	if(outputa>20)
				{	outputa=20; }
				else if(outputa<-20)
				{outputa=-20;}*/
				//printf("%d %d\n\r",output, outputa);
				printf("%d,%d \n\r",1,output_filter);
				up_val_max=0;
				down_val_max=0;

			}

		}




	/*	while(1)
		{

		printf("osccal %d\n\r",OSCCAL);
		OSCCAL++;
		_delay_ms(50);
		}*/
		prev_value=value;
		
	//	while(1)
	//	{
		value=get_adc(4);
	
	//	printf("%d \n\r",value);
	//	_delay_ms(10);
	//	}
		if(TCNT1<(1000))
		{
			set_up_led(1);
		}
		else
		{
			set_up_led(0);

		}
		if(pulse_min>value)
		{
			pulse_min=value;
			pulse_half=(pulse_max-pulse_min)/2+pulse_min;

		}

		if(pulse_max<value)
		{

			pulse_max=value;
			pulse_half=(pulse_max-pulse_min)/2+pulse_min;
		}


		if((uint16_t)TCNT1>60000)
		{

			pulse_min=pulse_min+10;
			if(pulse_min>pulse_half)
			{
				pulse_min=pulse_half;
			}
			pulse_max=pulse_max-10;
			if(pulse_max<pulse_half)
			{
				pulse_max=pulse_half;
			}
			TCNT1=0;
			//printf("timeout  ");

			
		//	printf("%d %d\n\r",pulse_min,pulse_max);
		}




		int quarter=(pulse_max-pulse_min)/4+pulse_min;
		int quarterp=pulse_max-(pulse_max-pulse_min)/4;
		int half=(pulse_max-pulse_min)/2+pulse_min;


	

		if(value<quarter)
		{
			quarter_visited=1;
		

		}

		//pickup_sensed

		if((pulse_max-pulse_min)>30)
		{

			if(quarter_visited==1&&value>half)
			{
			
				pulse_min=pulse_min+10;
				if(pulse_min>pulse_half)
				{
					pulse_min=pulse_half;
				}
				pulse_max=pulse_max-10;
				if(pulse_max<pulse_half)
				{
					pulse_max=pulse_half;
				}
				prev_time=TCNT1;

				inhibit_on=prev_time*.75;
				inhibit_off=prev_time*.25;
				TCNT1=0;
				quarter_visited=0;

					sensor_max=0;

						oldmax=sensor_max_time*.3+oldmax*.7;
					//	oldmax=sensor_max_time;
					
						sensor_max_time=0;

						//old_elevation=elevation*.3+old_elevation*.7;


						//printf("rotation time %u max sensed at %u active between %u and %u \r\n",prev_time,oldmax,inhibit_off,inhibit_on);
					//	printf("%u %u\n\r",prev_time,oldmax);
		      //  printf("%d \n\r",(int)(10000*((float)oldmax/(float)prev_time)));
					printf("%d,%d\n\r",(int)(10000*((float)oldmax/(float)prev_time)),elevation);
				
				//printf("timeout\n\r");
			//	printf("%d %d\n\r",pulse_min,pulse_max);

			}
		}
		else
		{

		//	printf("no pulse signal detected");
		}
	

		//_delay_ms(10);
	//	int temp=get_adc(3);
	//	temp=get_adc(4);
	//	printf("%d\n\r",temp);
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


	
		//_delay_ms(10);
	
	
	
	
	
	

	//	printf("%4d %4d %4d\n\r",(int)((int)temp1-(int)temp),(int)temp,(int)temp1);
	//	printf("%d %d \n\r",(int)temp,(int)temp1);
	//	}
		if((TCNT1>inhibit_off ) && (TCNT1<inhibit_on))//hackey!
		{
			int temp2=get_adc(3);

		//	temp1=temp1*.9+(float)get_adc(1)*.1;
		//	temp=temp*.9+(float)get_adc(4)*.1;
			temp1=temp1*.9+get_adc(1)*.1;
			temp=temp*.9+get_adc(4)*.1;
			if(temp2>sensor_max)
			{
				sensor_max=temp2;
				sensor_max_time=TCNT1;

			

				elevation=(int)((int)temp-(int)temp1);
				//printf("new max %d %d\n\r",sensor_max,sensor_max_time);
			set_out_led(1);

			}
			else
			{
				set_out_led(1);
			}
		}
	

		if((TCNT1>inhibit_off ) && (TCNT1<inhibit_on))//hackey!
		{
			if((TCNT1>oldmax-400)&&(TCNT1<oldmax+400))		
			{
				set_out_led(0);
			}
			else
			{
				set_out_led(1);
			}
	

		}
		else
		{

		set_out_led(0);

		}

	/*	*/

	/*	if((TCNT1>time_on-(500))&&(TCNT1<time_on+500))		
		{
		
			set_out_led(1);
		
		}
		else
		{
			set_out_led(0);
		}
		*/
	
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

