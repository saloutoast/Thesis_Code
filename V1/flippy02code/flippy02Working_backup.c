#define F_CPU 8000000UL

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
#define BAUD 9600
#define MYUBRR (((((FOSC * 10) / (16L * BAUD)) + 5) / 10))


#define ee_POWER_STATE 0x00


void setLED(unsigned char red, unsigned char green, unsigned char blue);
int switch_power(void);
int switch_tension1(void);
static int power_state;
void init(void);
uint8_t getz(void);

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


void init(void)
{

	//enable printf output
	 ioinit();

	//power on 
	DDRC |= (1<<0);
	PORTC |= (1<<0);

	//rgb led init
	DDRD |= (1<<0);
	DDRB &= ~(1<<1);


}
ISR(INT1_vect)
{
	if(power_state==1)
	{
		power_state=0;
		printf("sleeping  \n\r");
		int i;
		for(i=50;i>0;i--)
		{
			setLED(i,0,0);
		_delay_ms(10);
		}

		DDRC =0;
		PORTC = 0;

		power_state=0;
		sei();

		set_sleep_mode (SLEEP_MODE_PWR_SAVE); 
		sleep_mode();
		//power off 
	
	}
	else
	{
		printf("waking up \n\r\n\r");

		power_state=1;
		//enable printf output
		init();
		int i;
		for(i=0;i<50;i++)
		{
			setLED(0,i,0);
		_delay_ms(10);
		}
		
			_delay_ms(500);
			setLED(0,0,0);


	}


}	

uint8_t getz(void)
{


 



	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	uint8_t	SLA_W=0b11010010;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x18)	
	printf("first ack problem 0x%x \n\r",(TWSR & 0xF8));
	//printf("ack recieved OK 0x%x \n\r",(TWSR & 0xF8));

	//send data, in this case an address
	TWDR=0x3f;// accell z value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("second ack problem is 0x%x\n\r",(TWSR & 0xF8));
	//printf("second ack received OK is 0x%x\n\r",(TWSR & 0xF8));

	//send stop bit
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);


	////master receiver mode
	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));


	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x10)
	printf("start condition error 0x%x\n\r",(TWSR & 0xF8));

	//Load read addres of slave in to TWDR, 
	 SLA_W=0b11010011;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x40)	
	printf("third ack problem 0x%x \n\r",(TWSR & 0xF8));
	//printf("third ack recieved OK 0x%x \n\r",(TWSR & 0xF8));
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);

//wait for TWINT flag to se, indicating transmission and ack/nack receive
	//while(!(TWCR & (1<<TWINT)));
	//_delay_ms(10);
	//printf("data? 0x%x \n\r",TWDR);

	return(TWDR);
}
int main(void)
{

	init();	
		//enable power switch interrupt
	DDRD |= 1<<3;		// Set PD2 as input (Using for interupt INT0)
	PORTD |= 1<<3;		// Enable PD2 pull-up resistor
	EIMSK = 1<<1;					// Enable INT0
	MCUCR = 1<<ISC01 | 1<<ISC00;	// Trigger INT0 on rising edge 
	sei();	

		power_state=1;
	
	
		
	printf("\n\r\n\r\n\r\n\r--Flippy v0.2 startup--\n\r\n\r");

	//twi bit rate set for 400khz
  	TWBR=0x04;



	while(1)
	{

		//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	uint8_t SLA_W=0b11010010;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x18)	
	printf("first ack problem 0x%x \n\r",(TWSR & 0xF8));
//	printf("ack recieved OK 0x%x \n\r",(TWSR & 0xF8));

	//send data, in this case an address
	TWDR=0x6b;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("second ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("second ack received OK is 0x%x\n\r",(TWSR & 0xF8));


/////

	//send data, in this case an address
	TWDR=0x00;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("third ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("third ack received OK is 0x%x\n\r",(TWSR & 0xF8));

	//send stop bit
	TWCR = (1<<TWINT)|(1<<TWSTO);

	////


	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	SLA_W=0b11010010;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x18)	
	printf("first ack problem 0x%x \n\r",(TWSR & 0xF8));
//	printf("ack recieved OK 0x%x \n\r",(TWSR & 0xF8));

	//send data, in this case an address
	TWDR=0x3f;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("second ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("second ack received OK is 0x%x\n\r",(TWSR & 0xF8));

	//send stop bit
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);


	////master receiver mode
	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));


	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x10)
	printf("start condition error 0x%x\n\r",(TWSR & 0xF8));

	//Load read addres of slave in to TWDR, 
	 SLA_W=0b11010011;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x40)	
	printf("third ack problem 0x%x \n\r",(TWSR & 0xF8));
