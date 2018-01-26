#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRComms.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

void send_msg(char);

enum states {
	IDLE,
	SENDING
};

static volatile int rcv_time = 0; // variables for reception ISR
static volatile char rcvd = 0;
static volatile char rcvd2 = 0;
static volatile int bits_rcvd = 0;

static volatile int rcv_sx = 0; // store most recent message
static volatile char lastRcv = 0;

static volatile char toSend = 0xDB; // message variables
static volatile char toRcv1 = 0xDB;
static volatile char toRcv2 = 0xA5;

enum states state = SENDING;

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs

	DDRB |= (1<<7); // enable EPM control pins
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	DDRC=0; // enable IR LED
	PORTC=0;
	DDRC = (1<<DDC3);

	cli(); // disable interrupts

	// Initialize analog compare pins
    DIDR1 = (1<<AIN1D) | (1<<AIN0D); // Disable the digital input buffers
    ACSR = (1<<ACIE) | (1<<ACIS1) | (1<<ACIS0); // Setup the comparator: enable interrupt, interrupt on rising edge

	// Initialize timer0 for timing (1/8 prescaler, 8-bit timer rolls over at ~120 Hz)
	TCCR0B |= (0<<CS02)|(1<<CS01)|(0<<CS00);

	// test power by turning on LEDs
	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	PORTC |= (1<<PORTC3); // IR

	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	PORTC &= ~(1<<PORTC3);

	sei(); // enable interrupts	

	
	while(1) {
		switch(state) {
			case SENDING: ;

				int jj=0;
				while(jj<50) { // SENDING for 5 seconds
					//cli(); // disable interrupts temporarily to ensure a complete message is sent
					//send_msg(0xA5); 
					//sei(); 
					//_delay_ms(98);
					cli();
					send_msg(0xDB);
					sei();
					_delay_ms(98);
					jj++;
				}

				state = IDLE;
				break;

			case IDLE: ;

				int ii=0;
				while(1) {
					if(TCNT0>=200) {
						ii++;
						TCNT0=0;
					}
					if(ii>=25000) { // IDLE for 5 seconds
						break;
					}
				}

				state = SENDING;
				break;

			default: // do nothing
				break;



		}
	}

}

ISR(ANALOG_COMP_vect) { // essentially the receive_msg() routine

	//PORTB &= ~(1<<PORTB2); // ensure success LED is off
	PORTB &= ~(1<<PORTB1);

	_delay_us(90);
	while(bits_rcvd<8) {

		//PORTB ^= (1<<PORTB2); // toggle LED for each received bit
		
		if (ACSR & (1<<ACO)) { // either set or clear received bit
			rcvd |= (1<<(7-bits_rcvd));
		} else {
			rcvd &= ~(1<<(7-bits_rcvd));
		}
		bits_rcvd+=1;
		if (bits_rcvd==8) { 
			_delay_us(90); 
		} else { _delay_us(190); }
	}

	bits_rcvd = 0;
	PORTB |= (1<<PORTB2);

	rcv_sx = (rcvd | 0b01111110);
	if (rcv_sx==0xFF) { // test that first and last bits are 1
		if (rcvd==toRcv1) {
			PORTB |= (1<<PORTB1);
		} else if (rcvd==toRcv2) {
			PORTB |= (1<<PORTB0);
		}
	}
	rcv_sx = 0;

	_delay_ms(10);
	PORTB &= ~( (1<<PORTB2) | (1<<PORTB1) | (1<<PORTB0) );

} 

void send_msg(char msg) { // timing values/delays are calibrated with other 
	int bits_sent=0;
	int start_bit=0;
	int new_bit=0;

	//TCNT0 = 0; // reset counter value

	//PORTC |= (1<<PORTC3); // start bit
	//PORTB |= (1<<PORTB2);

	/*while (start_bit==0) {
		if (TCNT0>190) { // end of start bit, start sending message
			start_bit=1;
			TCNT1=0;
			break;
		} else if (TCNT0>125) { // clear start bit after ~2.5 bit cycles
			PORTC &= ~(1<<PORTC3);
			PORTB &= ~(1<<PORTB2);
		}
	} */

	while(bits_sent<8) { // send first 8-bit messages
		new_bit = (msg & (1<<(7-bits_sent))) >> (7-bits_sent);
		if(new_bit==1) { // turn on LEDs
			PORTC |= (1<<PORTC3);
			PORTB |= (1<<PORTB2);
			_delay_us(100);
		} else { // turn off LEDs
			_delay_us(100);
		}
		PORTC &= ~(1<<PORTC3);
		PORTB &= ~(1<<PORTB2);
		bits_sent+=1;
		_delay_us(92); // wait one bit
	}

	/*while(bits_sent<16) { // send first 8-bit messages
		new_bit = (msg & (1<<(15-bits_sent))) >> (15-bits_sent);
		if(new_bit==1) { // turn on LEDs
			PORTC |= (1<<PORTC3);
			PORTB |= (1<<PORTB2);
			_delay_us(100);
		} else { // turn off LEDs
			_delay_us(100);
		}
		PORTC &= ~(1<<PORTC3);
		PORTB &= ~(1<<PORTB2);
		bits_sent+=1;
		_delay_us(95); // wait one bit
	}*/
	
}

