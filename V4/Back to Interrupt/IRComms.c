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

static volatile char toSend = 0xFF; // message variables
static volatile char toRcv1 = 0xDB;
static volatile char toRcv2 = 0xA5;

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

	// Initialize timer0 for timing sending of messages (1/8 prescaler, 8-bit timer rolls over at ~120 Hz)
	TCCR0B |= (0<<CS02)|(1<<CS01)|(0<<CS00); // TODO: set to CTC mode?

	// Initialize timer2 for timing receiving of messages (___ prescaler, 8-bit timer rolls over at ____ Hz )
	// TODO: set up timer here for CTC
	/* TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 50; // compare every 50 counts (every 50us (20kHz), 10x frequency of communication bits)
    TCCR2B |= (0<<CS22)|(1<<CS21)|(0<<CS20); // prescaler of 1/8: count every 1us */

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
		
		// TODO: main loop -> validate messages, act on messages

		}
	}

}

ISR(ANALOG_COMP_vect) { // essentially the receive_msg() routine

	if (rcving==0) {
		TCNT2=0;
		rcving=1;
		rcvd=0x80;
		ACSR &= ~(1<<ACIS0); // change to falling edge
	}

	// TODO: time first pulse and record distance, switch back to rising edge

	// TODO: match rising edges to closest expected time in rcvd

	// TODO: if bit 8 is received, set "new message" flag and record message


	/* // simple code to follow pulse train
	while(ACSR & (1<<ACO)) {
		PORTB |= (1<<PORTB2);
	}
	PORTB &= ~(1<<PORTB2); */

} 

// TODO: ISR for timer 2, clear receiving variables on rollover


// TODO: ISR for timer 0 to send messages
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
}