//	printf("third ack recieved OK 0x%x \n\r",(TWSR & 0xF8));
	
	TWCR = (1<<TWINT)|(1<<TWEN);

	
//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x58)	
	printf("third ack problem 0x%x \n\r",(TWSR & 0xF8));
//	printf("third ack recieved OK 0x%x \n\r",(TWSR & 0xF8));
	
	TWCR = (1<<TWINT)|(1<<TWSTO);
//wait for TWINT flag to se, indicating transmission and ack/nack receive
	//while(!(TWCR & (1<<TWINT)));
	//_delay_ms(10);
	printf("data? 0x%x \n\r",TWDR);

	if(TWDR<20)
	{
		setLED(TWDR,0,0);
	}
	else if(TWDR<40)
	{
		setLED(20,(TWDR-20),0);
	}
	else if(TWDR<60)
	{
		setLED(20,20,(TWDR-40));
	}
	
//	_delay_ms(1000);
	}
	
	/*
	//twi bit rate set for 400khz
  	TWBR=0x04;

	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	uint8_t SLA_W=0b11010010;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x18)	
	printf("first ack problem 0x%x \n\r",(TWSR & 0xF8));
	printf("ack recieved OK 0x%x \n\r",(TWSR & 0xF8));

	//send data, in this case an address
	TWDR=0x3b;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("second ack problem is 0x%x\n\r",(TWSR & 0xF8));
	printf("second ack received OK is 0x%x\n\r",(TWSR & 0xF8));

	//send stop bit
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);


	////master receiver mode
	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));


	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x10)
	printf("start condition error 0x%x\n\r",(TWSR & 0xF8));

	//Load read addres of slave in to TWDR, 
	 SLA_W=0b11010011;
	TWDR=SLA_W;

	//start transmission
	TWCR=(1<<TWINT)|(1<<TWEN);

	//wait for TWINT flag to se, indicating transmission and ack/nack receive
	while(!(TWCR & (1<<TWINT)));

	//check to see if ack received
	if((TWSR & 0xF8) != 0x40)	
	printf("third ack problem 0x%x \n\r",(TWSR & 0xF8));
	printf("third ack recieved OK 0x%x \n\r",(TWSR & 0xF8));
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);

//wait for TWINT flag to se, indicating transmission and ack/nack receive
	//while(!(TWCR & (1<<TWINT)));
	_delay_ms(10);
	printf("data? 0x%x \n\r",TWDR);*/

	while(1)
	{
	






	if(switch_tension1())
	setLED(10,0,0);
	else
	setLED(0,10,0);
	_delay_ms(100);
	

	}



	//PORTD  &= ~(1<<0);//turn ir led off	

	//uint8_t power_state=eeprom_read_byte((uint8_t *) (ee_POWER_STATE));//load previous state value

	/*if(power_state==0)//if 0, go to sleep
	{
		eeprom_write_byte((uint8_t *)(ee_POWER_STATE),1);//store current state in eeprom
		OCR0B = 0x00;// motor off
		OCR0A = 0x00;// motor off
		DDRB=0;
		PORTB=0;
		DDRC=0;
		PORTC=0;
		DDRD=0;
		PORTD=0;
	//	PORTD  &= ~(1<<0);//turn ir led off	
		sei();
		SMCR = 0x05;
		asm volatile("sleep\n\t");

	}
	else
	{
		eeprom_write_byte((uint8_t *)(ee_POWER_STATE),0);//store current state in eeprom

		

		//start init

		//turn on power for rest of robot
		DDRD |= (1<<7);
		PORTD |= (1<<7);

		DDRB |= 0x04;
	
		//initalize adc
		ADMUX = (1<<REFS0)|(1<<MUX1);//choose analog pin 
		ADCSRA = (1<<ADEN) |    (1<<ADPS0); //set up a/d
	

		
		//enable pins to control motor 
		DDRB |= (1<<1);
		DDRC |= (1<<0);


		//initalize motors/pwm
		DDRD |= (1<<5);
		DDRD |= (1<<6);
		TCCR0A |= (1<<COM0A1) | (1<<COM0B1) | (1<<WGM00);
		TCCR0B =0x03; //prescaler set to 0
		OCR0B = 0x00;//start with motor off
		OCR0A = 0x00;//start with motor off
	
		
		//end init
			DDRC &= ~_BV(PC4);
			DDRC &= ~_BV(PC1);
			PORTC = 0xff;

		
				//when the following code was in main, while loop started here
				ADMUX &= 0xf0;//clear adc port selection 
				ADMUX |= 6;//set adc port to left rx
				_delay_ms(1);
				ADCSRA |= (1<<ADSC);//start adc conversion to sample sensor with led off
				while((ADCSRA&(1<<ADSC))!=0);//busy wait for converstion to end
				int value=ADCW;
				if(value>608)
				setLED(0,20,0);
				else if(value>570)
				setLED(20,0,0);
				else
				setLED(0,0,0);
				_delay_ms(10);



		
			int state_counter=0;
			int next_state=1;
			int state=2;
			int r=0;
			int g=0;
			int b=0;
			
			while(1)
			{
				//when the following code was in main, while loop started here
				ADMUX &= 0xf0;//clear adc port selection 
				ADMUX |= 6;//set adc port to left rx
				_delay_ms(1);
				ADCSRA |= (1<<ADSC);//start adc conversion to sample sensor with led off
				while((ADCSRA&(1<<ADSC))!=0);//busy wait for converstion to end
				int value=ADCW;
				if(value>608)
				setLED(0,20,0);
				else if(value>570)
				setLED(20,0,0);
				else
				setLED(0,0,0);
				_delay_ms(10);

				if(state==1)//wind a
				{
					wind_a();
					state_counter++;
					if((PINC & (1<<1))!=0)//switch b
					{
						unwind_b();
					//	stop_a();
						g=20;

					}
					else
					{
						stop_b();
						wind_a();
						g=0;				

					}

					if(value>607)
					{
						state=3;
						next_state=2;
						state_counter=0;
					}


				}
				if(state==2)//wind b
				{
					wind_b();
					state_counter++;
					if((PINC & (1<<4))!=0)//switch a  wind b
					{
						unwind_a();
						//stop_b();
						r=20;
					}
					else
					{
						stop_a();
						wind_b();
						r=0;				

					}

					if(value<545)
					{
						state=3;
						next_state=1;
						state_counter=0;
					}

				}
				if(state==3)//state switch
				{

					b=20;
					int switch_state=1;

					if((PINC & (1<<4))!=0)//switch a  
					{
						stop_a();
					}
					else
					{
						wind_a();
						switch_state=0;
					}
					if((PINC & (1<<1))!=0)//switch a  
					{
						stop_b();
					}
					else
					{
						wind_b();
						switch_state=0;
					}

					if(switch_state==1)
					{
						state=next_state;
						state_counter=0;
						b=0;
					}	

				}
			
			
				
				setLED(r,g,b);



				_delay_ms(1);
			}
		
			while(1)
			{
				stop_b();
				int r=0;
				int g=0;
				if((PINC & (1<<4))!=0)//switch a
				{
					stop_a();
					r=20;
						
			
				}
				else
				{
					wind_a();
				}
				if((PINC & (1<<1))!=0)//switch b
				{
					g=20;
					stop_b();
		
				}
				else
				{
					wind_b();

				}
				setLED(r,g,0);

				_delay_ms(1);
			}
		
	
	
	
		

	}*/


}
int switch_tension1(void)
{

	if((PINB & (1<<1))!=0)//switch b
	{
		return(0);
	}
	else
	{
		return(1);
	}
}

int switch_power(void)
{

	if((PIND & (1<<3))!=0)//switch b
	{
		return(0);
	}
	else
	{
		return(1);
	}
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
			PORTD |= 0x01;
			_delay_us(2);
			PORTD &=~ 0x01;
			_delay_us(3);

			if(array[byte] & (0x80>>bit))
				PORTD |= 0x01;
			else
				PORTD &=~0x01;
			_delay_us(10); 
			PORTD &=~0x01;
			_delay_us(5);  //~50kHz	
		}
	}
	_delay_us(80);//End of Sequence
}
