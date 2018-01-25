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
static volatile char rcvd1 = 0;
static volatile char rcvd2 = 0;
static volatile int bits_rcvd = 0;

static volatile int rcv_sx = 0; // store most recent message
static volatile char lastRcv = 0;

static volatile char toSend = 0xAA; // message variables
static volatile char toRcv = 0xAA;

enum states state = IDLE;

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
    
	sei(); // enable interrupts

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

	int ii=0;
	while(1) {
		switch(state) {
			case SENDING:

				//PORTB |= ( (1<<PORTB0) | (1<<PORTB1) ); // debugging LEDs
				
				cli(); // disable interrupts temporarily to ensure a complete message is sent
				send_msg(toSend); 
				sei(); 

				state = IDLE; // return to IDLE state

				//_delay_ms(100);

				//PORTB &= ~( (1<<PORTB0) | (1<<PORTB1) );

				break;

			case IDLE:
				ii++; // wait to receive messages

				//_delay_ms(100);
				//state = SENDING;
				if (ii>=20000) { // send a message once in a while (10000 loops -> ~16ms)
					state = SENDING;
					ii=0;
				} 

				break;

			default: // do nothing
				break;



		}
	}

}

ISR(ANALOG_COMP_vect) { // essentially the receive_msg() routine

	TCNT0 = 0; // reset counter
	PORTB |= (1<<PORTB0); // turn on LED to indicate potential start bit
	PORTB &= ~(1<<PORTB1); // ensure success LED is off
	rcv_sx = 0; // reset receive success flag

	while ( TCNT0 < 100) { // while potential "start" bit should be ongoing
		if ((ACSR&(1<<ACO))==0) { // record time after falling edge
			rcv_time = TCNT0;
			break;
		}
	}

	if ((rcv_time>70)&(rcv_time<80)) { // check that it was a legitimate start bit

		PORTB &= ~(1<<PORTB2); // clear start bit LED

		while (bits_rcvd<8) { // receive first message
			_delay_us(50); // wait until middle of next bit
			rcvd1 |= ( ((ACSR&(1<<ACO))>>5) << (7-bits_rcvd) ); // set new bit
			bits_rcvd+=1;
		}

		while (bits_rcvd<16) { // receive second message
			_delay_us(50); // wait until middle of next bit
			rcvd2 |= ( ((ACSR&(1<<ACO))>>5) << (16-bits_rcvd) ); // set new bit
			bits_rcvd+=1;
		}

		if (rcvd1==rcvd2) {// check if messages are the same; set success flag here
			
			PORTB |= (1<<PORTB1); // turn on success LED for transmission
			if (rcvd1==toRcv) { // check if message is the same as expected
				rcv_sx = 1;
				lastRcv &= 0; // reset lastRcv variable
				lastRcv |= rcvd1; // store most recent sucessful message
				PORTB |= (1<<PORTB0); // second LED for correct message
			}
		}
	}
	// if start bit was not legitimate, exit ISR

	rcvd1 = 0; // reset variables for next ISR call
	rcvd2 = 0;
	bits_rcvd = 0;

} 

void send_msg(char msg) { // timing values/delays are calibrated with other 
	int bits_sent=0;
	int start_bit=0;
	int new_bit=0;

	TCNT0 = 0; // reset counter value

	PORTC |= (1<<PORTC3); // start bit
	PORTB |= (1<<PORTB2);

	while (start_bit==0) {
		if (TCNT0>190) { // end of start bit, start sending message
			start_bit=1;
			TCNT1=0;
			break;
		} else if (TCNT0>125) { // clear start bit after ~1.5 bit cycles
			PORTC &= ~(1<<PORTC3);
			PORTB &= ~(1<<PORTB2);
		}
	}

	while(bits_sent<8) { // send first 8-bit messages
		new_bit = (msg & (1<<(7-bits_sent))) >> (7-bits_sent);
		if(new_bit==1) { // turn on LEDs
			PORTC |= (1<<PORTC3);
			PORTB |= (1<<PORTB2);
		} else { // turn off LEDs
			PORTC &= ~(1<<PORTC3);
			PORTB &= ~(1<<PORTB2);
		}
		bits_sent+=1;
		_delay_us(42); // wait one bit
	}

	while(bits_sent<16) { // send first 8-bit messages
		new_bit = (msg & (1<<(15-bits_sent))) >> (15-bits_sent);
		if(new_bit==1) { // turn on LEDs
			PORTC |= (1<<PORTC3);
			PORTB |= (1<<PORTB2);
		} else { // turn off LEDs
			PORTC &= ~(1<<PORTC3);
			PORTB &= ~(1<<PORTB2);
		}
		bits_sent+=1;
		_delay_us(42); // wait one bit
	}

	PORTC &= ~(1<<PORTC3); // ensure LEDs are turned off after last bit of the message
	PORTB &= ~(1<<PORTB2);
	
}

