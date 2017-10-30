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
#define accell_slave  0b11010000
#define	accell_master 0b11010010
#define atmega_slave 0xf0

#define MASTER 1


void setLED(unsigned char red, unsigned char green, unsigned char blue);
int switch_power(void);
int switch_tension1(void);
static int power_state;
void init(void);

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



uint8_t i2c_read_slave(uint8_t address)
{

	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	//uint8_t	SLA_W=0b11010000;
	uint8_t	SLA_W=atmega_slave;
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
	TWDR=address;// accell x value msb's
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
	 SLA_W=atmega_slave | 1;//	 SLA_W=0b11010001;
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
	return TWDR;

}



uint8_t i2c_write_slave(uint8_t data1,uint8_t data2)
{
//	printf("in slave write\n\r");
	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

//	printf("1");
	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));
	//printf("2");
	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
		//printf("3");
	//uint8_t SLA_W=0b11010000;
	uint8_t SLA_W=atmega_slave;
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
	TWDR=data1;//0x6b;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("second ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("second ack received OK is 0x%x\n\r",(TWSR & 0xF8));


/////

	//send data, in this case an address
	TWDR=data2;//data;//0x00;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("third ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("third ack received OK is 0x%x\n\r",(TWSR & 0xF8));

	//send stop bit
	TWCR = (1<<TWINT)|(1<<TWSTO);

//	printf("data1=%d data2=%d\n\r",data1,data2);
	return(0);

}

uint8_t i2c_write_accell(uint8_t accell,uint8_t address,uint8_t data)
{
	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	
	//uint8_t SLA_W=0b11010000;
	uint8_t SLA_W=accell;
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
	TWDR=address;//0x6b;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("second ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("second ack received OK is 0x%x\n\r",(TWSR & 0xF8));


/////

	//send data, in this case an address
	TWDR=data;//0x00;// accell x value msb's
	TWCR = (1<<TWINT)|(1<<TWEN);//start tx of data

	while(!(TWCR&(1<<TWINT)));//wait for data to tx

	//check for data ack	
	if((TWSR & 0xF8) != 0x28)	
	printf("third ack problem is 0x%x\n\r",(TWSR & 0xF8));
//	printf("third ack received OK is 0x%x\n\r",(TWSR & 0xF8));

	//send stop bit
	TWCR = (1<<TWINT)|(1<<TWSTO);

	return(0);

}
uint8_t i2c_read_accell(uint8_t accell,uint8_t address)
{

	//start twi transmission
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	//wait for TWCR flag to set indication start is transmitted
	while(!(TWCR &(1<<TWINT)));

	//check to see if start is an error or not
	if((TWSR & 0xF8) != 0x08)
	printf("twi error\n\r");

	//Load write addres of slave in to TWDR, 
	//uint8_t	SLA_W=0b11010000;
	uint8_t	SLA_W=accell;
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
	TWDR=address;// accell x value msb's
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
	 SLA_W=accell | 1;//	 SLA_W=0b11010001;
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
	return TWDR;

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

	//twi bit rate set 
  	TWBR=0x04;
	//setup accell


	//master/slave startup
	if(MASTER==1)
	{
	
	}
	else
	{
		TWAR=atmega_slave;
		TWCR= (1<<TWEA)|(1<<TWEN);//|(1<<TWINT);
		

	}
	
		


;
	while(1)
	{	
	static uint8_t i=0;
		if(MASTER==1)
		{
		//	printf("master loop\n\r");
		//	i2c_write_accell(accell_slave,0x6b, 0x00);
		//	uint8_t accelval_s=i2c_read_accell(accell_slave,0x3f);
		
			static int i=0;
			i2c_write_slave(1,(uint8_t)switch_tension1());
			if((uint8_t)switch_tension1()==0)
			{
				setLED(10,0,0);
				}
				else
				{

					setLED(0,0,0);

				}
				_delay_ms(2);
			
		}
		else
		{
			static uint8_t data[4];
			static uint8_t status[4];
			static uint8_t new_message=0;
				if((TWCR & (1<<TWINT)))
				{

					if((TWSR & 0xF8)==0x60)
					{new_message=0;
					TWCR= (1<<TWEA)|(1<<TWEN)|(1<<TWINT);
						data[new_message]=TWDR;
						status[new_message]=60;
						new_message++;
					//	printf("message %d, data%d, state%d\n\r",new_message,(TWSR & 0xF8),data);
					
					}
					else if((TWSR & 0xF8)==0x80)
					{
						data[new_message]=TWDR;
						
						status[new_message]=80;
					new_message++;
					TWCR= (1<<TWEA)|(1<<TWEN)|(1<<TWINT);
					
					//	printf("message %d, data%d, state%d\n\r",new_message,(TWSR & 0xF8),data);
				

					}
					else if((TWSR & 0xF8)==0xa0)
					{
//
						TWCR= (1<<TWEA)|(1<<TWEN)|(1<<TWINT);
					
						data[new_message]=TWDR;
						
						status[new_message]=128;
						new_message++;
					
					//	printf("message %d, data%d, state%d\n\r",new_message,(TWSR & 0xF8),data);
					}
				

					
			
		

				}	
					if(new_message==3)
					{
				
						
						printf("data=%d,%d\n\r",data[1],data[2]);
						//printf("s=%d s=%d s=%d s=%d\n\r",status[0],status[1],status[2],status[2]);
					
						if((data[1]==1)&&(data[2]==0))
						{
							setLED(10,0,0);

						}
						else
						{
							setLED(0,0,0);
						}
						new_message=0;
					}
			
		
		

		}

	}





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
